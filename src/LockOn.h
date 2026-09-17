#pragma once

#include "Module.h"
#include "Prompt.h"

namespace TDM_API
{
	class IVTDM1;
}

namespace CIGAR
{
	// True Directional Movement target lock in combat, and Grapple on the locked target (both
	// optional; each part idles when its mod is absent). Accepting a prompt presses that mod's
	// own key through the game's input event source.
	class LockOn final : public Module
	{
	public:
		static LockOn* GetSingleton();

		const char* Name() const override { return "LockOn"; }
		void OnGameLoaded() override;
		void Tick() override;
		void OnAccepted(std::uint16_t a_eventID) override;

	private:
		enum : std::uint16_t
		{
			kLock = 0,
			kGrapple = 1
		};

		using Clock = std::chrono::steady_clock;

		LockOn() = default;

		void ResolveGrapple();
		void SyncGrappleLockKey();

		PromptSlot lock{ this, kLock };
		PromptSlot grapple{ this, kGrapple };

		TDM_API::IVTDM1* tdm{ nullptr };
		std::int64_t tdmLockKey{ -1 };

		RE::TESQuest* grappleQuest{ nullptr };
		std::int32_t grappleKey{ -1 };
		bool grappleModifier{ false };

		Clock::time_point quietUntil{};
		std::optional<std::uint16_t> checkResult;
		bool warnedKey{ false };
	};
}
