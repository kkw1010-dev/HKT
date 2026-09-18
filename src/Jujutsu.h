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
	// kill. The victim's clip ends with a KillActor event; a hook on the engine's KillActorHandler
	// swallows it for that victim only, so the victim survives. At that moment the victim loses its
	// stamina, or, with Valhalla Combat, a large share of its stun meter, and a little health.
	// Balance values are placeholders until the user tunes them (the user asked for function first).
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

		// Hooks KillActorHandler::ExecuteHandler (kDataLoaded, once).
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
			kStarting,  // idle requested, waiting for the pair to start
			kRunning,   // pair running
			kSettling   // pair over, watching the victim's state for a moment
		};

		Jujutsu();

		static bool KillActorHook(RE::AnimHandler* a_this, RE::Actor& a_actor, const RE::BSFixedString& a_parameter);

		RE::Actor* FindTarget(RE::PlayerCharacter* a_player, std::string& a_gate) const;
		void Start(RE::PlayerCharacter* a_player, RE::Actor* a_victim);
		void Watch(RE::PlayerCharacter* a_player);
		void ApplyPayoff(RE::PlayerCharacter* a_player, RE::Actor* a_victim);
		void Finish(const char* a_reason);
		std::string DescribeVictim(RE::Actor* a_victim) const;

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
		bool payoffDone{ false };
		bool warnedNoStart{ false };
		bool warnedDied{ false };
	};
}
