#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// Voice answers (docs/046, docs/047): in a fight, a single-press prompt that puts the shout which fits
	// the situation into the voice slot; the player shouts with their own key. A small curated table of
	// {rule, shout}, read from the top, the first rule that holds and whose shout the player can use wins;
	// one prompt at a time. Nothing is put back after the fight (the user, D40, 2026-10-03). The rules are
	// the user's D57 (2026-10-04): a friend in front or an unshakeable foe -> Slow Time, a pack of animals
	// -> Kyne's Peace, a crowd Dismay can break -> Dismay, any other crowd -> Unrelenting Force.
	class VoiceAnswer final : public Module
	{
	public:
		static VoiceAnswer* GetSingleton();

		const char* Name() const override { return "VoiceAnswer"; }
		void OnGameLoaded() override;
		void Tick() override;
		void OnAccepted(std::uint16_t a_eventID) override;

		// What the fight looks like around the player, for the rules.
		struct Fight
		{
			// Counted from the actors near the player, each step narrowing the last, so the gate line shows
			// where a crowd stops counting (r18, r19).
			int listed{ 0 };          // actors in the high-process list (the whole neighbourhood)
			int hostile{ 0 };         // alive, loaded, hostile to the player and within range (Util::NearbyHostiles)
			int bleeding{ 0 };        // of those, bleeding out (not counted further)
			int enemies{ 0 };         // the rest that are in combat: what the rules use
			int playerSees{ 0 };      // of those, in the player's line of sight (logged only)
			int seesPlayer{ 0 };      // of those, with the player in their line of sight (logged only)
			// About the enemies, for the rules (docs/047).
			int animals{ 0 };         // ActorTypeAnimal
			int unshakeable{ 0 };     // dragons and Dwarven centurions: Unrelenting Force does not throw them
			int dismayProof{ 0 };     // undead, daedra, dwarven automatons, dragons: Dismay does nothing to them
			int maxLevel{ 0 };        // the highest enemy level
			// Around the player.
			int friendsAhead{ 0 };    // non-hostile actors within range and within the front cone
			std::string friendAhead;  // one of them, for the log
			int dismayCap{ 0 };       // the level Dismay reaches with the words the player has unlocked (0: none)
		};

	private:
		VoiceAnswer();

		enum : std::uint16_t
		{
			kEquip = PromptID::kVoiceAnswer
		};

		struct Answer
		{
			const char* rule;  // for the log
			RE::FormID shoutID;
			// Empty when the situation holds, else why not (for the log).
			std::string (*check)(const Fight&);
			RE::TESShout* shout{ nullptr };
		};

		Fight Look(RE::PlayerCharacter* a_player) const;
		// The first answer whose rule holds and which the player can use now, or null. a_rules gets every
		// rule's verdict, in order, for the gate line.
		const Answer* Pick(RE::PlayerCharacter* a_player, const Fight& a_fight, std::string& a_rules) const;

		PromptSlot prompt{ this, kEquip };
		std::vector<Answer> answers;
		RE::TESShout* dismay{ nullptr };
		const Answer* offered{ nullptr };
		bool wasInCombat{ false };
		std::uint64_t fightNumber{ 0 };
		// The pick must hold on two ticks in a row (about a second), so it does not flicker.
		const Answer* candidate{ nullptr };
	};
}
