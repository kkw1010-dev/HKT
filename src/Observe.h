#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// SI's Observer (살펴보기): after the player has stood still for 5 s looking at something within
	// 5000 units, hold the key to narrow the field of view by up to 40 degrees at 20 degrees a second
	// (SI's defaults idle_timer 5, max_distance 5000, fov_offset 40, fov_increment 20). Releasing the
	// key or moving restores it. docs/029-si-leftovers.md.
	class Observe final : public Module
	{
	public:
		static Observe* GetSingleton();

		const char* Name() const override { return "Observe"; }
		void OnGameLoaded() override;
		void Tick() override {}
		void FastTick() override;
		void OnAccepted(std::uint16_t) override {}
		void OnHold(std::uint16_t a_eventID, bool a_down) override;
		void OnDisabled() override;

	private:
		Observe();

		enum : std::uint16_t
		{
			kLook = PromptID::kObserve
		};

		using Clock = std::chrono::steady_clock;

		void Restore(std::string_view a_reason);

		PromptSlot look{ this, kLook };
		Clock::time_point stillSince{};
		RE::ObjectRefHandle target;
		std::string targetName;

		bool zooming{ false };
		bool firstPerson{ false };
		float baseFOV{ 0.0f };
		float currentFOV{ 0.0f };
		Clock::time_point lastStep{};
	};
}
