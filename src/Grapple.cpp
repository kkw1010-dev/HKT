#include "Grapple.h"

#include "Settings.h"
#include "TDMLock.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		constexpr auto kGrapplePlugin = "FH_Grapple.esp"sv;
		constexpr auto kGrappleScript = "FH_Grapple";
		constexpr RE::FormID kGrappleQuestID = 0x800;  // FHGrapple_Quest (MCM)
		// FH_Grapple_Plugin.dll restores its keys from here at startup and saves them on every
		// FH_Grapple_UpdateKeys call.
		constexpr auto kGrappleIni = "Data/SKSE/Plugins/FH_Grapple_Plugin.ini"sv;
		// Prompt-only mode moves Grapple's hotkey here: F13, which ordinary keyboards never send.
		// Grapple's input sink compares the keyboard scan code with its key and nothing else, and its
		// own QTE prompts do not use the hotkey (FH_Grapple_Plugin.dll 1.2.0, disassembled).
		constexpr std::int32_t kHiddenKey = 0x64;
		constexpr auto kPromptOnlyTarget = "grapple"sv;

		// After a press, give Grapple time to act before offering again.
		constexpr auto kQuietAfterPress = 3s;
		constexpr auto kCheckDelay = 1s;

		// Grapple's reach is not published; this is a close melee distance.
		constexpr float kReach = 350.0f;
		// Re-lock after a grapple that was started while locked.
		constexpr auto kGrappleStartWindow = 3s;
		constexpr auto kRelockSettle = 500ms;
		constexpr auto kRelockTimeout = 20s;

		class UpdateResult final : public RE::BSScript::IStackCallbackFunctor
		{
		public:
			void operator()(RE::BSScript::Variable) override
			{
				Grapple::GetSingleton()->Log("Grapple ApplySettings returned");
			}

			void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}
		};
	}

	Grapple* Grapple::GetSingleton()
	{
		static Grapple singleton;
		return &singleton;
	}

	void Grapple::ReadIni()
	{
		const auto found = Util::IniInt(std::filesystem::path{ kGrappleIni }, "Keys", "kbKey");
		if (found && *found >= 0 && *found < 264 && *found != kHiddenKey) {
			knownKey = static_cast<std::int32_t>(*found);
		}
		Log("Grapple INI kbKey={} at startup", found ? std::to_string(*found) : "-"s);
	}

	void Grapple::Resolve()
	{
		quest = nullptr;
		key = -1;
		modifier = false;

		auto* handler = RE::TESDataHandler::GetSingleton();
		if (!handler || !handler->LookupModByName(kGrapplePlugin)) {
			Log("Grapple not found; this module idles");
			return;
		}
		const bool native = GetModuleHandleW(L"FH_Grapple_Plugin.dll") != nullptr;
		auto* found = handler->LookupForm<RE::TESQuest>(kGrappleQuestID, kGrapplePlugin);
		const auto script = Util::ScriptObject(found, kGrappleScript);
		key = Util::ScriptInt(script, "Hotkey");
		modifier = Util::ScriptBool(script, "ModifierEnabled");
		Log("Grapple quest={} script={} dll={} key={} modifier={} lockKey={} knownKey={}",
			found ? std::format("{:08X}", found->GetFormID()) : "-", static_cast<bool>(script), native,
			key, modifier, Util::ScriptInt(script, "TargetLockKey"), knownKey);
		if (!script || !native) {
			Log("WARN Grapple is installed but its MCM script or DLL is missing; the grapple prompt is off");
			return;
		}
		quest = found;
	}

	void Grapple::SyncKeys()
	{
		if (!quest) {
			return;
		}
		const auto script = Util::ScriptObject(quest, kGrappleScript);
		auto* hotkey = script ? script->GetProperty("Hotkey") : nullptr;
		auto* lockKey = script ? script->GetProperty("TargetLockKey") : nullptr;
		if (!hotkey || !hotkey->IsInt() || !lockKey || !lockKey->IsInt()) {
			Log("WARN Grapple Hotkey/TargetLockKey properties not readable; keys not synced");
			return;
		}
		bool changed = false;
		const auto current = hotkey->GetSInt();
		if (Settings::PromptOnly(kPromptOnlyTarget)) {
			if (current != kHiddenKey) {
				// Remember the player's key so switching prompt-only off gives it back.
				if (current >= 0 && current < 264) {
					Settings::SetManualKey(kPromptOnlyTarget, current);
				}
				Log("prompt-only: Grapple Hotkey {} -> hidden key {}", current, kHiddenKey);
				hotkey->SetSInt(kHiddenKey);
				changed = true;
			}
		} else if (current < 0 || current == kHiddenKey) {
			// A new game starts Grapple's MCM with no key (-1) and pushes that to its DLL, and prompt-only
			// mode leaves the hidden key behind. Restore the player's own key.
			const auto manual = Settings::ManualKey(kPromptOnlyTarget);
			const auto restore = manual >= 0 ? manual : knownKey;
			if (restore >= 0) {
				Log("Grapple Hotkey {}; restoring the player's key {}", current, restore);
				hotkey->SetSInt(restore);
				changed = true;
			} else if (current == kHiddenKey) {
				Log("WARN Grapple Hotkey is the hidden key and no earlier key is known; set one in Grapple's MCM");
				hotkey->SetSInt(-1);
				changed = true;
			}
		}
		// Grapple presses this key to release TDM's lock during a grapple; it must be TDM's key.
		// Without TDM there is no lock to release, so the key is left as it is.
		const auto tdmKey = TDMLock::Key();
		if (TDMLock::Api() && tdmKey >= 0 && lockKey->GetSInt() != tdmKey) {
			Log("Grapple TargetLockKey {} differs from TDM's {}; syncing", lockKey->GetSInt(), tdmKey);
			lockKey->SetSInt(static_cast<std::int32_t>(tdmKey));
			changed = true;
		}
		if (!changed) {
			return;
		}
		// ApplySettings registers the hotkey and calls UpdateGlobals, which pushes the keys to the DLL.
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* policy = vm->GetObjectHandlePolicy();
		const auto handle = policy->GetHandleForObject(quest->GetFormType(), quest);
		auto* args = RE::MakeFunctionArguments();
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback{ new UpdateResult() };
		const bool queued = vm->DispatchMethodCall2(handle, kGrappleScript, "ApplySettings", args, callback);
		Log("Grapple ApplySettings requested queued={}", queued);
	}

	void Grapple::RefreshKey()
	{
		// Runs at load and on the control panel's key check; an MCM change in between is not seen.
		const auto script = quest ? Util::ScriptObject(quest, kGrappleScript) : nullptr;
		const auto found = script ? Util::ScriptInt(script, "Hotkey") : -1;
		const bool hasModifier = script && Util::ScriptBool(script, "ModifierEnabled");
		if (found != key || hasModifier != modifier) {
			Log("Grapple key changed: {} -> {}, modifier {} -> {}", key, found, modifier, hasModifier);
			key = found;
			modifier = hasModifier;
		}
		if (key >= 0 && key < 264 && key != kHiddenKey) {
			knownKey = key;
		}
		const bool ok = quest && !modifier && key >= 0 && key < 264;
		if (quest && !ok && keyOk.load()) {
			Log("grapple prompt off: {}", modifier ? "a modifier key is enabled, which a single press cannot hold" : "no keyboard hotkey");
		}
		keyOk = ok;
		keyShown = key;
	}

	void Grapple::CheckKeys()
	{
		if (!quest) {
			Log("key check: Grapple not loaded");
			return;
		}
		// Applies the prompt-only switch, and moves a key set in the MCM back to the hidden key
		// while prompt-only is on (that key is remembered).
		SyncKeys();
		RefreshKey();
		Log("key check: Grapple key {} (usable={})", key, keyOk.load());
		if (!keyOk.load() && !warnedKey) {
			warnedKey = true;
			Log("WARN Grapple has no usable hotkey (key={} modifier={}); the grapple prompt is off", key, modifier);
			Util::Notify(Text::L("CIGAR: 그래플 키 미지정 또는 조합키. 그래플 프롬프트 비활성", "CIGAR: Grapple key unset or a key combination. Grapple prompt off"));
		}
	}

	void Grapple::OnGameLoaded()
	{
		grapple.Reset();
		lastGate.clear();
		quietUntil = {};
		checkPending = false;
		relockPending = false;
		quest = nullptr;
		keyOk = false;

		// LockOn resolves this too; both do, because either module may be switched off.
		TDMLock::Resolve();
		Log("TDM {} (lock key {} from {})", TDMLock::Api() ? "present" : "absent", TDMLock::Key(), TDMLock::KeySource());

		Resolve();
		CheckKeys();
	}

	void Grapple::OnDisabled()
	{
		if (relockPending) {
			relockPending = false;
			TDMLock::SetBusy(false);
		}
	}

	bool Grapple::InGrapple(RE::PlayerCharacter* a_player, bool a_movable)
	{
		// Grapple plays synchronized (paired) animations and takes the controls meanwhile.
		bool synced = false;
		a_player->GetGraphVariableBool("bIsSynced", synced);
		return synced || !a_movable;
	}

	void Grapple::UpdateRelock(RE::PlayerCharacter* a_player, bool a_combat, bool a_locked, bool a_movable)
	{
		// Grapple releases TDM's lock for its wind-up and does not lock again afterwards.
		const auto now = Clock::now();
		const auto drop = [this, a_locked, a_combat](std::string_view a_why) {
			relockPending = false;
			TDMLock::SetBusy(false);
			Log("re-lock dropped ({}; locked={} combat={})", a_why, a_locked, a_combat);
		};
		if (now >= relockDeadline) {
			drop("grapple did not end in time");
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
			drop("not needed");
			return;
		}
		if (relockStable == Clock::time_point{}) {
			relockStable = now;
		}
		if (now - relockStable < kRelockSettle) {
			return;
		}
		relockPending = false;
		TDMLock::SetBusy(false);
		const bool pressed = TDMLock::Press();
		Log("re-lock after grapple (grapple seen={}) key {} pressed={}", sawGrapple, TDMLock::Key(), pressed);
		checkAt = now + kCheckDelay;
		checkPending = true;
	}

	void Grapple::FastTick()
	{
		auto* player = Util::Player();
		const bool combat = player->IsInCombat();
		const bool locked = TDMLock::Locked();
		const auto* controls = RE::ControlMap::GetSingleton();
		const bool movable = controls && controls->IsMovementControlsEnabled();
		const auto now = Clock::now();
		const bool quiet = now < quietUntil;

		if (checkPending && now >= checkAt) {
			Log("after re-lock press: locked={}", locked);
			checkPending = false;
		}
		if (relockPending) {
			UpdateRelock(player, combat, locked, movable);
		}

		const bool usable = keyOk.load();
		// Grapple picks its own target in front of the player; a lock is not required.
		const bool hostileNear = combat && usable && !Util::NearbyHostiles(player, kReach).empty();
		LogGate(std::format("combat={} locked={} movable={} quiet={} relock={} key={} near={}",
			combat, locked, movable, quiet, relockPending, usable, hostileNear));

		const bool ready = combat && movable && !quiet && !relockPending;
		grapple.Update(ready && usable && (locked || hostileNear), [] { return std::string(Text::L("그래플", "Grapple")); });
	}

	void Grapple::OnAccepted(std::uint16_t a_eventID)
	{
		if (a_eventID != kGrapple || !keyOk.load()) {
			return;
		}
		const bool locked = TDMLock::Locked();
		const bool pressed = Util::PressKey(key);
		Log("grapple key {} pressed={} (locked={})", key, pressed, locked);
		const auto now = Clock::now();
		quietUntil = now + kQuietAfterPress;
		if (locked && pressed) {
			relockPending = true;
			sawGrapple = false;
			relockStart = now;
			relockStable = {};
			relockDeadline = now + kRelockTimeout;
			// Hold the lock-on prompt down until the re-lock is done, so both never press TDM's key
			// in the same frame.
			TDMLock::SetBusy(true);
		}
	}
}
