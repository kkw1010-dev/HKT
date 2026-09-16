#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// Undressing and dressing by the player's current state: at any bed or wardrobe/dresser the
	// prompt is 탈의하기 when dressed and 착용하기 when naked; in water it is 탈의하기 (bathing
	// follows), and 착용하기 comes after leaving the water. Replaces Streamlined Interactions'
	// Water, Bed and Wardrobe Undress, which strip the Softbody SMP collision carrier.
	class Dress final :
		public Module,
		public RE::BSTEventSink<SKSE::CrosshairRefEvent>
	{
	public:
		static Dress* GetSingleton();

		const char* Name() const override { return "Dress"; }
		void OnGameLoaded() override;
		void Tick() override;
		void OnAccepted(std::uint16_t a_eventID) override;

		void RegisterEvents();

		// Co-save: the remembered outfit and whether CIGAR undressed the player.
		void Save(SKSE::SerializationInterface* a_intfc) const;
		void Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_version);
		void Revert();

		RE::BSEventNotifyControl ProcessEvent(const SKSE::CrosshairRefEvent* a_event, RE::BSTEventSource<SKSE::CrosshairRefEvent>*) override;

	private:
		enum : std::uint16_t
		{
			kUndress = 0,
			kDress = 1
		};

		Dress() = default;

		void OnAim(RE::ObjectRefHandle a_handle);
		const char* PlaceKind(RE::TESObjectREFR* a_ref);
		const char* CurrentContext(RE::PlayerCharacter* a_player);
		void Remember(const std::vector<RE::TESObjectARMO*>& a_worn);
		bool OutfitAvailable(RE::PlayerCharacter* a_player) const;
		void Undress(RE::PlayerCharacter* a_player);
		void DressUp(RE::PlayerCharacter* a_player);

		PromptSlot undress{ this, kUndress };
		PromptSlot dress{ this, kDress };

		// The last strippable set the player wore; dress restores it.
		std::vector<RE::FormID> outfit;
		// CIGAR undressed the player and they have not dressed since.
		bool undressedByCIGAR{ false };
		// Ticks to wait after undressing before trusting the worn state again (unequips are queued).
		int settleTicks{ 0 };

		RE::ObjectRefHandle place;
		const char* placeKind{ "" };
		RE::ObjectRefHandle lastLoggedFurniture;
		std::string lastContext;
		bool ready{ false };
	};
}
