#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// Quest-specific equip prompts. While The Way of the Voice (MQ105)
	// asks the player to demonstrate Unrelenting Force, 장착하기 puts that shout in the voice slot.
	// The same is offered for the quest's Whirlwind Sprint demonstration (docs/025-quest-action.md).
	class QuestAction final : public Module
	{
	public:
		static QuestAction* GetSingleton();

		const char* Name() const override { return "QuestAction"; }
		void OnGameLoaded() override;
		void Tick() override;
		void OnAccepted(std::uint16_t a_eventID) override;

	private:
		QuestAction();

		enum : std::uint16_t
		{
			kEquipShout = PromptID::kEquipShout
		};

		// The shout a displayed MQ105 objective asks for, or null; a_objective names it for the log.
		RE::TESShout* Wanted(std::uint16_t& a_objective) const;

		PromptSlot prompt{ this, kEquipShout };

		RE::TESQuest* wayOfTheVoice{ nullptr };
		RE::TESShout* unrelentingForce{ nullptr };
		RE::TESShout* whirlwindSprint{ nullptr };
	};
}
