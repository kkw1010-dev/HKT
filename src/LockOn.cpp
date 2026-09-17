#include "LockOn.h"

#include "Util.h"

#include "TDM/TrueDirectionalMovementAPI.h"

namespace CIGAR
{
	namespace
	{
		// TDM reads its MCM Helper settings over the shipped defaults; so does this.
		constexpr auto kTDMSettings = "Data/MCM/Settings/TrueDirectionalMovement.ini"sv;
		constexpr auto kTDMDefaults = "Data/MCM/Config/TrueDirectionalMovement/settings.ini"sv;
		constexpr std::int64_t kTDMDefaultLockKey = 258;  // middle mouse button

		constexpr auto kGrapplePlugin = "FH_Grapple.esp"sv;
		constexpr auto kGrappleScript = "FH_Grapple";
		constexpr RE::FormID kGrappleQuestID = 0x800;  // FHGrapple_Quest (MCM)

		// After a press, give the target mod time to act before offering again, so a lock that
		// finds no target does not re-offer at once.
		constexpr auto kQuietAfterPress = 3s;
		constexpr auto kCheckDelay = 1s;

		// Grapple's reach is not published; this is a close melee distance.
		constexpr float kGrappleReach = 350.0f;
		// Re-lock after a grapple that was started while locked.
		constexpr auto kGrappleStartWindow = 3s;
		constexpr auto kRelockSettle = 500ms;
		constexpr auto kRelockTimeout = 20s;

		class UpdateResult final : public RE::BSScript::IStackCallbackFunctor
		{
		public:
			void operator()(RE::BSScript::Variable) override
			{
				LockOn::GetSingleton()->Log("Grapple UpdateGlobals returned");
			}

			void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}
		};
	}

	LockOn* LockOn::GetSingleton()
	{
		static LockOn singleton;
		return &singleton;
	}

	void LockOn::ResolveGrapple()
	{
		grappleQuest = nullptr;
		grappleKey = -1;
		grappleModifier = false;

		auto* handler = RE::TESDataHandler::GetSingleton();
		if (!handler || !handler->LookupModByName(kGrapplePlugin)) {
			Log("Grapple not found; the grapple prompt is off");
			return;
		}
		const bool native = GetModuleHandleW(L"FH_Grapple_Plugin.dll") != nullptr;
		auto* quest = handler->LookupForm<RE::TESQuest>(kGrappleQuestID, kGrapplePlugin);
		const auto script = Util::ScriptObject(quest, kGrappleScript);
		grappleKey = Util::ScriptInt(script, "Hotkey");
		grappleModifier = Util::ScriptBool(script, "ModifierEnabled");
		Log("Grapple quest={} script={} dll={} key={} modifier={} lockKey={}",
			quest ? std::format("{:08X}", quest->GetFormID()) : "-", static_cast<bool>(script), native,
			grappleKey, grappleModifier, Util::ScriptInt(script, "TargetLockKey"));
		if (!script || !native) {
			Log("WARN Grapple is installed but its MCM script or DLL is missing; the grapple prompt is off");
			return;
		}
		grappleQuest = quest;
		if (grappleModifier) {
			Log("Grapple uses a modifier key, which a single press cannot hold; the grapple prompt is off");
		} else if (grappleKey < 0) {
			Log("Grapple has no keyboard hotkey; the grapple prompt is off");
		}
	}

	void LockOn::SyncGrappleLockKey()
	{
		// Grapple presses this key to release TDM's lock during a grapple; it must be TDM's key.
		if (!tdm || !grappleQuest || tdmLockKey < 0) {
			return;
		}
		const auto script = Util::ScriptObject(grappleQuest, kGrappleScript);
		auto* var = script ? script->GetProperty("TargetLockKey") : nullptr;
		if (!var || !var->IsInt()) {
			Log("WARN Grapple TargetLockKey property not readable; lock key not synced");
			return;
		}
		if (var->GetSInt() == tdmLockKey) {
			return;
		}
		Log("Grapple TargetLockKey {} differs from TDM's {}; syncing", var->GetSInt(), tdmLockKey);
		var->SetSInt(static_cast<std::int32_t>(tdmLockKey));
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* policy = vm->GetObjectHandlePolicy();
		const auto handle = policy->GetHandleForObject(grappleQuest->GetFormType(), grappleQuest);
		auto* args = RE::MakeFunctionArguments();
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback{ new UpdateResult() };
		const bool queued = vm->DispatchMethodCall2(handle, kGrappleScript, "UpdateGlobals", args, callback);
		Log("Grapple UpdateGlobals requested queued={}", queued);
	}

	void LockOn::OnGameLoaded()
	{
		lock.Reset();
		grapple.Reset();
		lastGate.clear();
		quietUntil = {};
		checkWhat = nullptr;
		relockPending = false;

		if (!tdm) {
			tdm = TDM_API::RequestPluginAPI();
		}
		if (!tdm) {
			Log("True Directional Movement not found; lock-on and grapple prompts are off");
			return;
		}

		std::string_view source = kTDMSettings;
		auto key = Util::IniInt(std::filesystem::path{ kTDMSettings }, "Keys", "uTargetLockKey");
		if (!key) {
			source = kTDMDefaults;
			key = Util::IniInt(std::filesystem::path{ kTDMDefaults }, "Keys", "uTargetLockKey");
		}
		if (!key) {
			source = "TDM default";
			key = kTDMDefaultLockKey;
		}
		tdmLockKey = *key;
		const bool pressable = tdmLockKey >= 0 && tdmLockKey < 264;
		Log("TDM api ok, lock key {} from {} pressable={}", tdmLockKey, source, pressable);
		if (!pressable && !warnedKey) {
			warnedKey = true;
			Log("WARN TDM lock key {} is not a keyboard or mouse key; the lock-on prompt is off", tdmLockKey);
			Util::Notify("CIGAR: TDM 록온 키가 키보드/마우스 키가 아님. 록온 프롬프트 비활성");
		}

		ResolveGrapple();
		SyncGrappleLockKey();
	}

	bool LockOn::InGrapple(RE::PlayerCharacter* a_player, bool a_movable)
	{
		// Grapple plays synchronized (paired) animations and takes the controls meanwhile.
		bool synced = false;
		a_player->GetGraphVariableBool("bIsSynced", synced);
		return synced || !a_movable;
	}

	void LockOn::UpdateRelock(RE::PlayerCharacter* a_player, bool a_combat, bool a_locked, bool a_movable)
	{
		// Grapple releases TDM's lock for its wind-up and does not lock again afterwards.
		const auto now = Clock::now();
		if (now >= relockDeadline) {
			relockPending = false;
			Log("re-lock given up: grapple did not end in time");
			return;
		}
		const bool busy = InGrapple(a_player, a_movable);
		if (busy) {
			sawGrapple = true;
			relockStable = {};
			return;
		}
		// No grapple animation within a few seconds: it missed or was on cooldown.
		if (!sawGrapple && now - relockStart < kGrappleStartWindow) {
			return;
		}
		if (a_locked || !a_combat) {
			relockPending = false;
			Log("re-lock not needed (locked={} combat={})", a_locked, a_combat);
			return;
		}
		if (relockStable == Clock::time_point{}) {
			relockStable = now;
		}
		if (now - relockStable < kRelockSettle) {
			return;
		}
		relockPending = false;
		const bool pressed = Util::PressKey(tdmLockKey);
		Log("re-lock after grapple (grapple seen={}) key {} pressed={}", sawGrapple, tdmLockKey, pressed);
		checkAt = now + kCheckDelay;
		checkWhat = "re-lock";
	}

	void LockOn::FastTick()
	{
		if (!tdm) {
			return;
		}
		auto* player = Util::Player();
		const bool combat = player->IsInCombat();
		const bool locked = tdm->GetTargetLockState();
		const auto* controls = RE::ControlMap::GetSingleton();
		const bool movable = controls && controls->IsMovementControlsEnabled();
		const auto now = Clock::now();
		const bool quiet = now < quietUntil;

		if (checkWhat && now >= checkAt) {
			Log("after {} press: locked={}", checkWhat, locked);
			checkWhat = nullptr;
		}
		if (relockPending) {
			UpdateRelock(player, combat, locked, movable);
		}

		const bool lockKeyOk = tdmLockKey >= 0 && tdmLockKey < 264;
		const bool grappleOk = grappleQuest && !grappleModifier && grappleKey >= 0 && grappleKey < 264;
		// Grapple picks its own target in front of the player; a lock is not required.
		const bool hostileNear = combat && grappleOk && !Util::NearbyHostiles(player, kGrappleReach).empty();
		LogGate(std::format("combat={} locked={} movable={} quiet={} relock={} grapple={} near={}",
			combat, locked, movable, quiet, relockPending, grappleOk, hostileNear));

		const bool ready = combat && movable && !quiet && !relockPending;
		lock.Update(ready && !locked && lockKeyOk, [] { return "록온"s; });
		grapple.Update(ready && grappleOk && (locked || hostileNear), [] { return "그래플"s; });
	}

	void LockOn::OnAccepted(std::uint16_t a_eventID)
	{
		if (!tdm || (a_eventID != kLock && a_eventID != kGrapple)) {
			return;
		}
		const bool isLock = a_eventID == kLock;
		const bool locked = tdm->GetTargetLockState();
		// Re-check: pressing TDM's key while locked would unlock instead.
		if (isLock && locked) {
			Log("accept ignored: lock requested while already locked");
			return;
		}
		const auto key = isLock ? tdmLockKey : static_cast<std::int64_t>(grappleKey);
		const bool pressed = Util::PressKey(key);
		Log("{} key {} pressed={} (locked={})", isLock ? "lock" : "grapple", key, pressed, locked);
		const auto now = Clock::now();
		quietUntil = now + kQuietAfterPress;
		checkAt = now + kCheckDelay;
		checkWhat = isLock ? "lock" : "grapple";
		if (!isLock && locked && pressed) {
			relockPending = true;
			sawGrapple = false;
			relockStart = now;
			relockStable = {};
			relockDeadline = now + kRelockTimeout;
		}
	}
}
