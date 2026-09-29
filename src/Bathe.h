#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// Bathing and showering through Bathing in Skyrim - Renewed (optional; the module stays idle
	// when BiS is not installed).
	class Bathe final : public Module
	{
	public:
		static Bathe* GetSingleton();

		const char* Name() const override { return "Bathe"; }
		void OnGameLoaded() override;
		void Tick() override;
		void OnAccepted(std::uint16_t a_eventID) override;

		// Bathing in Skyrim is running its wash animation on the actor (its AnimationKeyword effect).
		// Other modules keep their prompts out of a bath (Needs, since 2026-09-27).
		bool Washing(RE::Actor* a_actor) const;

	private:
		enum : std::uint16_t
		{
			kBathe = PromptID::kBathe,
			kShower = PromptID::kShower
		};

		// Non-combat actions fill a ring (the user's rule, 2026-09-29): a single press would act on
		// the first tap, so the double-tap decline could never reach it.
		Bathe()
		{
			bathe.SetPromptType(SkyPromptAPI::kHold);
			shower.SetPromptType(SkyPromptAPI::kHold);
		}

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
