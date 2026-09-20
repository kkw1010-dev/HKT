#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// The 록온 prompt for True Directional Movement's target lock in combat. Accepting presses TDM's
	// own lock key through the game's input event source, because TDM has no API to set the lock.
	// The module idles when TDM is absent.
	//
	// The Grapple module is separate (`Grapple.h`): Grapple is a Patreon mod that most setups do not
	// have, while almost every setup has TDM. The two share only TDM's lock, through `TDMLock`.
	class LockOn final : public Module
	{
	public:
		static LockOn* GetSingleton();

		const char* Name() const override { return "LockOn"; }
		void OnGameLoaded() override;
		void Tick() override {}
		void FastTick() override;
		void OnAccepted(std::uint16_t a_eventID) override;

	private:
		enum : std::uint16_t
		{
			kLock = PromptID::kLock
		};

		using Clock = std::chrono::steady_clock;

		LockOn() = default;

		PromptSlot lock{ this, kLock };

		Clock::time_point quietUntil{};
		Clock::time_point checkAt{};
		bool checkPending{ false };
		bool warnedKey{ false };
	};
}
