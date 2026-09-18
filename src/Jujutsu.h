#pragma once

#include "Module.h"
#include "Prompt.h"

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
	// Keeping the victim alive is still being worked out in game. Test 1 (2026-09-19) showed the
	// KillActor event swallowed and the victim dead anyway, so something else in the kill move kills it.
	// Until the next test decides, attempts alternate two strategies, and every step is logged:
	//   A: the victim is flagged essential (its own Actor flag, not the shared base) for the kill move;
	//   B: the victim's KillMoveEnd is swallowed too, and its in-kill-move flag is cleared afterwards.
	class Jujutsu final : public Module
	{
	public:
		static Jujutsu* GetSingleton();

		const char* Name() const override { return "Jujutsu"; }
		void OnGameLoaded() override;
		void Tick() override {}
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

		enum class Strategy
		{
			kEssential,
			kSwallowEnd
		};

		Jujutsu();

		RE::Actor* FindTarget(RE::PlayerCharacter* a_player, std::string& a_gate) const;
		bool TryPlay(RE::PlayerCharacter* a_player, RE::Actor* a_victim);
		void Watch(RE::PlayerCharacter* a_player);
		void Sample(RE::Actor* a_victim, float a_time);
		void ApplyPayoff(RE::PlayerCharacter* a_player, RE::Actor* a_victim);
		void Finish(const char* a_reason);
		std::string DescribeVictim(RE::Actor* a_victim) const;
		float Elapsed() const;

		PromptSlot jujutsu{ this, kJujutsu };

		std::vector<RE::TESIdleForm*> idles;
		const char* idleSource{ "-" };
		VAL_API::IVVAL2* valhalla{ nullptr };
		RE::TESFaction* sexlabAnimating{ nullptr };

		// Blocking comes and goes; the target stays offered briefly after its guard drops.
		RE::ActorHandle lastBlocker;
		Clock::time_point blockSeen{};
		RE::Actor* offeredTarget{ nullptr };

		Phase phase{ Phase::kIdle };
		Strategy strategy{ Strategy::kEssential };
		Strategy nextStrategy{ Strategy::kEssential };
		RE::ActorHandle victim;
		RE::TESIdleForm* playing{ nullptr };
		Clock::time_point phaseStart{};
		Clock::time_point settleUntil{};
		int tries{ 0 };
		bool setEssential{ false };
		bool payoffDone{ false };
		std::string lastSample;
		bool warnedNoStart{ false };
		bool warnedDied{ false };
	};
}
