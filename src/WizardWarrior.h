#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// 마검사 모드 (the user, 2026-09-27): drawing a weapon while The Wizard Warrior (Nexus 14890) is
	// off offers one prompt that turns it on, through the same QK_MainQuestScript.ToggleAbility()
	// its X key runs. Nothing turns it off: X and WW's own keys and MCM stay the player's, and CIGAR
	// does not move, unbind or report them. Absent WW, the module stays silent. docs/037.
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
		WizardWarrior() = default;

		enum : std::uint16_t
		{
			kActivate = PromptID::kWizardWarrior
		};

		bool IsOn() const;

		PromptSlot activate{ this, kActivate };

		RE::TESQuest* quest{ nullptr };
		RE::BSTSmartPointer<RE::BSScript::Object> script;  // QK_MainQuestScript on the quest
		RE::TESGlobal* toggle{ nullptr };  // QK_SpellToggle, WW's PowerToggle: 1 while on
		// Declined with SkyPrompt's double tap: hidden until the weapon is sheathed.
		bool dismissed{ false };
		std::chrono::steady_clock::time_point quietUntil{};
		std::chrono::steady_clock::time_point checkAt{};
		bool checking{ false };
	};
}
