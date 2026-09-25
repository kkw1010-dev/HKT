#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// The helmet stays off so the player's face shows (the user, 2026-09-25): 투구 벗기 keeps offering
	// itself while headgear is worn, except inside a dungeon; 투구 쓰기 comes up in combat. The
	// helmet is really unequipped and re-equipped (Helmet Toggle's hidden-variant swap left SMP hair
	// without physics), with Helmet Toggle 2's clips, copied into `CIGAR - Helmet Motions` and driven
	// by the graph variable they are keyed on (docs/028-helmet.md).
	class Helmet final : public Module
	{
	public:
		static Helmet* GetSingleton();

		const char* Name() const override { return "Helmet"; }
		void OnGameLoaded() override;
		void Tick() override;
		void FastTick() override;
		void OnAccepted(std::uint16_t a_eventID) override;
		void OnDeclined(std::uint16_t a_eventID) override;
		void OnDisabled() override;

		void Save(SKSE::SerializationInterface* a_intfc) const;
		void Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_version);
		void Revert();

	private:
		Helmet();

		enum : std::uint16_t
		{
			kHelmetOff = PromptID::kHelmetOff,
			kHelmetOn = PromptID::kHelmetOn
		};

		using Clock = std::chrono::steady_clock;

		enum class Step
		{
			kIdle,
			kUnequip,   // the take-off clip runs; unequip when it reaches the head
			kStopClip   // unequipped; end the clip
		};

		// Worn head armor CIGAR manages: ArmorHelmet or ClothingHead on the head or hair slot, not a
		// circlet.
		std::vector<RE::TESObjectARMO*> WornHeadgear(RE::PlayerCharacter* a_player) const;
		bool InDungeon(RE::PlayerCharacter* a_player, std::string& a_why) const;
		bool Carried(RE::PlayerCharacter* a_player, RE::FormID a_id) const;
		void TakeOff(RE::PlayerCharacter* a_player);
		void PutOn(RE::PlayerCharacter* a_player);
		void Clip(RE::PlayerCharacter* a_player, int a_type, const char* a_event);

		PromptSlot off{ this, kHelmetOff };
		PromptSlot on{ this, kHelmetOn };

		RE::BGSKeyword* armorHelmet{ nullptr };
		RE::BGSKeyword* clothingHead{ nullptr };
		RE::BGSKeyword* clothingCirclet{ nullptr };
		std::vector<RE::BGSKeyword*> dungeonKeywords;

		// What CIGAR took off and will put back on, kept in the co-save (record HELM).
		std::vector<RE::FormID> stowed;

		Step step{ Step::kIdle };
		Clock::time_point stepAt{};
		std::vector<RE::TESObjectARMO*> removing;
		bool clipRunning{ false };

		// A decline of 투구 벗기 hides it until the location changes; of 투구 쓰기, until combat ends.
		RE::FormID location{ 0 };
		bool offDismissed{ false };
		bool onDismissed{ false };
		bool wasInCombat{ false };
	};
}
