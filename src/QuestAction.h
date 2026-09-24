#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// SI's QuestActions action. The one SI's menu lists is "Greybeards: Dragonborn, show us your
	// Thu'um." -> "Quest Action Prompt: Equip Unrelenting Force": while The Way of the Voice (MQ105)
	// asks the player to demonstrate Unrelenting Force, 장착하기 puts that shout in the voice slot.
	// CIGAR adds the same for the quest's Whirlwind Sprint demonstration (docs/025-quest-action.md).
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
