#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// SI's chair drink, as the user set it for CIGAR (2026-09-24): roleplay in inns and houses only.
	// Seated in a chair there with a drink of alcohol in the pack, 마시기 plays the vanilla chair
	// drinking idle (ChairDrinkingStart, which brings its own tankard) and really drinks one bottle,
	// so its effects apply. Moving gets up, as from any chair.
	class ChairDrink final : public Module
	{
	public:
		static ChairDrink* GetSingleton();

		const char* Name() const override { return "ChairDrink"; }
		void OnGameLoaded() override;
		void Tick() override;
		void OnAccepted(std::uint16_t a_eventID) override;

	private:
		ChairDrink();

		enum : std::uint16_t
		{
			kDrink = PromptID::kChairDrink
		};

		using Clock = std::chrono::steady_clock;

		// An inn or a house: the current location or one of its parents carries a place keyword.
		// a_where names the location and the keyword (or its absence) for the log.
		bool AtInnOrHome(RE::PlayerCharacter* a_player, std::string& a_where) const;
		bool IsAlcohol(const RE::AlchemyItem* a_item) const;
		// The cheapest alcoholic drink carried, so a rare vintage is not the first to go.
		RE::AlchemyItem* PickDrink(RE::PlayerCharacter* a_player) const;

		PromptSlot prompt{ this, kDrink };

		std::vector<RE::BGSKeyword*> alcoholKeywords;
		std::vector<RE::BGSKeyword*> placeKeywords;

		// After an accept: the bottle count is checked, and getting up is watched for.
		bool checkPending{ false };
		Clock::time_point checkAt{};
		RE::AlchemyItem* drank{ nullptr };
		std::int32_t countBefore{ 0 };
		bool drinking{ false };
		Clock::time_point moveSince{};
		bool moving{ false };
		bool warnedStuck{ false };
	};
}
