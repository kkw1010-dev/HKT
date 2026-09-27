#include "WizardWarrior.h"

#include "Util.h"

namespace CIGAR
{
	namespace
	{
		using Clock = std::chrono::steady_clock;

		// The Wizard Warrior 5.0.1 (read from its sources and ESP, docs/037): QK_QuestMain carries
		// QK_MainQuestScript; its OnKeyDown runs ToggleAbility() while Allow_Switch is true, and
		// ToggleAbility() sets the global QK_SpellToggle (the script's PowerToggle) to 1 or 0.
		constexpr auto kPlugin = "The Wizard Warrior.esp"sv;
		constexpr RE::FormID kQuestID = 0x878;   // QK_QuestMain
		constexpr RE::FormID kToggleID = 0x87E;  // QK_SpellToggle
		constexpr auto kScript = "QK_MainQuestScript";
		constexpr auto kToggleFunction = "ToggleAbility";
		// False while WW casts a concentration spell through block; X then re-casts it instead.
		constexpr auto kAllowSwitch = "Allow_Switch";

		// ToggleAbility runs on the Papyrus VM; the prompt waits this long before it may show again,
		// and the result is checked after it.
		constexpr auto kQuietAfterAccept = 2s;
		constexpr auto kCheckAfterAccept = 2s;

		class NoResult final : public RE::BSScript::IStackCallbackFunctor
		{
		public:
			void operator()(RE::BSScript::Variable) override {}
			void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}
		};
	}

	WizardWarrior* WizardWarrior::GetSingleton()
	{
		static WizardWarrior singleton;
		return &singleton;
	}

	void WizardWarrior::OnGameLoaded()
	{
		activate.Reset();
		deactivate.Reset();
		lastGate.clear();
		dismissedOn = false;
		dismissedOff = false;
		checking = false;
		auto* handler = RE::TESDataHandler::GetSingleton();
		quest = handler && handler->LookupModByName(kPlugin) ? handler->LookupForm<RE::TESQuest>(kQuestID, kPlugin) : nullptr;
		toggle = quest ? handler->LookupForm<RE::TESGlobal>(kToggleID, kPlugin) : nullptr;
		script = nullptr;
		if (quest) {
			script = Util::ScriptObject(quest, kScript);
		}
		if (!quest) {
			Log("The Wizard Warrior not found: 마검사 모드 is off");
			return;
		}
		if (!toggle || !script) {
			Log("WARN The Wizard Warrior found but {} is missing: 마검사 모드 is off", !toggle ? "QK_SpellToggle" : kScript);
			quest = nullptr;
			script = nullptr;
			return;
		}
		Log("ready: quest {:08X}, toggle global {:08X} = {:.0f}{}", quest->GetFormID(), toggle->GetFormID(), toggle->value, Util::DescribeScenes());
	}

	bool WizardWarrior::IsOn() const
	{
		return toggle && toggle->value >= 0.5f;
	}

	void WizardWarrior::FastTick()
	{
		auto* player = Util::Player();
		if (!quest || !player) {
			return;
		}
		const auto now = Clock::now();
		if (checking && now >= checkAt) {
			checking = false;
			if (IsOn() == expectOn) {
				Log("{}: QK_SpellToggle = {}", expectOn ? "on" : "off", expectOn ? 1 : 0);
			} else if (expectOn) {
				Log("WARN ToggleAbility ran but QK_SpellToggle is still 0");
				Util::Notify(Text::L("CIGAR: 마검사 모드가 켜지지 않음. 로그 확인", "CIGAR: Wizard Warrior did not turn on. See the log"));
			} else {
				Log("WARN ToggleAbility ran but QK_SpellToggle is still 1");
				Util::Notify(Text::L("CIGAR: 마검사 모드가 꺼지지 않음. 로그 확인", "CIGAR: Wizard Warrior did not turn off. See the log"));
			}
		}

		const auto* state = player->AsActorState();
		const bool drawn = state && state->IsWeaponDrawn();
		if (drawn) {
			dismissedOff = false;  // 마검사 해제: hidden until the next draw and sheathe
		} else {
			dismissedOn = false;  // 마검사 모드: hidden until the weapon is sheathed
		}

		const auto* allowVar = script ? script->GetVariable(kAllowSwitch) : nullptr;
		const bool allowSwitch = !allowVar || !allowVar->IsBool() || allowVar->GetBool();
		const bool on = IsOn();
		const bool combat = player->IsInCombat();
		const bool scene = Util::InScene(player);
		const bool quiet = now < quietUntil;
		LogGate(std::format("drawn={} on={} allowSwitch={} combat={} scene={} dismissedOn={} dismissedOff={} quiet={}", drawn, on, allowSwitch,
			combat, scene, dismissedOn, dismissedOff, quiet));
		const bool free = allowSwitch && !scene && !quiet;
		activate.Update(free && drawn && !on && !dismissedOn, [] { return Text::L("마검사 모드", "Wizard Warrior Mode"); });
		// Off only out of combat: sheathing mid-fight can be a stance change (the user, 2026-09-27).
		deactivate.Update(free && !drawn && on && !combat && !dismissedOff, [] { return Text::L("마검사 해제", "End Wizard Warrior"); });
	}

	void WizardWarrior::Toggle(bool a_turnOn)
	{
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto* args = RE::MakeFunctionArguments();
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback{ new NoResult() };
		const bool queued = vm && vm->DispatchMethodCall2(Util::Handle(quest), kScript, kToggleFunction, args, callback);
		quietUntil = Clock::now() + kQuietAfterAccept;
		checking = queued;
		expectOn = a_turnOn;
		checkAt = Clock::now() + kCheckAfterAccept;
		Log("{}ToggleAbility ({}) queued={}", queued ? "" : "WARN ", a_turnOn ? "turn on" : "turn off", queued);
		if (!queued) {
			Util::Notify(Text::L("CIGAR: 마검사 모드 호출 실패. 로그 확인", "CIGAR: Could not call Wizard Warrior. See the log"));
		}
	}

	void WizardWarrior::OnAccepted(std::uint16_t a_eventID)
	{
		if (!quest || (a_eventID != kActivate && a_eventID != kDeactivate)) {
			return;
		}
		const bool turnOn = a_eventID == kActivate;
		// ToggleAbility flips whatever state WW is in, so it runs only when that state still matches.
		if (IsOn() == turnOn) {
			Log("accept ignored: already {}", turnOn ? "on" : "off");
			return;
		}
		(turnOn ? activate : deactivate).Withdraw();
		Toggle(turnOn);
	}

	void WizardWarrior::OnDeclined(std::uint16_t a_eventID)
	{
		if (a_eventID == kActivate) {
			dismissedOn = true;
			activate.Withdraw();
			Log("마검사 모드 dismissed: hidden until the weapon is sheathed");
		} else if (a_eventID == kDeactivate) {
			dismissedOff = true;
			deactivate.Withdraw();
			Log("마검사 해제 dismissed: hidden until the weapon is drawn and sheathed again");
		}
	}
}
