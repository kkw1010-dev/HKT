#include "Jujutsu.h"

#include "Settings.h"
#include "Util.h"

#include "ValhallaCombat/API.h"

#include <random>

namespace CIGAR
{
	namespace
	{
		// The four vanilla hand-to-hand kill moves the user named. ValhallaCombat.esp carries copies
		// without conditions; Update.esm's BodySlam and KneeThrow carry GetRandomPercent <= 33 / 25,
		// which may refuse a forced play, so Valhalla's copies are preferred when it is loaded.
		struct IdleRef
		{
			const char* name;
			RE::FormID id;
			std::string_view plugin;
		};
		constexpr std::array kValhallaIdles{
			IdleRef{ "KneeThrow", 0xAA3A, "ValhallaCombat.esp"sv },
			IdleRef{ "BodySlam", 0xAA3B, "ValhallaCombat.esp"sv },
			IdleRef{ "ComboA", 0xAA3C, "ValhallaCombat.esp"sv },
			IdleRef{ "SlamA", 0xAA3D, "ValhallaCombat.esp"sv },
		};
		constexpr std::array kVanillaIdles{
			IdleRef{ "KneeThrow", 0x821, "Update.esm"sv },     // H2HKillMoveKneeThrow
			IdleRef{ "BodySlam", 0x820, "Update.esm"sv },      // H2HKillMoveBodySlam
			IdleRef{ "ComboA", 0x0F9958, "Skyrim.esm"sv },     // pa_KillMoveH2HComboA
			IdleRef{ "SlamA", 0x100EF8, "Skyrim.esm"sv },      // H2HKillMoveSlamA00
		};

		constexpr auto kSexLabPlugin = "SexLab.esm"sv;
		constexpr RE::FormID kSexLabAnimatingID = 0xE50F;
		constexpr RE::FormID kHumanoidBodyPartData = 0x1D;

		// Kill moves snap the pair together from about this distance (Valhalla uses the same for its own).
		constexpr float kReach = 250.0f;
		// A guard drops between blows; keep offering the target this long after it was last seen blocking.
		constexpr auto kBlockGrace = 700ms;
		constexpr auto kStartWindow = 1s;
		constexpr auto kPairTimeout = 10s;
		// KillActor can arrive just after PairEnd (ComboA: 2.768 s vs 2.757 s), so the hook stays armed
		// while the victim's state is watched after the pair.
		constexpr auto kSettle = 2s;

		// Placeholder balance, not the user's numbers: the user asked for the function first.
		constexpr float kStunShare = 0.5f;     // of Valhalla's max stun, (base health + base stamina) / 2
		constexpr float kHealthShare = 0.05f;  // of max health; never takes the victim below 1

		using KillActorFn = bool (*)(RE::AnimHandler*, RE::Actor&, const RE::BSFixedString&);
		KillActorFn originalKillActor = nullptr;
		std::atomic<RE::Actor*> armedVictim{ nullptr };
		// Bit 1: KillActor for the victim was swallowed; bit 2: for the player.
		std::atomic<int> swallowed{ 0 };

		bool IsHumanoid(RE::Actor* a_actor)
		{
			const auto* race = a_actor ? a_actor->GetRace() : nullptr;
			return race && race->bodyPartData && race->bodyPartData->GetFormID() == kHumanoidBodyPartData;
		}

		float AV(RE::Actor* a_actor, RE::ActorValue a_value)
		{
			return a_actor->AsActorValueOwner()->GetActorValue(a_value);
		}

		void DamageAV(RE::Actor* a_actor, RE::ActorValue a_value, float a_amount)
		{
			a_actor->AsActorValueOwner()->DamageActorValue(a_value, a_amount);
		}
	}

	bool Jujutsu::KillActorHook(RE::AnimHandler* a_this, RE::Actor& a_actor, const RE::BSFixedString& a_parameter)
	{
		// Runs wherever the animation graph delivers its events: touch atomics only.
		if (auto* victimNow = armedVictim.load(); victimNow) {
			if (&a_actor == victimNow) {
				swallowed.fetch_or(1);
				return true;
			}
			if (&a_actor == RE::PlayerCharacter::GetSingleton()) {
				swallowed.fetch_or(2);
				return true;
			}
		}
		return originalKillActor(a_this, a_actor, a_parameter);
	}

	void Jujutsu::InstallHook()
	{
		if (originalKillActor) {
			return;
		}
		REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_KillActorHandler[0] };
		// Slot 0 is the destructor; slot 1 is IHandlerFunctor::ExecuteHandler.
		originalKillActor = reinterpret_cast<KillActorFn>(vtbl.write_vfunc(1, reinterpret_cast<std::uintptr_t>(&KillActorHook)));
		logs::info("[Jujutsu] KillActorHandler::ExecuteHandler hooked (original {:X})", reinterpret_cast<std::uintptr_t>(originalKillActor));
	}

	Jujutsu::Jujutsu()
	{
		jujutsu.SetRepeat(true);
	}

	Jujutsu* Jujutsu::GetSingleton()
	{
		static Jujutsu singleton;
		return &singleton;
	}

	void Jujutsu::OnGameLoaded()
	{
		jujutsu.Reset();
		lastGate.clear();
		armedVictim = nullptr;
		swallowed = 0;
		phase = Phase::kIdle;
		victim = {};
		offeredTarget = nullptr;
		lastBlocker = {};
		idles.clear();

		auto* handler = RE::TESDataHandler::GetSingleton();
		const bool valhallaEsp = handler && handler->LookupModByName("ValhallaCombat.esp"sv);
		const auto& table = valhallaEsp ? std::span<const IdleRef>(kValhallaIdles) : std::span<const IdleRef>(kVanillaIdles);
		idleSource = valhallaEsp ? "ValhallaCombat.esp (no conditions)" : "Skyrim.esm/Update.esm";
		std::string found;
		for (const auto& ref : table) {
			auto* idle = handler ? handler->LookupForm<RE::TESIdleForm>(ref.id, ref.plugin) : nullptr;
			found += std::format(" {}={}", ref.name, idle ? std::format("{:08X}", idle->GetFormID()) : "missing"s);
			if (idle) {
				idles.push_back(idle);
			}
		}
		valhalla = GetModuleHandleW(L"ValhallaCombat.dll") ? VAL_API::RequestPluginAPI() : nullptr;
		sexlabAnimating = handler && handler->LookupModByName(kSexLabPlugin) ?
		                      handler->LookupForm<RE::TESFaction>(kSexLabAnimatingID, kSexLabPlugin) :
		                      nullptr;
		Log("idles from {}:{}; hook={} valhalla={} sexlab={}", idleSource, found, originalKillActor != nullptr, valhalla != nullptr,
			sexlabAnimating != nullptr);
		if (idles.empty() || !originalKillActor) {
			Log("WARN {}; the 유술 prompt is off", idles.empty() ? "no kill-move idle resolved" : "the KillActor hook is not installed");
			return;
		}
		Log("ready");
	}

	RE::Actor* Jujutsu::FindTarget(RE::PlayerCharacter* a_player, std::string& a_gate) const
	{
		const auto* controls = RE::ControlMap::GetSingleton();
		if (!controls || !controls->IsMovementControlsEnabled()) {
			a_gate = "why=no-controls";
			return nullptr;
		}
		if (sexlabAnimating && a_player->IsInFaction(sexlabAnimating)) {
			a_gate = "why=sexlab";
			return nullptr;
		}
		if (a_player->IsDead() || a_player->IsInKillMove() || a_player->IsOnMount() || !IsHumanoid(a_player)) {
			a_gate = "why=player";
			return nullptr;
		}
		RE::Actor* blocking = nullptr;
		RE::Actor* recent = nullptr;
		const auto lastBlockerNow = lastBlocker.get();
		for (auto* actor : Util::NearbyHostiles(a_player, kReach)) {
			if (!IsHumanoid(actor) || actor->IsInKillMove() || actor->IsOnMount() || actor->IsPlayerTeammate()) {
				continue;
			}
			if (actor->IsBlocking()) {
				blocking = actor;
				break;
			}
			if (!recent && actor == lastBlockerNow.get() && Clock::now() - blockSeen < kBlockGrace) {
				recent = actor;
			}
		}
		auto* target = blocking ? blocking : recent;
		// Blocking flips with every guard raised and dropped, so the gate carries the target, not the flag.
		a_gate = std::format("target={} why={}", target ? Util::NameOf(target) : "-"s, target ? "-" : "no-blocking-humanoid");
		return target;
	}

	void Jujutsu::FastTick()
	{
		if (idles.empty() || !originalKillActor) {
			return;
		}
		auto* player = Util::Player();
		if (phase != Phase::kIdle) {
			Watch(player);
			LogGate("busy: 유술 in progress");
			jujutsu.Update(false, [] { return ""s; });
			return;
		}
		std::string gate;
		auto* target = FindTarget(player, gate);
		// Remember who blocked, for the grace period (read from the gate's own search).
		if (target && target->IsBlocking()) {
			lastBlocker = target->GetHandle();
			blockSeen = Clock::now();
		}
		LogGate(std::move(gate));
		if (jujutsu.Offered() && target != offeredTarget) {
			jujutsu.Withdraw();
			jujutsu.Reset();
		}
		offeredTarget = target;
		jujutsu.Update(target != nullptr, [target] { return std::format("유술: {}", Util::NameOf(target)); });
	}

	void Jujutsu::OnAccepted(std::uint16_t a_eventID)
	{
		if (a_eventID != kJujutsu || phase != Phase::kIdle) {
			return;
		}
		auto* player = Util::Player();
		std::string gate;
		auto* target = FindTarget(player, gate);
		if (!target) {
			Log("accept ignored, no target now: {}", gate);
			return;
		}
		Start(player, target);
	}

	void Jujutsu::Start(RE::PlayerCharacter* a_player, RE::Actor* a_victim)
	{
		auto* process = a_player->GetActorRuntimeData().currentProcess;
		if (!process) {
			Log("WARN player has no AI process; 유술 not started");
			return;
		}
		static std::mt19937 rng{ std::random_device{}() };
		playing = idles[std::uniform_int_distribution<std::size_t>(0, idles.size() - 1)(rng)];
		swallowed = 0;
		armedVictim = a_victim;
		victim = a_victim->GetHandle();
		payoffDone = false;
		Log("start idle {:08X} on {} ({:08X}); victim before: {}", playing->GetFormID(), Util::NameOf(a_victim), a_victim->GetFormID(),
			DescribeVictim(a_victim));
		// Valhalla plays its execution idles the same way (playPairedIdle = AIProcess::SetupSpecialIdle).
		const bool requested = process->SetupSpecialIdle(a_player, RE::DEFAULT_OBJECT::kActionIdle, playing, true, false, a_victim);
		Log("SetupSpecialIdle returned {}", requested);
		phase = Phase::kStarting;
		phaseStart = Clock::now();
	}

	std::string Jujutsu::DescribeVictim(RE::Actor* a_victim) const
	{
		if (!a_victim) {
			return "gone";
		}
		bool synced = false;
		a_victim->GetGraphVariableBool("bIsSynced", synced);
		const auto* state = a_victim->AsActorState();
		return std::format("dead={} health={:.0f}/{:.0f} stamina={:.0f} bleedout={} knock={} ragdoll={} killmove={} synced={} blocking={}{}",
			a_victim->IsDead(), AV(a_victim, RE::ActorValue::kHealth), a_victim->GetActorValueMax(RE::ActorValue::kHealth),
			AV(a_victim, RE::ActorValue::kStamina), state && state->IsBleedingOut(),
			state ? static_cast<int>(state->GetKnockState()) : -1, a_victim->IsInRagdollState(), a_victim->IsInKillMove(), synced,
			a_victim->IsBlocking(), valhalla ? std::format(" stunned={}", valhalla->isActorStunned(a_victim)) : ""s);
	}

	void Jujutsu::ApplyPayoff(RE::PlayerCharacter* a_player, RE::Actor* a_victim)
	{
		payoffDone = true;
		if (!a_victim || a_victim->IsDead()) {
			Log("payoff skipped: victim {}", a_victim ? "dead" : "gone");
			return;
		}
		std::string what;
		if (valhalla) {
			auto* owner = a_victim->AsActorValueOwner();
			const float maxStun = (owner->GetPermanentActorValue(RE::ActorValue::kHealth) + owner->GetPermanentActorValue(RE::ActorValue::kStamina)) / 2.0f;
			const float stun = maxStun * kStunShare;
			// timedBlock applies the base damage times fStunTimedBlockMult (1 here) and nothing else.
			valhalla->processStunDamage(VAL_API::timedBlock, nullptr, a_player, a_victim, stun);
			what = std::format("stun {:.0f} of max {:.0f} (stunned now={})", stun, maxStun, valhalla->isActorStunned(a_victim));
		} else {
			const float stamina = AV(a_victim, RE::ActorValue::kStamina);
			DamageAV(a_victim, RE::ActorValue::kStamina, stamina);
			what = std::format("stamina {:.0f} -> {:.0f}", stamina, AV(a_victim, RE::ActorValue::kStamina));
		}
		const float health = AV(a_victim, RE::ActorValue::kHealth);
		const float damage = std::min(a_victim->GetActorValueMax(RE::ActorValue::kHealth) * kHealthShare, std::max(0.0f, health - 1.0f));
		DamageAV(a_victim, RE::ActorValue::kHealth, damage);
		Log("payoff: {}; health {:.0f} -> {:.0f}", what, health, AV(a_victim, RE::ActorValue::kHealth));
	}

	void Jujutsu::Watch(RE::PlayerCharacter* a_player)
	{
		auto victimPtr = victim.get();
		auto* v = victimPtr.get();
		const auto now = Clock::now();
		if (const int seen = swallowed.exchange(0); seen) {
			Log("KillActor swallowed for{}{} at {:.2f} s", seen & 1 ? " victim" : "", seen & 2 ? " player" : "",
				std::chrono::duration<float>(now - phaseStart).count());
			if (!payoffDone) {
				ApplyPayoff(a_player, v);
			}
		}
		bool synced = false;
		a_player->GetGraphVariableBool("bIsSynced", synced);
		const bool pairOn = synced || a_player->IsInKillMove() || (v && v->IsInKillMove());
		switch (phase) {
		case Phase::kStarting:
			if (pairOn) {
				phase = Phase::kRunning;
				Log("pair started after {:.2f} s (synced={} playerKillMove={} victimKillMove={})",
					std::chrono::duration<float>(now - phaseStart).count(), synced, a_player->IsInKillMove(), v && v->IsInKillMove());
			} else if (now - phaseStart >= kStartWindow) {
				Log("WARN idle {:08X} did not start a pair within 1 s; victim: {}", playing->GetFormID(), DescribeVictim(v));
				if (!warnedNoStart) {
					warnedNoStart = true;
					Util::Notify("CIGAR: 유술 모션 미발동. 로그 확인");
				}
				Finish("no start");
			}
			break;
		case Phase::kRunning:
			if (!pairOn) {
				if (!payoffDone) {
					Log("pair ended without a KillActor event; payoff applied at the end");
					ApplyPayoff(a_player, v);
				}
				Log("pair ended after {:.2f} s; victim: {}", std::chrono::duration<float>(now - phaseStart).count(), DescribeVictim(v));
				phase = Phase::kSettling;
				settleUntil = now + kSettle;
			} else if (now - phaseStart >= kPairTimeout) {
				Log("WARN pair still running after 10 s; victim: {}", DescribeVictim(v));
				Finish("timeout");
			}
			break;
		case Phase::kSettling:
			if (now >= settleUntil) {
				Log("2 s after the pair: victim: {}", DescribeVictim(v));
				if (v && v->IsDead() && !warnedDied) {
					warnedDied = true;
					Log("WARN the victim died although KillActor was swallowed{}", payoffDone ? "" : " (no KillActor seen)");
					Util::Notify("CIGAR: 유술 대상 사망. 로그 확인");
				}
				Finish("done");
			}
			break;
		default:
			break;
		}
	}

	void Jujutsu::Finish(const char* a_reason)
	{
		armedVictim = nullptr;
		swallowed = 0;
		phase = Phase::kIdle;
		victim = {};
		Log("finished ({})", a_reason);
	}

	void Jujutsu::OnDisabled()
	{
		if (phase != Phase::kIdle) {
			Finish("module switched off");
		}
	}
}
