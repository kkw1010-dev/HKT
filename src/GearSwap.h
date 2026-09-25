#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// SI's Outfit Swap (Piecewise): "prompt to swap armor/clothing at different body parts with
	// available ones in the vicinity. Note that the items can be out in the world or in a container."
	// CIGAR reads the crosshair: a loose armor piece, or a container or corpse, that holds a better
	// piece for the head, body, hands or feet than the one worn (same weight class, higher armor
	// rating; empty parts are left alone). Taking it is never a crime. docs/029-si-leftovers.md.
	class GearSwap final : public Module
	{
	public:
		static GearSwap* GetSingleton();

		const char* Name() const override { return "GearSwap"; }
		void OnGameLoaded() override;
		void Tick() override;
		void OnAccepted(std::uint16_t a_eventID) override;

	private:
		GearSwap();

		enum : std::uint16_t
		{
			kSwap = PromptID::kSwapGear
		};

		struct Offer
		{
			RE::ObjectRefHandle source;    // the loose item or the container
			RE::TESObjectARMO*  armor{ nullptr };
			const char*         part{ "" };
			float               gain{ 0.0f };
			std::string         why;
		};

		Offer Evaluate(RE::PlayerCharacter* a_player) const;
		static float Rating(const RE::TESObjectARMO* a_armor);
		static bool SameClass(const RE::TESObjectARMO* a_a, const RE::TESObjectARMO* a_b);
		void Consider(RE::PlayerCharacter* a_player, RE::TESObjectARMO* a_armor, Offer& a_best) const;

		PromptSlot swap{ this, kSwap };
		Offer offered;
	};
}
