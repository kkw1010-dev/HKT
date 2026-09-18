#pragma once

#include "Module.h"
#include "Prompt.h"

namespace TDM_API
{
	class IVTDM1;
}

namespace CIGAR
{
	// Offers a ranged weapon when the enemy is beyond the control panel's distance or fleeing, and a
	// melee weapon when it comes back inside it (bows and crossbows only; staves are not ranged here).
	// Each side picks a favourited weapon first, then the strongest by the inventory's damage figure.
	// A bow without arrows or a crossbow without bolts is never picked. The enemy is TDM's locked
	// target when TDM is present and locked, otherwise the nearest hostile in combat.
	class WeaponSwap final : public Module
	{
	public:
		static WeaponSwap* GetSingleton();

		const char* Name() const override { return "WeaponSwap"; }
		void OnGameLoaded() override;
		void Tick() override {}
		void FastTick() override;
		void OnAccepted(std::uint16_t a_eventID) override;

		// The switch distance in game units (control panel). The default of 800 is the user's choice.
		static constexpr float kRangeLow = 300.0f;
		static constexpr float kRangeHigh = 3000.0f;
		static constexpr float kRangeDefault = 800.0f;

	private:
		enum : std::uint16_t
		{
			kRanged = PromptID::kRanged,
			kMelee = PromptID::kMelee
		};

		enum class Zone
		{
			kNone,
			kNear,
			kFar,
			kFleeing
		};

		enum class Hands
		{
			kEmpty,
			kMelee,
			kRanged,
			kOther
		};

		struct Pick
		{
			RE::TESObjectWEAP* weapon{ nullptr };
			RE::ExtraDataList* extra{ nullptr };
			RE::TESAmmo* ammo{ nullptr };
			bool favorite{ false };
			float damage{ 0.0f };
		};

		struct State
		{
			bool combat{ false };
			bool movable{ false };
			bool sexlab{ false };
			bool quiet{ false };
			bool locked{ false };
			RE::Actor* target{ nullptr };
			float distance{ 0.0f };
			Zone zone{ Zone::kNone };
			Hands hands{ Hands::kOther };
		};

		using Clock = std::chrono::steady_clock;

		WeaponSwap() = default;

		RE::NiPointer<RE::Actor> FindTarget(RE::PlayerCharacter* a_player, bool& a_locked) const;
		static bool Fleeing(RE::Actor* a_actor);
		static Hands Wielded(RE::PlayerCharacter* a_player);
		static bool IsRanged(const RE::TESObjectWEAP* a_weapon);
		static bool IsTwoHanded(const RE::TESObjectWEAP* a_weapon);
		static RE::TESAmmo* PickAmmo(RE::PlayerCharacter* a_player, bool a_bolt);
		Pick PickWeapon(RE::PlayerCharacter* a_player, bool a_ranged) const;
		State Read(RE::PlayerCharacter* a_player, RE::NiPointer<RE::Actor>& a_hold);
		void RefreshPicks(RE::PlayerCharacter* a_player, bool a_force);
		void RestoreLeft(RE::PlayerCharacter* a_player);
		void CheckResult(RE::PlayerCharacter* a_player);

		PromptSlot ranged{ this, kRanged };
		PromptSlot melee{ this, kMelee };

		TDM_API::IVTDM1* tdm{ nullptr };
		RE::TESFaction* sexlabAnimating{ nullptr };
		const RE::BGSEquipSlot* rightSlot{ nullptr };
		const RE::BGSEquipSlot* leftSlot{ nullptr };

		Zone lastZone{ Zone::kNone };
		Pick rangedPick;
		Pick meleePick;
		RE::TESObjectWEAP* offeredRanged{ nullptr };
		RE::TESObjectWEAP* offeredMelee{ nullptr };
		Clock::time_point nextScan{};
		Clock::time_point quietUntil{};

		// The left hand before switching to a bow, given back with a one-handed melee weapon.
		RE::TESForm* savedLeft{ nullptr };

		// Checked once the equip has settled.
		RE::TESObjectWEAP* expected{ nullptr };
		bool checkPending{ false };
		bool warnedEquip{ false };
	};
}
