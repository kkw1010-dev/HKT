#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// SI's ItemUse make-light action: in the dark for a while, out of combat and not already lit,
	// 불 밝히기 presses Torches Candlelight and Lanterns' own hotkey, so TCL picks the lantern, torch
	// or Candlelight and handles fuel, sneaking and weapons itself. In prompt-only mode TCL's key is
	// moved to a key no keyboard sends, through MCM Helper, and given back when switched off.
	class Light final : public Module
	{
	public:
		static Light* GetSingleton();

		const char* Name() const override { return "Light"; }
		void OnGameLoaded() override;
		void Tick() override;
		void FastTick() override;
		void OnAccepted(std::uint16_t a_eventID) override;

		// Moves TCL's key to or from the hidden key to match the panel (game thread).
		void ApplyKeyMode();
		// The key TCL listens to now (its i329Hotkey global), or -1.
		std::int32_t Key() const { return tclKey.load(); }

	private:
		Light();

		enum : std::uint16_t
		{
			kLight = PromptID::kMakeLight
		};

		using Clock = std::chrono::steady_clock;

		bool Dark(RE::PlayerCharacter* a_player) const;
		// A few thresholds, for the log only: the engine gives a comparison, not the level.
		std::string LightBand(RE::PlayerCharacter* a_player) const;
		bool Lit(RE::PlayerCharacter* a_player, std::string& a_how) const;
		void SetTCLKey(std::int32_t a_key);

		PromptSlot prompt{ this, kLight };

		RE::TESQuest* mcmQuest{ nullptr };
		RE::TESGlobal* hotkeyGlobal{ nullptr };
		RE::TESGlobal* lanternHandOn{ nullptr };
		std::vector<RE::BGSListForm*> litLanterns;
		RE::BGSKeyword* candlelightKeyword{ nullptr };
		std::atomic<std::int32_t> tclKey{ -1 };

		Clock::time_point darkSince{};
		Clock::time_point acceptedAt{};
		bool checkAfterAccept{ false };
	};
}
