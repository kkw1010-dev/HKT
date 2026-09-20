#pragma once

#include "Module.h"
#include "Prompt.h"

#include <set>

namespace CIGAR
{
	// Drinks a potion from the inventory when the player needs one: low health, stamina or magicka,
	// a disease, a poison, or swimming with the head under water. It replaces Streamlined
	// Interactions' ItemUse potion actions, which CIGAR's SI override turns off.
	//
	// Potions are recognised by what their effects do — the actor value they modify, or their
	// archetype — never by form ID, so an alchemy overhaul's potions (Apothecary here) work with no
	// patch. A potion with any hostile or detrimental effect is never picked.
	//
	// One prompt at a time: SkyPrompt shows at most four per client, and six potion prompts would
	// crowd out every other module. The needs are checked in the order of the `Need` enum.
	class Potion final : public Module
	{
	public:
		static Potion* GetSingleton();

		const char* Name() const override { return "Potion"; }
		void OnGameLoaded() override;
		void Tick() override;
		void FastTick() override;
		void OnAccepted(std::uint16_t a_eventID) override;

		// What a prompt is for, in the order they are offered.
		enum class Need
		{
			kNone,
			kHealth,
			kWaterBreathing,
			kStamina,
			kMagicka,
			kCurePoison,
			kCureDisease
		};
		// The short name used in the log and the gate line.
		static const char* NeedTag(Need a_need);

		// The fraction of the bar at which each prompt starts (control panel).
		static constexpr float kThresholdLow = 0.05f;
		static constexpr float kThresholdHigh = 1.0f;

	private:
		enum : std::uint16_t
		{
			kDrink = PromptID::kDrink
		};

		using Clock = std::chrono::steady_clock;

		struct Pick
		{
			RE::AlchemyItem* potion{ nullptr };
			std::size_t candidates{ 0 };
			// Every AlchemyItem stack looked at, and why the rejected ones were rejected. A need
			// that holds with no pick is otherwise indistinguishable from an empty pack.
			std::size_t examined{ 0 };
			std::size_t food{ 0 };
			std::size_t poison{ 0 };
			std::size_t harmful{ 0 };
			std::size_t noMatch{ 0 };
			// The first few rejections, named, for the one diagnostic line.
			std::string rejected;
		};

		Potion();

		// The strength of a_potion for a_need: the restored amount, or 0 when it does not serve it.
		// A potion with a hostile or detrimental effect scores 0 and sets a_harmful.
		static float Strength(const RE::AlchemyItem* a_potion, Need a_need, bool& a_harmful);
		static float Strength(const RE::AlchemyItem* a_potion, Need a_need);
		Pick Scan(RE::PlayerCharacter* a_player, Need a_need, float a_missing, bool a_strongest) const;

		// The need with the highest priority that holds right now, and how much of the bar is gone.
		Need Current(RE::PlayerCharacter* a_player, float& a_ratio, float& a_missing, bool& a_urgent) const;
		static bool HasActiveValue(RE::Actor* a_actor, RE::ActorValue a_value);
		static bool Diseased(RE::Actor* a_actor);
		static bool Poisoned(RE::Actor* a_actor);

		PromptSlot drink{ this, kDrink };

		RE::TESFaction* sexlabAnimating{ nullptr };

		Need offeredNeed{ Need::kNone };
		RE::AlchemyItem* offeredPotion{ nullptr };
		Clock::time_point scannedAt{};
		Clock::time_point quietUntil{};
		bool checkAfterDrink{ false };
		Need drankFor{ Need::kNone };
		float ratioBeforeDrink{ 0.0f };
		bool warnedNoChange{ false };
		// The needs whose empty-handed scan has already been explained, once per session each.
		std::set<Need> explained;
	};
}
