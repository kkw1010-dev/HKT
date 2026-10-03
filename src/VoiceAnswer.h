#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// Voice answers (docs/046): in a fight, a single-press prompt that puts the shout which fits the
	// situation into the voice slot; the player shouts with their own key. A small curated table of
	// {shout, situation}, one prompt at a time; the first entry is Unrelenting Force against two or more
	// enemies. Nothing is put back after the fight (the user, D40, 2026-10-03: other shouts will be offered
	// as other situations come).
	class VoiceAnswer final : public Module
	{
	public:
		static VoiceAnswer* GetSingleton();

		const char* Name() const override { return "VoiceAnswer"; }
		void OnGameLoaded() override;
		void Tick() override;
		void OnAccepted(std::uint16_t a_eventID) override;

		// What the fight looks like around the player, for the table's situations.
		struct Fight
		{
			// Counted from the actors near the player, each step narrowing the last, so the gate line shows
			// where a crowd stops counting (r18: the prompt never showed and the log could not say why).
			int nearby{ 0 };          // alive, loaded, not bleeding out, within range
			int hostile{ 0 };         // ... and hostile to the player
			int enemies{ 0 };         // ... and in combat: what the situations use
			int playerSees{ 0 };      // of those, in the player's line of sight (logged only)
			int seesPlayer{ 0 };      // of those, with the player in their line of sight (logged only)
		};

	private:
		VoiceAnswer();

		enum : std::uint16_t
		{
			kEquip = PromptID::kVoiceAnswer
		};

		struct Answer
		{
			RE::FormID shoutID;
			const char* situation;  // for the log
			bool (*fits)(const Fight&);
			RE::TESShout* shout{ nullptr };
		};

		Fight Look(RE::PlayerCharacter* a_player) const;
		// The first answer whose situation holds and which the player can use now, or null; a_why says
		// why none, for the gate line.
		const Answer* Pick(RE::PlayerCharacter* a_player, const Fight& a_fight, std::string& a_why) const;

		PromptSlot prompt{ this, kEquip };
		std::vector<Answer> answers;
		const Answer* offered{ nullptr };
		bool wasInCombat{ false };
		std::uint64_t fightNumber{ 0 };
		// The situation must hold on two ticks in a row (about a second), so it does not flicker.
		const Answer* candidate{ nullptr };
	};
}
