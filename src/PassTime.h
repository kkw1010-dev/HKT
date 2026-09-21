#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// Opens Skyrim's own wait menu after the player has stood still for a short time.
	class PassTime final : public Module
	{
	public:
		static PassTime* GetSingleton();

		const char* Name() const override { return "PassTime"; }
		void OnGameLoaded() override;
		void Tick() override;
		void FastTick() override;
		void OnAccepted(std::uint16_t a_eventID) override;
		void OnDisabled() override;

	private:
		PassTime();

		enum : std::uint16_t
		{
			kWait = PromptID::kPassTime
		};

		using Clock = std::chrono::steady_clock;

		PromptSlot wait{ this, kWait };
		Clock::time_point idleSince{};
	};
}
