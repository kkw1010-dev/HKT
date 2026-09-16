#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// Bathing and showering through Bathing in Skyrim - Renewed (optional; the module stays idle
	// when BiS is not installed). Replaces Streamlined Interactions' animation-only Bathe.
	class Bathe final : public Module
	{
	public:
		static Bathe* GetSingleton();

		const char* Name() const override { return "Bathe"; }
		void OnGameLoaded() override;
		void Tick() override;
		void OnAccepted(std::uint16_t a_eventID) override;

	private:
		enum : std::uint16_t
		{
			kBathe = 0,
			kShower = 1
		};

		Bathe() = default;

		bool ResolveBiS();
		bool UnderWaterfall(RE::PlayerCharacter* a_player) const;
		std::string DirtText() const;

		PromptSlot bathe{ this, kBathe };
		PromptSlot shower{ this, kShower };

		// Bathing in Skyrim, read from its quest script at load so no FormID is hard-coded.
		RE::TESQuest* bisQuest{ nullptr };
		RE::TESGlobal* bisEnabled{ nullptr };
		RE::TESGlobal* dirtiness{ nullptr };
		RE::TESGlobal* waterRestriction{ nullptr };
		RE::BGSKeyword* animationKeyword{ nullptr };
		RE::BGSListForm* waterfalls{ nullptr };

		bool wasInWater{ false };
		bool warnedBisOff{ false };
	};
}
