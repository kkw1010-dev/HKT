#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// SI's ItemUse make-light action, without Torches Candlelight and Lanterns (dropped from the
	// order on 2026-09-24 for its defects). In the dark, out of combat and not already lit, 불 밝히기
	// takes a torch from the pack into the left hand (what the hand held is remembered), or, with no
	// torch, casts a light spell the player knows (Candlelight first). With neither, no prompt.
	// Once it is bright again, 불 끄기 gives the left hand back.
	class Light final : public Module
	{
	public:
		static Light* GetSingleton();

		const char* Name() const override { return "Light"; }
		void OnGameLoaded() override;
		void Tick() override;
		void FastTick() override;
		void OnAccepted(std::uint16_t a_eventID) override;
		void OnDisabled() override;

	private:
		Light();

		enum : std::uint16_t
		{
			kLight = PromptID::kMakeLight,
			kPutOut = PromptID::kPutOutLight
		};

		using Clock = std::chrono::steady_clock;

		struct Source
		{
			RE::TESObjectLIGH* torch{ nullptr };
			RE::SpellItem* spell{ nullptr };
			float cost{ 0.0f };
			// Spells known but not castable now (too little magicka), for the log.
			std::size_t unaffordable{ 0 };
		};

		bool Dark(RE::PlayerCharacter* a_player) const;
		// A few thresholds, for the log only: the engine gives a comparison, not the level.
		std::string LightBand(RE::PlayerCharacter* a_player) const;
		static bool Lit(RE::PlayerCharacter* a_player, std::string& a_how);
		static bool InWater(RE::PlayerCharacter* a_player);
		// A torch carried (the one with the most light), else the cheapest light spell known.
		static Source FindSource(RE::PlayerCharacter* a_player);
		static bool IsLightSpell(const RE::SpellItem* a_spell, bool& a_self);
		void PutOut(RE::PlayerCharacter* a_player);

		PromptSlot prompt{ this, kLight };
		PromptSlot putOut{ this, kPutOut };

		RE::BGSEquipSlot* leftSlot{ nullptr };

		Clock::time_point darkSince{};
		Clock::time_point brightSince{};
		bool bright{ false };

		// The torch CIGAR put in the left hand and what the hand held before (null: empty).
		RE::TESObjectLIGH* heldTorch{ nullptr };
		RE::TESForm* savedLeft{ nullptr };
		bool torchOut{ false };

		bool checkAfterAccept{ false };
		Clock::time_point acceptedAt{};
		std::string lastNoSource;
	};
}
