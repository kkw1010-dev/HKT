#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// The 그래플 prompt for Grapple (a Patreon mod, so it is absent from most setups; the module
	// then idles and logs why). Accepting presses Grapple's own hotkey through the game's input
	// event source, because its DLL handles the key itself and publishes no API.
	//
	// True Directional Movement is optional here. With TDM present the prompt also shows while
	// locked, Grapple's TargetLockKey is kept equal to TDM's, and a grapple started while locked is
	// followed by an automatic re-lock, because Grapple releases the lock for its wind-up. Without
	// TDM the prompt shows on a hostile in reach alone.
	class Grapple final : public Module
	{
	public:
		static Grapple* GetSingleton();

		const char* Name() const override { return "Grapple"; }
		void OnGameLoaded() override;
		void Tick() override {}
		void FastTick() override;
		void OnAccepted(std::uint16_t a_eventID) override;
		void OnDisabled() override;
		// Reads the grapple key Grapple's DLL saved in its INI. Call at kDataLoaded, before any game
		// starts: a new game's MCM init pushes its empty key to the DLL, which may save it there.
		void ReadIni();
		// The key the grapple prompt presses, or -1 (control panel conflict check).
		// Reads Grapple's hotkey and applies the prompt-only switch; runs at load, on the control
		// panel's key check and when the switch changes (game thread).
		void CheckKeys();
		std::int32_t Key() const { return keyOk.load() ? keyShown.load() : -1; }

	private:
		enum : std::uint16_t
		{
			kGrapple = PromptID::kGrapple
		};

		using Clock = std::chrono::steady_clock;

		Grapple() = default;

		void Resolve();
		void SyncKeys();
		void RefreshKey();
		static bool InGrapple(RE::PlayerCharacter* a_player, bool a_movable);
		void UpdateRelock(RE::PlayerCharacter* a_player, bool a_combat, bool a_locked, bool a_movable);

		PromptSlot grapple{ this, kGrapple };

		RE::TESQuest* quest{ nullptr };
		std::int32_t key{ -1 };
		bool modifier{ false };
		// The last usable grapple key seen: the DLL's INI at startup, then the MCM property.
		std::int32_t knownKey{ -1 };
		bool warnedKey{ false };
		std::atomic<bool> keyOk{ false };
		std::atomic<std::int32_t> keyShown{ -1 };

		Clock::time_point quietUntil{};
		Clock::time_point checkAt{};
		bool checkPending{ false };

		bool relockPending{ false };
		bool sawGrapple{ false };
		Clock::time_point relockStart{};
		Clock::time_point relockStable{};
		Clock::time_point relockDeadline{};
	};
}
