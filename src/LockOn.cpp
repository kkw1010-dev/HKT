#include "LockOn.h"

#include "Settings.h"
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
		// FH_Grapple_Plugin.dll restores its keys from here at startup and saves them on every
		// FH_Grapple_UpdateKeys call.
		constexpr auto kGrappleIni = "Data/SKSE/Plugins/FH_Grapple_Plugin.ini"sv;
		// Prompt-only mode moves Grapple's hotkey here: F13, which ordinary keyboards never send.
		// Grapple's input sink compares the keyboard scan code with its key and nothing else, and its
		// own QTE prompts do not use the hotkey (FH_Grapple_Plugin.dll 1.2.0, disassembled).
		constexpr std::int32_t kHiddenGrappleKey = 0x64;
		constexpr auto kPromptOnlyTarget = "grapple"sv;

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
				LockOn::GetSingleton()->Log("Grapple ApplySettings returned");
			}

			void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}
		};
	}

	LockOn* LockOn::GetSingleton()
	{
		static LockOn singleton;
		return &singleton;
	}

	void LockOn::ReadGrappleIni()
	{
		const auto key = Util::IniInt(std::filesystem::path{ kGrappleIni }, "Keys", "kbKey");
		if (key && *key >= 0 && *key < 264 && *key != kHiddenGrappleKey) {
			knownGrappleKey = static_cast<std::int32_t>(*key);
		}
		Log("Grapple INI kbKey={} at startup", key ? std::to_string(*key) : "-"s);
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
		Log("Grapple quest={} script={} dll={} key={} modifier={} lockKey={} knownKey={}",
			quest ? std::format("{:08X}", quest->GetFormID()) : "-", static_cast<bool>(script), native,
			grappleKey, grappleModifier, Util::ScriptInt(script, "TargetLockKey"), knownGrappleKey);
		if (!script || !native) {
			Log("WARN Grapple is installed but its MCM script or DLL is missing; the grapple prompt is off");
			return;
		}
		grappleQuest = quest;
	}

	void LockOn::SyncGrappleKeys()
	{
		if (!grappleQuest) {
			return;
		}
		const auto script = Util::ScriptObject(grappleQuest, kGrappleScript);
		auto* hotkey = script ? script->GetProperty("Hotkey") : nullptr;
		auto* lockKey = script ? script->GetProperty("TargetLockKey") : nullptr;
		if (!hotkey || !hotkey->IsInt() || !lockKey || !lockKey->IsInt()) {
			Log("WARN Grapple Hotkey/TargetLockKey properties not readable; keys not synced");
			return;
		}
		bool changed = false;
		const auto current = hotkey->GetSInt();
		if (Settings::PromptOnly(kPromptOnlyTarget)) {
			if (current != kHiddenGrappleKey) {
				// Remember the player's key so switching prompt-only off gives it back.
				if (current >= 0 && current < 264) {
					Settings::SetManualKey(kPromptOnlyTarget, current);
				}
				Log("prompt-only: Grapple Hotkey {} -> hidden key {}", current, kHiddenGrappleKey);
				hotkey->SetSInt(kHiddenGrappleKey);
				changed = true;
			}
		} else if (current < 0 || current == kHiddenGrappleKey) {
			// A new game starts Grapple's MCM with no key (-1) and pushes that to its DLL, and prompt-only
			// mode leaves the hidden key behind. Restore the player's own key.
			const auto manual = Settings::ManualKey(kPromptOnlyTarget);
			const auto restore = manual >= 0 ? manual : knownGrappleKey;
			if (restore >= 0) {
				Log("Grapple Hotkey {}; restoring the player's key {}", current, restore);
				hotkey->SetSInt(restore);
				changed = true;
			} else if (current == kHiddenGrappleKey) {
				Log("WARN Grapple Hotkey is the hidden key and no earlier key is known; set one in Grapple's MCM");
				hotkey->SetSInt(-1);
				changed = true;
			}
		}
		// Grapple presses this key to release TDM's lock during a grapple; it must be TDM's key.
		if (tdm && tdmLockKey >= 0 && lockKey->GetSInt() != tdmLockKey) {
			Log("Grapple TargetLockKey {} differs from TDM's {}; syncing", lockKey->GetSInt(), tdmLockKey);
			lockKey->SetSInt(static_cast<std::int32_t>(tdmLockKey));
			changed = true;
		}
		if (!changed) {
			return;
		}
		// ApplySettings registers the hotkey and calls UpdateGlobals, which pushes the keys to the DLL.
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* policy = vm->GetObjectHandlePolicy();
		const auto handle = policy->GetHandleForObject(grappleQuest->GetFormType(), grappleQuest);
		auto* args = RE::MakeFunctionArguments();
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback{ new UpdateResult() };
		const bool queued = vm->DispatchMethodCall2(handle, kGrappleScript, "ApplySettings", args, callback);
		Log("Grapple ApplySettings requested queued={}", queued);
	}

	void LockOn::RefreshGrappleKey()
	{
		// Runs at load and on the control panel's key check; an MCM change in between is not seen.
		const auto script = grappleQuest ? Util::ScriptObject(grappleQuest, kGrappleScript) : nullptr;
		const auto key = script ? Util::ScriptInt(script, "Hotkey") : -1;
		const bool modifier = script && Util::ScriptBool(script, "ModifierEnabled");
		if (key != grappleKey || modifier != grappleModifier) {
			Log("Grapple key changed: {} -> {}, modifier {} -> {}", grappleKey, key, grappleModifier, modifier);
			grappleKey = key;
			grappleModifier = modifier;
		}
		if (grappleKey >= 0 && grappleKey < 264 && grappleKey != kHiddenGrappleKey) {
			knownGrappleKey = grappleKey;
		}
		const bool ok = grappleQuest && !grappleModifier && grappleKey >= 0 && grappleKey < 264;
		if (grappleQuest && !ok && grappleOk.load()) {
			Log("grapple prompt off: {}", grappleModifier ? "a modifier key is enabled, which a single press cannot hold" : "no keyboard hotkey");
		}
		grappleOk = ok;
		grappleKeyShown = grappleKey;
	}

	void LockOn::CheckKeys()
	{
		if (!grappleQuest) {
			Log("key check: Grapple not loaded");
			return;
		}
		// Applies the prompt-only switch, and moves a key set in the MCM back to the hidden key
		// while prompt-only is on (that key is remembered).
		SyncGrappleKeys();
		RefreshGrappleKey();
		Log("key check: Grapple key {} (usable={})", grappleKey, grappleOk.load());
		if (!grappleOk.load() && !warnedGrappleKey) {
			warnedGrappleKey = true;
			Log("WARN Grapple has no usable hotkey (key={} modifier={}); the grapple prompt is off", grappleKey, grappleModifier);
			Util::Notify("CIGAR: 그래플 키 미지정 또는 조합키. 그래플 프롬프트 비활성");
		}
	}

	void LockOn::OnGameLoaded()
	{
		lock.Reset();
		grapple.Reset();
		lastGate.clear();
		quietUntil = {};
		checkWhat = nullptr;
		relockPending = false;
		grappleQuest = nullptr;
		grappleOk = false;

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
		CheckKeys();
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
		const bool grappleUsable = grappleOk.load();
		// Grapple picks its own target in front of the player; a lock is not required.
		const bool hostileNear = combat && grappleUsable && !Util::NearbyHostiles(player, kGrappleReach).empty();
		LogGate(std::format("combat={} locked={} movable={} quiet={} relock={} grapple={} near={}",
			combat, locked, movable, quiet, relockPending, grappleUsable, hostileNear));

		const bool ready = combat && movable && !quiet && !relockPending;
		lock.Update(ready && !locked && lockKeyOk, [] { return "록온"s; });
		grapple.Update(ready && grappleUsable && (locked || hostileNear), [] { return "그래플"s; });
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
