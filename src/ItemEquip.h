#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// Offers newly acquired playable weapons and armor for immediate equip; armor only when it beats
	// the worn piece and that piece is not enchanted (GearSwap's rule, folded in on 2026-09-25).
	class ItemEquip final :
		public Module,
		public RE::BSTEventSink<RE::TESContainerChangedEvent>
	{
	public:
		static ItemEquip* GetSingleton();

		const char* Name() const override { return "ItemEquip"; }
		void RegisterEvents();
		void OnGameLoaded() override;
		void Tick() override;
		void FastTick() override;
		void OnAccepted(std::uint16_t a_eventID) override;
		void OnDisabled() override;

		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESContainerChangedEvent* a_event,
			RE::BSTEventSource<RE::TESContainerChangedEvent>* a_source) override;

	private:
		ItemEquip();

		enum : std::uint16_t
		{
			kEquip = PromptID::kEquipItem
		};

		using Clock = std::chrono::steady_clock;

		void ReceiveAcquiredItem(RE::FormID a_itemID);
		bool Live(RE::PlayerCharacter* a_player, RE::TESBoundObject* a_item, std::string& a_reason) const;
		static bool AlreadyEquipped(RE::PlayerCharacter* a_player, RE::TESBoundObject* a_item);
		static bool BetterArmor(RE::PlayerCharacter* a_player, RE::TESObjectARMO* a_armor, std::string& a_reason);

		PromptSlot equip{ this, kEquip };
		RE::FormID offeredItem{ 0 };
		Clock::time_point expiresAt{};
		RE::BGSListForm* woodAxes{ nullptr };
	};
}
