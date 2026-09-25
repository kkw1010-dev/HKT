#pragma once

#include "Module.h"
#include "Prompt.h"

#include <unordered_map>
#include <unordered_set>

namespace VAL_API
{
	class IVVAL2;
}

namespace CIGAR
{
	// 유술: a vanilla hand-to-hand kill move played on a humanoid enemy that is blocking, without the
	// kill. At the victim's kill moment it loses its stamina, or, with Valhalla Combat, a large share of
	// its stun meter, and a little health. Balance values are placeholders (the user asked for function
	// first).
	//
	// The kill comes from the victim's KillMoveEnd event (test 2, 2026-09-19: with it passed through the
	// victim died on that very tick, essential flag or not). Hooks on the engine's KillActor and
	// KillMoveEnd handlers swallow both for the victim; its in-kill-move flag is cleared by hand, and it
	// is knocked into ragdoll so it does not stand straight up out of the throw (the user's call).
	class Jujutsu final : public Module
	{
	public:
		static Jujutsu* GetSingleton();

		const char* Name() const override { return "Jujutsu"; }
		void OnGameLoaded() override;
		void Tick() override;
		void FastTick() override;
		void OnAccepted(std::uint16_t a_eventID) override;
		void OnDisabled() override;

		// Hooks the KillActor, KillMoveStart and KillMoveEnd anim-event handlers (kDataLoaded, once).
		static void InstallHook();

	private:
		enum : std::uint16_t
		{
			kJujutsu = PromptID::kJujutsu
		};

		using Clock = std::chrono::steady_clock;

		enum class Phase
		{
			kIdle,
			kPreparing,  // the victim's guard is being dropped; the idle is retried
			kStarting,   // idle accepted, waiting for the pair to start
			kRunning,    // pair running
			kSettling    // pair over, watching the victim's state for a moment
		};

		Jujutsu();

		RE::Actor* FindTarget(RE::PlayerCharacter* a_player, std::string& a_gate) const;
		bool TryPlay(RE::PlayerCharacter* a_player, RE::Actor* a_victim);
		void Watch(RE::PlayerCharacter* a_player);
		void Sample(RE::Actor* a_victim, float a_time);
		void ApplyPayoff(RE::PlayerCharacter* a_player, RE::Actor* a_victim);
		void Finish(const char* a_reason);
		std::string DescribeVictim(RE::Actor* a_victim) const;
		std::string DescribeRefusal(RE::PlayerCharacter* a_player, RE::Actor* a_victim) const;
		void EndKillMove(RE::PlayerCharacter* a_player, RE::Actor* a_victim);

	public:
		// Game thread, queued by the KillMoveEnd hook: the moment the victim would have died.
		void OnVictimKillMoveEnd();

	private:
		float Elapsed() const;

		PromptSlot jujutsu{ this, kJujutsu };

		std::vector<RE::TESIdleForm*> idles;
		std::vector<const char*> idleNames;  // parallel to idles, for the log
		std::vector<bool> idleLethal;        // parallel to idles
		bool lethal{ false };                // the move being played kills
		// Moves that have started a pair since the game started (the process, not the save): the
		// first-use experiment's memory. Not cleared on load, as loaded animations are not.
		std::unordered_set<RE::FormID> playedThisSession;
		// Test 25 split (the user: the first fight of every test is poor). Test 24 ruled out a move's
		// first use; the first fight and an actor already there when the save loaded are still
		// confounded, so each press logs both. Actors seen in the first scan after a load are "at load".
		std::unordered_map<RE::FormID, bool> seenActors;  // FormID -> present at load
		bool scannedSinceLoad{ false };
		bool inCombat{ false };
		int combatIndex{ 0 };
		std::unordered_map<RE::FormID, std::pair<int, int>> victimTally;  // played, refused
		bool firstUse{ false };
		std::chrono::milliseconds prepareWindow{ 300 };
		const char* idleSource{ "-" };
		VAL_API::IVVAL2* valhalla{ nullptr };
		RE::TESFaction* sexlabAnimating{ nullptr };

		// Blocking comes and goes; the target stays offered briefly after its guard drops.
		RE::ActorHandle lastBlocker;
		Clock::time_point blockSeen{};
		RE::Actor* offeredTarget{ nullptr };

		Phase phase{ Phase::kIdle };
		RE::ActorHandle victim;
		RE::TESIdleForm* playing{ nullptr };
		Clock::time_point phaseStart{};
		Clock::time_point settleUntil{};
		int tries{ 0 };
		bool knocked{ false };
		bool ended{ false };
		Clock::time_point knockAt{};
		bool payoffDone{ false };
		std::string lastSample;
		bool warnedNoStart{ false };
		bool warnedDied{ false };

	public:
		// The grapple reach (control panel). The default of 250 is the user's choice.
		static constexpr float kReachLow = 100.0f;
		static constexpr float kReachHigh = 400.0f;
		static constexpr float kReachDefault = 250.0f;
	};
}
