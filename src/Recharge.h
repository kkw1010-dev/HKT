#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// SI's ItemUse recharge-weapon action: out of combat, an equipped enchanted weapon (staves
	// included) whose charge has run low is offered 충전하기, which spends a filled soul gem from the
	// inventory on it. The gem's worth is the game's own iSoulLevelValue* setting (a magic overhaul
	// changes it) passed through the perk entry point Mod Soul Gem Recharge, so enchanting perks count
	// as they do in the inventory's own recharge.
	//
	// The charge is written the way SI's DLL names it (RestoreAV<TESSoulGem>): the hand's item-charge
	// actor value is restored, and the equipped instance's ExtraCharge is set to match. A reusable
	// gem (Azura's Star, the Black Star) is given back empty.
	class Recharge final : public Module
	{
	public:
		static Recharge* GetSingleton();

		const char* Name() const override { return "Recharge"; }
		void OnGameLoaded() override;
		void Tick() override;
		void OnAccepted(std::uint16_t a_eventID) override;

	private:
		Recharge();

		enum : std::uint16_t
		{
			kRecharge = PromptID::kRecharge
		};

		using Clock = std::chrono::steady_clock;

		struct Hand
		{
			bool left{ false };
			RE::TESObjectWEAP* weapon{ nullptr };
			RE::ExtraDataList* xList{ nullptr };
			// The charge the game uses: the hand's item-charge actor value while its permanent value
			// is set, otherwise the instance's ExtraCharge against its enchantment's capacity.
			float current{ 0.0f };
			float max{ 0.0f };
			bool fromAV{ false };
			// Both sources, for the log.
			float avCurrent{ 0.0f };
			float avMax{ 0.0f };
			float itemCurrent{ 0.0f };
			float itemMax{ 0.0f };

			float Ratio() const { return max > 0.0f ? std::clamp(current / max, 0.0f, 1.0f) : 1.0f; }
			float Missing() const { return std::max(max - current, 0.0f); }
		};

		struct Gem
		{
			RE::TESSoulGem* base{ nullptr };
			// The instance carrying the soul (ExtraSoul), or nullptr for a pre-filled base form.
			RE::ExtraDataList* xList{ nullptr };
			RE::SOUL_LEVEL soul{ RE::SOUL_LEVEL::kNone };
			bool reusable{ false };
			// The charge it restores after perks.
			float worth{ 0.0f };
		};

		struct Pick
		{
			std::optional<Gem> gem;
			std::size_t filled{ 0 };
			std::size_t skipped{ 0 };
			std::string skippedWhy;
		};

		static std::optional<Hand> ReadHand(RE::PlayerCharacter* a_player, bool a_left);
		// The lower of the two hands at or below the threshold, if any.
		static std::optional<Hand> NeedyHand(RE::PlayerCharacter* a_player);
		Pick Scan(RE::PlayerCharacter* a_player, const Hand& a_hand) const;
		float Worth(RE::PlayerCharacter* a_player, RE::TESSoulGem* a_gem, RE::SOUL_LEVEL a_soul, RE::TESObjectWEAP* a_weapon) const;
		std::int32_t FilledGemCount(RE::PlayerCharacter* a_player) const;

		PromptSlot prompt{ this, kRecharge };

		RE::BGSKeyword* reusableKeyword{ nullptr };
		// How Mod Soul Gem Recharge takes its arguments beyond the perk owner, read from the engine's
		// entry-point table at load: -1 unknown (perks skipped), 0 none, 1 the soul gem, 2 the weapon.
		int perkArgument{ -1 };

		bool checkAfterAccept{ false };
		Clock::time_point acceptedAt{};
		bool acceptedLeft{ false };
		float chargeBefore{ 0.0f };
		float expected{ 0.0f };
		std::int32_t gemsBefore{ 0 };
		bool warnedNoChange{ false };
		bool explainedNoGem{ false };
	};
}
