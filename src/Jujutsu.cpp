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

		// A guard drops between blows; keep offering the target this long after it was last seen blocking.
		constexpr auto kBlockGrace = 700ms;
		// Test 1: SetupSpecialIdle returned false on every victim that was blocking at that moment, and
		// true on the two that had just lowered their guard. Drop the guard and retry for this long.
		constexpr auto kPrepareWindow = 600ms;
		constexpr auto kStartWindow = 1s;
		constexpr auto kPairTimeout = 10s;
		// KillActor can arrive just after PairEnd (ComboA: 2.768 s vs 2.757 s), so the hooks stay armed
		// while the victim's state is watched after the pair.
		constexpr auto kSettle = 2s;

		// The push that knocks the victim into ragdoll at the end of the throw (Papyrus PushActorAway uses
		// the same call). Small, so it falls where it lies rather than flying: a placeholder to tune.
		constexpr float kKnockMagnitude = 1.0f;
		constexpr auto kKnockCheck = 400ms;

		// Placeholder balance, not the user's numbers: the user asked for the function first.
		constexpr float kStunShare = 0.5f;     // of Valhalla's max stun, (base health + base stamina) / 2
		constexpr float kHealthShare = 0.05f;  // of max health; never takes the victim below 1

		using HandlerFn = bool (*)(RE::AnimHandler*, RE::Actor&, const RE::BSFixedString&);
		HandlerFn originalKillActor = nullptr;
		HandlerFn originalKillMoveStart = nullptr;
		HandlerFn originalKillMoveEnd = nullptr;

		// Written by the game thread, read by the hooks (which run wherever graph events are delivered).
		std::atomic<RE::Actor*> armedVictim{ nullptr };
		std::atomic<std::int64_t> armedAtMs{ 0 };
		// Milliseconds after arming at which each victim event arrived, or -1; the game thread logs them.
		std::atomic<std::int64_t> killActorAt{ -1 };
		std::atomic<std::int64_t> playerKillActorAt{ -1 };
		std::atomic<std::int64_t> killMoveStartAt{ -1 };
		std::atomic<std::int64_t> killMoveEndAt{ -1 };

		std::int64_t NowMs()
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		}

		void Stamp(std::atomic<std::int64_t>& a_slot)
		{
			std::int64_t expected = -1;
			a_slot.compare_exchange_strong(expected, NowMs() - armedAtMs.load());
		}

		bool KillActorHook(RE::AnimHandler* a_this, RE::Actor& a_actor, const RE::BSFixedString& a_parameter)
		{
			if (auto* v = armedVictim.load(); v) {
				if (&a_actor == v) {
					Stamp(killActorAt);
					return true;
				}
				if (&a_actor == RE::PlayerCharacter::GetSingleton()) {
					Stamp(playerKillActorAt);
					return true;
				}
			}
			return originalKillActor(a_this, a_actor, a_parameter);
		}

		bool KillMoveStartHook(RE::AnimHandler* a_this, RE::Actor& a_actor, const RE::BSFixedString& a_parameter)
		{
			if (&a_actor == armedVictim.load()) {
				Stamp(killMoveStartAt);
			}
			return originalKillMoveStart(a_this, a_actor, a_parameter);
		}

		bool KillMoveEndHook(RE::AnimHandler* a_this, RE::Actor& a_actor, const RE::BSFixedString& a_parameter)
		{
			if (&a_actor == armedVictim.load()) {
				// This is the event that kills a kill-move victim.
				Stamp(killMoveEndAt);
				return true;
			}
			return originalKillMoveEnd(a_this, a_actor, a_parameter);
		}

		HandlerFn HookSlot(REL::VariantID a_vtable, HandlerFn a_hook)
		{
			REL::Relocation<std::uintptr_t> vtbl{ a_vtable };
			// Slot 0 is the destructor; slot 1 is IHandlerFunctor::ExecuteHandler.
			return reinterpret_cast<HandlerFn>(vtbl.write_vfunc(1, reinterpret_cast<std::uintptr_t>(a_hook)));
		}

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

	void Jujutsu::InstallHook()
	{
		if (originalKillActor) {
			return;
		}
		originalKillActor = HookSlot(RE::VTABLE_KillActorHandler[0], KillActorHook);
		originalKillMoveStart = HookSlot(RE::VTABLE_KillMoveStartHandler[0], KillMoveStartHook);
		originalKillMoveEnd = HookSlot(RE::VTABLE_KillMoveEndHandler[0], KillMoveEndHook);
		logs::info("[Jujutsu] anim handlers hooked: KillActor={} KillMoveStart={} KillMoveEnd={}", originalKillActor != nullptr,
			originalKillMoveStart != nullptr, originalKillMoveEnd != nullptr);
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
		const bool hooked = originalKillActor && originalKillMoveStart && originalKillMoveEnd;
		Log("idles from {}:{}; hooks={} valhalla={} sexlab={}", idleSource, found, hooked, valhalla != nullptr, sexlabAnimating != nullptr);
		if (idles.empty() || !hooked) {
			Log("WARN {}; the 유술 prompt is off", idles.empty() ? "no kill-move idle resolved" : "the anim-handler hooks are not installed");
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
		for (auto* actor : Util::NearbyHostiles(a_player, Settings::JujutsuReach())) {
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
		// Remember who blocked, for the grace period.
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
		static std::mt19937 rng{ std::random_device{}() };
		playing = idles[std::uniform_int_distribution<std::size_t>(0, idles.size() - 1)(rng)];
		victim = target->GetHandle();
		payoffDone = false;
		knocked = false;
		tries = 0;
		lastSample.clear();
		phase = Phase::kPreparing;
		phaseStart = Clock::now();
		// The distance at the press, beside each retry's and the start's: tells a play refused because the
		// target moved from one the engine refused at close range.
		Log("start idle {:08X} on {} ({:08X}) distance={:.0f} reach={:.0f}; victim before: {}", playing->GetFormID(), Util::NameOf(target),
			target->GetFormID(), player->GetPosition().GetDistance(target->GetPosition()), Settings::JujutsuReach(), DescribeVictim(target));
		if (TryPlay(player, target)) {
			phase = Phase::kStarting;
			phaseStart = Clock::now();
		}
	}

	bool Jujutsu::TryPlay(RE::PlayerCharacter* a_player, RE::Actor* a_victim)
	{
		auto* process = a_player->GetActorRuntimeData().currentProcess;
		if (!process) {
			Log("WARN player has no AI process");
			return false;
		}
		++tries;
		const bool wasBlocking = a_victim->IsBlocking();
		if (wasBlocking) {
			a_victim->NotifyAnimationGraph("blockStop");
		}
		// Armed before the call: KillMoveStart may be delivered during it.
		killActorAt = -1;
		playerKillActorAt = -1;
		killMoveStartAt = -1;
		killMoveEndAt = -1;
		armedAtMs = NowMs();
		armedVictim = a_victim;
		// Valhalla plays its execution idles the same way (playPairedIdle = AIProcess::SetupSpecialIdle).
		const bool requested = process->SetupSpecialIdle(a_player, RE::DEFAULT_OBJECT::kActionIdle, playing, true, false, a_victim);
		// Test 2: about half the plays were refused, many with no guard up. Log what both actors were doing.
		Log("try {}: {}{} SetupSpecialIdle returned {}", tries, wasBlocking ? "(sent blockStop) " : "", DescribeRefusal(a_player, a_victim),
			requested);
		if (!requested) {
			armedVictim = nullptr;
		}
		return requested;
	}

	std::string Jujutsu::DescribeRefusal(RE::PlayerCharacter* a_player, RE::Actor* a_victim) const
	{
		const auto state = [](RE::Actor* a_actor) {
			const auto* s = a_actor->AsActorState();
			bool staggered = false;
			a_actor->GetGraphVariableBool("IsStaggering", staggered);
			bool synced = false;
			a_actor->GetGraphVariableBool("bIsSynced", synced);
			return std::format("attack={} knock={} stagger={} synced={} killmove={} sprint={} ragdoll={}",
				s ? static_cast<int>(s->GetAttackState()) : -1, s ? static_cast<int>(s->GetKnockState()) : -1, staggered, synced,
				a_actor->IsInKillMove(), s && s->IsSprinting(), a_actor->IsInRagdollState());
		};
		return std::format("victim[blocking={} {}] player[weaponDrawn={} {}] distance={:.0f}", a_victim->IsBlocking(), state(a_victim),
			a_player->AsActorState()->IsWeaponDrawn(), state(a_player), a_player->GetPosition().GetDistance(a_victim->GetPosition()));
	}

	void Jujutsu::EndKillMove(RE::PlayerCharacter* a_player, RE::Actor* a_victim)
	{
		if (!a_victim || a_victim->IsDead()) {
			return;
		}
		// KillMoveEnd was swallowed, and it would have cleared this flag.
		if (a_victim->IsInKillMove()) {
			a_victim->GetActorRuntimeData().boolFlags.reset(RE::Actor::BOOL_FLAGS::kIsInKillMove);
			Log("cleared the victim's in-kill-move flag");
		}
		auto* process = a_victim->GetActorRuntimeData().currentProcess;
		if (!process) {
			Log("WARN victim has no AI process; not knocked down");
			return;
		}
		process->KnockExplosion(a_victim, a_player->GetPosition(), kKnockMagnitude);
		knocked = true;
		Log("knocked the victim into ragdoll (magnitude {:.1f})", kKnockMagnitude);
	}

	float Jujutsu::Elapsed() const
	{
		return std::chrono::duration<float>(Clock::now() - phaseStart).count();
	}

	std::string Jujutsu::DescribeVictim(RE::Actor* a_victim) const
	{
		if (!a_victim) {
			return "gone";
		}
		bool synced = false;
		a_victim->GetGraphVariableBool("bIsSynced", synced);
		const auto* state = a_victim->AsActorState();
		return std::format(
			"dead={} life={} health={:.0f}/{:.0f} stamina={:.0f} bleedout={} knock={} ragdoll={} killmove={} essential={} synced={} blocking={}{}",
			a_victim->IsDead(), state ? static_cast<int>(state->GetLifeState()) : -1, AV(a_victim, RE::ActorValue::kHealth),
			a_victim->GetActorValueMax(RE::ActorValue::kHealth), AV(a_victim, RE::ActorValue::kStamina), state && state->IsBleedingOut(),
			state ? static_cast<int>(state->GetKnockState()) : -1, a_victim->IsInRagdollState(), a_victim->IsInKillMove(),
			a_victim->IsEssential(), synced, a_victim->IsBlocking(),
			valhalla ? std::format(" stunned={}", valhalla->isActorStunned(a_victim)) : ""s);
	}

	void Jujutsu::Sample(RE::Actor* a_victim, float a_time)
	{
		// Logged only when the victim's life-relevant state changes, to time the death to 100 ms.
		if (!a_victim) {
			return;
		}
		const auto* state = a_victim->AsActorState();
		auto sample = std::format("dead={} life={} health={:.0f} bleedout={} killmove={} ragdoll={}", a_victim->IsDead(),
			state ? static_cast<int>(state->GetLifeState()) : -1, AV(a_victim, RE::ActorValue::kHealth), state && state->IsBleedingOut(),
			a_victim->IsInKillMove(), a_victim->IsInRagdollState());
		if (sample != lastSample) {
			Log("t={:.2f}s victim {}", a_time, sample);
			lastSample = std::move(sample);
		}
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
		const float t = Elapsed();

		if (phase == Phase::kPreparing) {
			if (v && TryPlay(a_player, v)) {
				phase = Phase::kStarting;
				phaseStart = now;
			} else if (!v || now - phaseStart >= kPrepareWindow) {
				Log("WARN the kill move was refused for {:.1f} s ({} tries); victim: {}", t, tries, DescribeVictim(v));
				if (!warnedNoStart) {
					warnedNoStart = true;
					Util::Notify("CIGAR: 유술 모션 미발동. 로그 확인");
				}
				Finish("refused");
			}
			return;
		}

		Sample(v, t);
		const auto report = [this](std::atomic<std::int64_t>& a_slot, const char* a_what) {
			if (const auto ms = a_slot.exchange(-1); ms >= 0) {
				Log("{} at {:.2f} s after the request", a_what, static_cast<float>(ms) / 1000.0f);
				return true;
			}
			return false;
		};
		report(killMoveStartAt, "victim KillMoveStart (passed through)");
		const bool killActor = report(killActorAt, "victim KillActor swallowed");
		report(playerKillActorAt, "player KillActor swallowed");
		report(killMoveEndAt, "victim KillMoveEnd swallowed");
		if (killActor && !payoffDone) {
			ApplyPayoff(a_player, v);
		}

		bool synced = false;
		a_player->GetGraphVariableBool("bIsSynced", synced);
		const bool pairOn = synced || a_player->IsInKillMove() || (v && v->IsInKillMove());
		switch (phase) {
		case Phase::kStarting:
			if (pairOn) {
				phase = Phase::kRunning;
				Log("pair started after {:.2f} s after {} tries (synced={} playerKillMove={} victimKillMove={})", t, tries, synced,
					a_player->IsInKillMove(), v && v->IsInKillMove());
			} else if (now - phaseStart >= kStartWindow) {
				Log("WARN idle {:08X} was accepted but no pair started within 1 s; victim: {}", playing->GetFormID(), DescribeVictim(v));
				if (!warnedNoStart) {
					warnedNoStart = true;
					Util::Notify("CIGAR: 유술 모션 미발동. 로그 확인");
				}
				Finish("no start");
			}
			break;
		case Phase::kRunning:
			// The victim's own kill-move flag stays set (its KillMoveEnd was swallowed); the player's side ends the pair.
			if (!synced && !a_player->IsInKillMove()) {
				if (!payoffDone) {
					Log("pair ended without a KillActor event; payoff applied at the end");
					ApplyPayoff(a_player, v);
				}
				Log("pair ended after {:.2f} s; victim: {}", t, DescribeVictim(v));
				EndKillMove(a_player, v);
				phase = Phase::kSettling;
				settleUntil = now + kSettle;
			} else if (now - phaseStart >= kPairTimeout) {
				Log("WARN pair still running after 10 s; victim: {}", DescribeVictim(v));
				Finish("timeout");
			}
			break;
		case Phase::kSettling:
			// The knock-down should show as ragdoll within a moment; say so when it does not.
			if (knocked && v && now >= settleUntil - kSettle + kKnockCheck) {
				knocked = false;
				Log("{:.1f} s after the knock-down: ragdoll={} knock={}", std::chrono::duration<float>(kKnockCheck).count(),
					v->IsInRagdollState(), static_cast<int>(v->AsActorState()->GetKnockState()));
				if (!v->IsInRagdollState() && !v->IsDead()) {
					Log("WARN the victim is not in ragdoll after the knock-down");
				}
			}
			if (now >= settleUntil) {
				Log("2 s after the pair: victim: {}", DescribeVictim(v));
				if (v && v->IsDead() && !warnedDied) {
					warnedDied = true;
					Log("WARN the victim died although KillActor and KillMoveEnd were swallowed");
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
		auto victimPtr = victim.get();
		if (auto* v = victimPtr.get()) {
			// A pair cut short (timeout, module off) must not leave the victim flagged as in a kill move.
			if (v->IsInKillMove() && phase != Phase::kPreparing) {
				v->GetActorRuntimeData().boolFlags.reset(RE::Actor::BOOL_FLAGS::kIsInKillMove);
				Log("cleared the victim's in-kill-move flag");
			}
			Log("finished ({}); victim: {}", a_reason, DescribeVictim(v));
		} else {
			Log("finished ({}); victim gone", a_reason);
		}
		knocked = false;
		phase = Phase::kIdle;
		victim = {};
	}

	void Jujutsu::OnDisabled()
	{
		if (phase != Phase::kIdle) {
			Finish("module switched off");
		}
	}
}
