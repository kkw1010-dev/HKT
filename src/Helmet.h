#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// SI's HelmetToggle ("prompt to toggle the helmet off/on when the player enters a safe/unsafe
	// location") on top of Helmet Toggle 2, whose own hotkey it replaces (the user, 2026-09-25).
	// Safe and unsafe come from Helmet Toggle's own lists (HT_SafeLocations / HT_UnsafeLocations,
	// filled by its FLM ini) and its location switches; accepting calls the function its hotkey
	// calls, HT_MCM.PressHotkey() (docs/028-helmet.md).
	class Helmet final : public Module
	{
	public:
		static Helmet* GetSingleton();

		const char* Name() const override { return "Helmet"; }
		void OnGameLoaded() override;
		void Tick() override;
		void OnAccepted(std::uint16_t a_eventID) override;
		void OnDeclined(std::uint16_t a_eventID) override;
		void OnDisabled() override;

	private:
		Helmet();

		enum : std::uint16_t
		{
			kHelmetOff = PromptID::kHelmetOff,
			kHelmetOn = PromptID::kHelmetOn
		};

		using Clock = std::chrono::steady_clock;

		// Helmet Toggle's own reading of the player's location: its unsafe list wins, then its safe
		// list, then (when its option is on) the parent location's keywords against the safe list.
		bool Safe(RE::PlayerCharacter* a_player, std::string& a_why) const;
		// The worn headgear Helmet Toggle manages (slots 30, 31, 42, 44, 55 with its keywords), or null.
		RE::TESObjectARMO* WornHeadgear(RE::PlayerCharacter* a_player) const;
		static float Value(const RE::TESGlobal* a_global);
		bool Press();

		PromptSlot off{ this, kHelmetOff };
		PromptSlot on{ this, kHelmetOn };

		RE::TESQuest* quest{ nullptr };
		RE::TESGlobal* hotkeyType{ nullptr };
		RE::TESGlobal* helmetState{ nullptr };
		RE::TESGlobal* enableLocation{ nullptr };
		RE::TESGlobal* enableParent{ nullptr };
		RE::TESGlobal* enableInteriors{ nullptr };
		RE::TESGlobal* enableWeather{ nullptr };
		RE::TESGlobal* enableSeasons{ nullptr };
		RE::BGSListForm* safeList{ nullptr };
		RE::BGSListForm* unsafeList{ nullptr };
		std::vector<RE::BGSKeyword*> headgearKeywords;
		RE::BGSKeyword* ignoreKeyword{ nullptr };
		RE::BGSKeyword* circletKeyword{ nullptr };
		bool ready{ false };

		// A decline hides both prompts until the location changes.
		RE::FormID location{ 0 };
		bool dismissed{ false };
		// After an accept: the state wanted, when to check it, and whether a second press was spent
		// (Helmet Toggle's simple hotkey flips its own remembered state, which can disagree with
		// what is worn after a dialogue or an equip).
		bool pending{ false };
		bool wantOn{ false };
		bool pressedTwice{ false };
		Clock::time_point checkAt{};
		bool warnedType{ false };
		// Helmet Toggle hides headgear with a Dynamic Armor Variants swap, so SMP hair stays without
		// physics until the next physics reset (the user, 2026-09-25). After a change CIGAR presses
		// Auto Physics Reset's manual reset key, read from its INI; 0 when that mod is absent.
		std::int64_t resetKey{ 0 };
		Clock::time_point resetAt{};
	};
}
