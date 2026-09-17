#pragma once

#include "Module.h"
#include "Prompt.h"

namespace TDM_API
{
	class IVTDM1;
}

namespace CIGAR
{
	// True Directional Movement target lock in combat, and Grapple when locked or a hostile is in
	// reach (both optional; each part idles when its mod is absent). Accepting a prompt presses
	// that mod's own key through the game's input event source. A grapple started while locked is
	// followed by an automatic re-lock, because Grapple releases the lock for its wind-up.
	class LockOn final : public Module
	{
	public:
		static LockOn* GetSingleton();

		const char* Name() const override { return "LockOn"; }
		void OnGameLoaded() override;
		void Tick() override {}
		void FastTick() override;
		void OnAccepted(std::uint16_t a_eventID) override;
		void OnDisabled() override { relockPending = false; }

	private:
		enum : std::uint16_t
		{
			kLock = PromptID::kLock,
			kGrapple = PromptID::kGrapple
		};

		using Clock = std::chrono::steady_clock;

		LockOn() = default;

		void ResolveGrapple();
		void SyncGrappleLockKey();
		static bool InGrapple(RE::PlayerCharacter* a_player, bool a_movable);
		void UpdateRelock(RE::PlayerCharacter* a_player, bool a_combat, bool a_locked, bool a_movable);

		PromptSlot lock{ this, kLock };
		PromptSlot grapple{ this, kGrapple };

		TDM_API::IVTDM1* tdm{ nullptr };
		std::int64_t tdmLockKey{ -1 };

		RE::TESQuest* grappleQuest{ nullptr };
		std::int32_t grappleKey{ -1 };
		bool grappleModifier{ false };

		Clock::time_point quietUntil{};
		Clock::time_point checkAt{};
		const char* checkWhat{ nullptr };

		bool relockPending{ false };
		bool sawGrapple{ false };
		Clock::time_point relockStart{};
		Clock::time_point relockStable{};
		Clock::time_point relockDeadline{};
		bool warnedKey{ false };
	};
}
