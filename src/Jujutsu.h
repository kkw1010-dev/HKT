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
	// kill. At the victim's kill moment it loses its stamina, or, with Valhalla Combat, a share of its stun
	// meter (panel; the user's defaults 15% on a guarding target, 25% after a perfect parry), and a little
	// health. A perfect parry (Valhalla or Parry for All) opens the attacker to 유술 at any distance for the
	// panel's window (1.5 s), with slow motion from the parry into the throw (panel; x0.3, held 0.5 s
	// after the pair starts). A 유술 on a guarding target has no slow motion (the user's rule).
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

		Jujutsu();

		RE::Actor* FindTarget(RE::PlayerCharacter* a_player, std::string& a_gate) const;
		// A perfect parry opens the attacker to 유술 for the panel's window, at any distance: Valhalla's
		// (the player blocked and the attacker was staggered at once) or Parry for All's (GotParriedCMF 2).
		void DetectParry(RE::PlayerCharacter* a_player);
		RE::Actor* ParriedTarget() const;
		void StartSlow(Clock::time_point a_until);
		// The engine refuses a paired idle while the player blocks, and a held block key puts the block
		// back the next frame, so the attempt clears the player's own want-to-block bit as well.
		void StopPlayerBlock(RE::PlayerCharacter* a_player);
		void EndSlow(const char* a_reason);
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

		// Perfect parry.
		bool parryAll{ false };
		RE::ActorHandle parried;
		Clock::time_point parryUntil{};
		const char* parrySource{ "-" };
		Clock::time_point playerBlockSeen{};
		std::unordered_map<RE::FormID, bool> wasStaggering;
		std::unordered_map<RE::FormID, std::int32_t> lastParriedCMF;
		bool fromParry{ false };

		Clock::time_point prepareUntil{};

		// Slow motion at the start of the throw.
		bool slowOwned{ false };
		float slowMultiplier{ 1.0f };
		Clock::time_point slowUntil{};
		bool warnedDied{ false };

	public:
		// The grapple reach (control panel). The default of 250 is the user's choice.
		static constexpr float kReachLow = 100.0f;
		static constexpr float kReachHigh = 400.0f;
		static constexpr float kReachDefault = 250.0f;
	};
}
