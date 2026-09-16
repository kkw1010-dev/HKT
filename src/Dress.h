#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// Undressing in water, at a bed or at a wardrobe/dresser, and dressing again after leaving.
	// Replaces Streamlined Interactions' Water, Bed and Wardrobe Undress, which strip the
	// Softbody SMP collision carrier.
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

		// Co-save: the items undress removed.
		void Save(SKSE::SerializationInterface* a_intfc) const;
		void Load(SKSE::SerializationInterface* a_intfc);
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
		void Undress(RE::PlayerCharacter* a_player);
		void DressUp(RE::PlayerCharacter* a_player);

		PromptSlot undress{ this, kUndress };
		PromptSlot dress{ this, kDress };

		std::vector<RE::FormID> removed;
		RE::ObjectRefHandle place;
		const char* placeKind{ "" };
		RE::ObjectRefHandle lastLoggedFurniture;
		std::string lastContext;
		bool ready{ false };
	};
}
