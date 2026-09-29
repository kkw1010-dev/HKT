#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// 마검사 모드 / 마검사 해제 (the user, 2026-09-27): drawing a weapon while The Wizard Warrior
	// (Nexus 14890) is off offers to turn it on; sheathing it out of combat while it is on offers to
	// turn it off. Both call QK_MainQuestScript.ToggleAbility(), the function its X key runs. WW's
	// keys and MCM stay the player's; CIGAR does not move, unbind or report them. Absent WW, the
	// module stays silent. docs/037.
	class WizardWarrior final : public Module
	{
	public:
		static WizardWarrior* GetSingleton();

		const char* Name() const override { return "WizardWarrior"; }
		void OnGameLoaded() override;
		void Tick() override {}
		void FastTick() override;
		void OnAccepted(std::uint16_t a_eventID) override;
		void OnDeclined(std::uint16_t a_eventID) override;

	private:
		// 마검사 해제 is shown out of combat only, so it fills a ring (the user's rule for non-combat
		// actions, 2026-09-29); 마검사 모드 is raised with the weapon drawn, in combat, and stays a press.
		WizardWarrior() { deactivate.SetPromptType(SkyPromptAPI::kHold); }

		enum : std::uint16_t
		{
			kActivate = PromptID::kWizardWarrior,
			kDeactivate = PromptID::kWizardWarriorOff
		};

		bool IsOn() const;
		void Toggle(bool a_turnOn);

		PromptSlot activate{ this, kActivate };
		PromptSlot deactivate{ this, kDeactivate };

		RE::TESQuest* quest{ nullptr };
		RE::BSTSmartPointer<RE::BSScript::Object> script;  // QK_MainQuestScript on the quest
		RE::TESGlobal* toggle{ nullptr };  // QK_SpellToggle, WW's PowerToggle: 1 while on
		// Declines (SkyPrompt's double tap): 마검사 모드 hides until the weapon is sheathed,
		// 마검사 해제 until the next draw and sheathe.
		bool dismissedOn{ false };
		bool dismissedOff{ false };
		std::chrono::steady_clock::time_point quietUntil{};
		std::chrono::steady_clock::time_point checkAt{};
		bool checking{ false };
		bool expectOn{ false };  // the state the last accept asked for
	};
}
