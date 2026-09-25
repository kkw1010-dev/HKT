#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// SI's Observer, shown as 주시하기 (renamed from 살펴보기 on 2026-09-25: the user found that it
	// read like turning a 3D model around). After the player has stood still for 5 s looking at an
	// actor within 5000 units, or at scenery (nothing under the crosshair, not looking at the floor;
	// the user, 2026-09-25: observation and scouting), holding the key narrows the field of view by up
	// to 40 degrees (SI's defaults idle_timer 5, max_distance 5000, fov_offset 40). The zoom is eased
	// every frame, in and out (the user, 2026-09-25: smoother than SI's fixed rate on a 100 ms
	// tick). Releasing the key or moving eases it back. A declined prompt stays hidden until the
	// player leaves the spot. docs/029-si-leftovers.md.
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
		void OnDeclined(std::uint16_t a_eventID) override;
		void OnDisabled() override;

	private:
		Observe();

		enum : std::uint16_t
		{
			kLook = PromptID::kObserve
		};

		using Clock = std::chrono::steady_clock;

		// The FOV glides from `from` to `to` over `length`, on a smootherstep curve.
		struct Ease
		{
			float             from{ 0.0f };
			float             to{ 0.0f };
			Clock::time_point start{};
			float             length{ 0.0f };
		};

		void Restore(std::string_view a_reason);
		void StartEase(float a_to, float a_seconds);
		void Frame();  // game thread, once a frame while an ease runs
		static void StartFrameDriver();
		void SetFOV(float a_fov) const;

		PromptSlot look{ this, kLook };
		Clock::time_point stillSince{};
		RE::ObjectRefHandle target;
		std::string targetName;

		bool zooming{ false };
		bool firstPerson{ false };
		float baseFOV{ 0.0f };
		float currentFOV{ 0.0f };
		Ease ease;
		std::atomic_bool easing{ false };
		// Declined (a double tap): hidden until the player walks away from where it was declined
		// (the user, 2026-09-25).
		bool dismissed{ false };
		RE::NiPoint3 dismissedAt{};
		// SkyPrompt's hold-and-keep type sends no decline, only down and up, so a double tap is read
		// here: two short presses close together (test 2026-09-25: the double tap only zoomed).
		Clock::time_point downAt{};
		Clock::time_point lastTapAt{};
		void Dismiss(std::string_view a_how);
		std::atomic_bool frameQueued{ false };
	};
}
