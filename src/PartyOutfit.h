#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// SI's quest outfit swap (its Quest Interactions video: "Party Clothes", then back to the previous
	// gear). Diplomatic Immunity (MQ201): while the party objectives are shown and the player carries
	// the party clothes, 파티 의상 입기 stores the worn armor and puts the clothes and boots on; once the
	// party is over, 원래 장비로 puts back what is still carried. docs/029-si-leftovers.md.
	class PartyOutfit final : public Module
	{
	public:
		static PartyOutfit* GetSingleton();

		const char* Name() const override { return "PartyOutfit"; }
		void OnGameLoaded() override;
		void Tick() override;
		void OnAccepted(std::uint16_t a_eventID) override;

		void Save(SKSE::SerializationInterface* a_intfc) const;
		void Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_version);
		void Revert();

	private:
		PartyOutfit();

		enum : std::uint16_t
		{
			kWear = PromptID::kPartyOutfit,
			kBack = PromptID::kPartyRevert
		};

		bool PartyShown() const;
		bool Wearing(RE::PlayerCharacter* a_player) const;
		std::size_t CarriedStored(RE::PlayerCharacter* a_player) const;

		PromptSlot wear{ this, kWear };
		PromptSlot back{ this, kBack };

		RE::TESQuest* quest{ nullptr };
		RE::TESObjectARMO* outfit{ nullptr };
		RE::TESObjectARMO* boots{ nullptr };
		std::vector<RE::FormID> stored;
	};
}
