#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// Eats the cheapest suitable food from the inventory when Survival Mode hunger reaches the
	// stage set in the control panel (optional; idle without Survival Mode). Eating is an equip,
	// so Survival Mode Improved lowers hunger and eating animations (Taberu) react as they do for
	// food eaten from the inventory menu. Raw, poisoned, alcoholic, drug and harmful food is never
	// picked. Hidden in combat.
	class Eat final : public Module
	{
	public:
		static Eat* GetSingleton();

		const char* Name() const override { return "Eat"; }
		void OnGameLoaded() override;
		void Tick() override;
		void OnAccepted(std::uint16_t a_eventID) override;

		// The hunger stage the prompt starts at, 1-5 (control panel).
		static constexpr int kMinStageLow = 1;
		static constexpr int kMinStageHigh = 5;
		static constexpr int kMinStageDefault = 3;

	private:
		enum : std::uint16_t
		{
			kEat = PromptID::kEat
		};

		using Clock = std::chrono::steady_clock;

		Eat();

		int HungerStage(float a_value) const;
		bool Edible(RE::AlchemyItem* a_food) const;
		RE::AlchemyItem* PickFood(RE::PlayerCharacter* a_player, std::size_t& a_candidates) const;
		bool Live(RE::PlayerCharacter* a_player, std::string& a_gate, RE::AlchemyItem*& a_food) const;

		PromptSlot eat{ this, kEat };

		bool active{ false };
		RE::TESGlobal* modeEnabled{ nullptr };
		RE::TESGlobal* hungerValue{ nullptr };
		std::array<RE::TESGlobal*, 5> stageValues{};
		RE::TESGlobal* smiHungerEnabled{ nullptr };
		std::array<RE::EffectSetting*, 4> hungerEffects{};
		RE::BGSListForm* rawMeat{ nullptr };
		std::vector<RE::BGSKeyword*> excludedKeywords;
		RE::TESFaction* sexlabAnimating{ nullptr };

		RE::AlchemyItem* offeredFood{ nullptr };
		Clock::time_point quietUntil{};
		bool warnedOff{ false };
		bool checkAfterEat{ false };
		float hungerBeforeEat{ 0.0f };
		bool warnedNoDrop{ false };
	};
}
