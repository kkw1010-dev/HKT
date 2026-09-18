#pragma once

#include "Module.h"
#include "Prompt.h"

#include <unordered_map>

namespace VAL_API
{
	class IVVAL2;
}

namespace CIGAR
{
	// Valhalla Combat's execution as a prompt (optional; idle without Valhalla). The prompt shows only
	// while Valhalla's own execution key would execute someone: the actor it would pick (the nearest
	// stun-broken actor within 250 units) passes every check of its attemptExecute, and a kill move
	// exists for that race and the player's weapon. Accepting presses Valhalla's execution key through
	// the input event source, so Valhalla plays its own kill move. Prompt-only mode (default on) binds
	// that key to F15, which no keyboard sends.
	class Execute final : public Module
	{
	public:
		static Execute* GetSingleton();

		const char* Name() const override { return "Execute"; }
		void OnGameLoaded() override;
		void Tick() override {}
		void FastTick() override;
		void OnAccepted(std::uint16_t a_eventID) override;
		void OnDisabled() override { pressPending = false; }

		// Applies the prompt-only switch to iExecutionKey in Valhalla's INI. Called at kPostLoad, before
		// Valhalla reads that file at kDataLoaded, so no reload is needed then.
		void PrepareKey();
		// Control panel key check and prompt-only switch: re-reads the INI, applies the switch and, when
		// the file changed, asks Valhalla to reload it (game thread).
		void CheckKey();
		// The key the prompt presses, or -1 (control panel; any thread).
		std::int64_t ExecutionKey() const { return present.load() ? executionKey.load() : -1; }

		static constexpr std::int32_t kHiddenKey = 0x66;  // F15

	private:
		enum : std::uint16_t
		{
			kExecute = PromptID::kExecute
		};

		// Valhalla's race categories, in its RaceMapping section order.
		enum class Category : int
		{
			kHumanoid = 0,
			kUndead,
			kFalmer,
			kSpider,
			kGargoyle,
			kGiant,
			kBear,
			kSabreCat,
			kWolf,
			kTroll,
			kHagraven,
			kSpriggan,
			kBoar,
			kRiekling,
			kAshHopper,
			kSteamCenturion,
			kDwarvenBallista,
			kChaurusFlyer,
			kLurker,
			kDragon
		};

		using Clock = std::chrono::steady_clock;

		Execute();

		// Reads the INI and applies the switch; true when the file was changed.
		bool ApplyKeyMode();
		void ReloadValhalla();
		void LoadRaceMap();
		RE::Actor* FindVictim(RE::PlayerCharacter* a_player, float& a_distance) const;
		bool Executable(RE::PlayerCharacter* a_player, std::string& a_gate, RE::Actor*& a_victim) const;

		PromptSlot execute{ this, kExecute };

		VAL_API::IVVAL2* api{ nullptr };
		std::atomic<bool> present{ false };
		std::atomic<std::int64_t> executionKey{ -1 };
		bool stunEnabled{ true };
		std::unordered_map<RE::TESRace*, Category> races;
		RE::TESFaction* sexlabAnimating{ nullptr };
		bool warned{ false };

		RE::Actor* offeredVictim{ nullptr };
		// After a press: did a kill move start?
		bool pressPending{ false };
		RE::ActorHandle pressedVictim;
		Clock::time_point pressDeadline{};
		bool warnedNoKillMove{ false };
	};
}
