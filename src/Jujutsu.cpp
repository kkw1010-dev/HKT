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
		// Tests 5-7: in 14 of 15 refused attempts the PLAYER's graph had IsAttacking set on every retry; the
		// victim's state varied. The engine will not start a paired idle on an actor mid-attack, so an attacking
		// player gets attackStop (the victim, blockStop) before each try.
		// Test 8: 19 of 20 plays started on the first try, right at the press; retrying for 1.5 s saved one
		// and cut the player's next attacks short, which the user saw as an awkward stop. So retries (and the
		// attackStop that comes with them) last only this long: the swing in progress at the press is cut,
		// and the next one is left alone.
		constexpr auto kPrepareWindow = 300ms;

		bool GraphBool(RE::Actor* a_actor, const char* a_name)
		{
			bool value = false;
			return a_actor && a_actor->GetGraphVariableBool(a_name, value) && value;
		}

		bool IsDodging(RE::Actor* a_actor)
		{
			return GraphBool(a_actor, "bIsDodging");
		}
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
				// This is the event that kills a kill-move victim. Test 3: knocking it down when the player's
				// side ended left it standing up for 0.5-2.5 s first, so it goes down now, on the next frame.
				Stamp(killMoveEndAt);
				SKSE::GetTaskInterface()->AddTask([] { Jujutsu::GetSingleton()->OnVictimKillMoveEnd(); });
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
			// A target Valhalla lets you execute gets the 처형 prompt, not this one (the user's rule).
			if (valhalla && valhalla->isActorStunned(actor)) {
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
		ended = false;
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
		// Test 4 logged the state after the call, which an accepted play has already changed; the state that
		// decides is the one before it (and before blockStop / attackStop).
		const auto before = DescribeRefusal(a_player, a_victim);
		if (wasBlocking) {
			a_victim->NotifyAnimationGraph("blockStop");
		}
		const bool playerAttacking = GraphBool(a_player, "IsAttacking");
		if (playerAttacking) {
			a_player->NotifyAnimationGraph("attackStop");
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
		Log("try {}: before {}{}{} -> SetupSpecialIdle returned {}", tries, before, wasBlocking ? " (sent blockStop)" : "",
			playerAttacking ? " (sent attackStop to the player)" : "", requested);
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
			// NPC Block Loop Fix (OAR) replaces the block idle only while the actor moves, and fires blockStop
			// every second from it; speed and the graph's own block/attack flags show whether that is in play.
			float speed = 0.0f;
			a_actor->GetGraphVariableFloat("Speed", speed);
			bool graphBlocking = false;
			a_actor->GetGraphVariableBool("IsBlocking", graphBlocking);
			bool graphAttacking = false;
			a_actor->GetGraphVariableBool("IsAttacking", graphAttacking);
			// TK Dodge RE's graph variables (its Nemesis patch adds them to 1hm_behavior and magicbehavior).
			bool iframe = false;
			a_actor->GetGraphVariableBool("bInIframe", iframe);
			return std::format(
				"attack={} knock={} stagger={} synced={} killmove={} sprint={} ragdoll={} speed={:.0f} gBlock={} gAttack={} dodge={} iframe={}",
				s ? static_cast<int>(s->GetAttackState()) : -1, s ? static_cast<int>(s->GetKnockState()) : -1, staggered, synced,
				a_actor->IsInKillMove(), s && s->IsSprinting(), a_actor->IsInRagdollState(), speed, graphBlocking, graphAttacking,
				IsDodging(a_actor), iframe);
		};
		// Test 3: every refusal was one bandit, at 66-153 units, while others were taken at 100-193. So also
		// the height difference, which way each faces the other, the race and the victim's weapon.
		const auto* race = a_victim->GetRace();
		const auto* weapon = a_victim->GetEquippedObject(false);
		// 2026-09-19: every H2H kill move was refused after Private Needs was installed, while Valhalla's
		// execution still played on the same victim. The player's side is logged in full to tell apart
		// what it holds, who it is, which controls are locked and what is active on it.
		const auto* right = a_player->GetEquippedObject(false);
		const auto* left = a_player->GetEquippedObject(true);
		const auto* playerRace = a_player->GetRace();
		const auto* base = a_player->GetActorBase();
		const auto* controls = RE::ControlMap::GetSingleton();
		std::int32_t pnoIdx = 0;
		a_player->GetGraphVariableInt("PNO_Animation_Idx", pnoIdx);
		bool animDriven = false;
		a_player->GetGraphVariableBool("bAnimationDriven", animDriven);
		std::string effects;
		std::size_t effectCount = 0;
		if (auto* list = a_player->AsMagicTarget()->GetActiveEffectList()) {
			for (auto* effect : *list) {
				const auto* mgef = effect ? effect->GetBaseObject() : nullptr;
				if (!mgef || effect->flags.any(RE::ActiveEffect::Flag::kInactive)) {
					continue;
				}
				++effectCount;
				const auto* file = mgef->GetFile(0);
				const std::string_view plugin = file ? file->GetFilename() : ""sv;
				// Name the effects that are not from the base game, which is where a new cause would come from.
				if (plugin != "Skyrim.esm"sv && plugin != "Update.esm"sv && plugin != "Dawnguard.esm"sv && plugin != "Dragonborn.esm"sv) {
					const std::string_view edid = mgef->GetFormEditorID();
					effects += std::format("{}{}({})", effects.empty() ? "" : ",",
						edid.empty() ? std::format("{:08X}", mgef->GetFormID()) : std::string(edid), plugin);
				}
			}
		}
		return std::format("victim[blocking={} {} race={} weapon={} facesPlayer={:.0f}] player[weaponDrawn={} {} facesVictim={:.0f} "
		                   "right={} left={} race={} female={} move={} fight={} activate={} pnoIdx={} animDriven={} effects={}:{}] "
		                   "distance={:.0f} dz={:.0f}",
			a_victim->IsBlocking(), state(a_victim), race ? Util::NameOf(race) : "-"s, weapon ? Util::NameOf(weapon) : "-"s,
			a_victim->GetHeadingAngle(a_player->GetPosition(), false), a_player->AsActorState()->IsWeaponDrawn(), state(a_player),
			a_player->GetHeadingAngle(a_victim->GetPosition(), false), right ? Util::NameOf(right) : "-"s, left ? Util::NameOf(left) : "-"s,
			playerRace ? Util::NameOf(playerRace) : "-"s, base && base->IsFemale(), controls && controls->IsMovementControlsEnabled(),
			controls && controls->IsFightingControlsEnabled(), controls && controls->IsActivateControlsEnabled(), pnoIdx, animDriven,
			effectCount, effects.empty() ? "-"s : effects, a_player->GetPosition().GetDistance(a_victim->GetPosition()),
			a_victim->GetPositionZ() - a_player->GetPositionZ());
	}

	void Jujutsu::OnVictimKillMoveEnd()
	{
		if (phase != Phase::kRunning && phase != Phase::kStarting) {
			return;
		}
		auto victimPtr = victim.get();
		auto* v = victimPtr.get();
		auto* player = Util::Player();
		if (!payoffDone) {
			ApplyPayoff(player, v);
		}
		Log("victim KillMoveEnd at {:.2f} s: knocking it down now", Elapsed());
		EndKillMove(player, v);
	}

	void Jujutsu::EndKillMove(RE::PlayerCharacter* a_player, RE::Actor* a_victim)
	{
		if (ended || !a_victim || a_victim->IsDead()) {
			return;
		}
		ended = true;
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
		knockAt = Clock::now();
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

		// The knock-down should show as ragdoll within a moment; say so when it does not.
		if (knocked && v && now >= knockAt + kKnockCheck) {
			knocked = false;
			Log("{:.1f} s after the knock-down: ragdoll={} knock={}", std::chrono::duration<float>(kKnockCheck).count(), v->IsInRagdollState(),
				static_cast<int>(v->AsActorState()->GetKnockState()));
			if (!v->IsInRagdollState() && !v->IsDead()) {
				Log("WARN the victim is not in ragdoll after the knock-down");
			}
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
