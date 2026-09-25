#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// 독 바르기 (the user, 2026-09-25): with a weapon drawn whose right-hand
	// entry carries no poison, offer the most valuable poison carried and apply it the way the
	// inventory does (InventoryEntryData::PoisonObject, doses through the Mod Poison Dose Count perk
	// entry point). docs/033-poison.md.
	class Poison final : public Module
	{
	public:
		static Poison* GetSingleton();

		const char* Name() const override { return "Poison"; }
		void OnGameLoaded() override;
		void Tick() override;
		void OnAccepted(std::uint16_t a_eventID) override;

	private:
		Poison();

		enum : std::uint16_t
		{
			kApply = PromptID::kPoison
		};

		struct Pick
		{
			RE::InventoryEntryData* entry{ nullptr };
			RE::TESObjectWEAP*      weapon{ nullptr };
			RE::AlchemyItem*        poison{ nullptr };
			std::string             why;
		};

		Pick Evaluate(RE::PlayerCharacter* a_player) const;

		PromptSlot apply{ this, kApply };
	};
}
