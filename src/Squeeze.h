#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// 비켜 지나가기: when a non-hostile humanoid blocks the way, a hold prompt lets the player squeeze
	// past. While it is held, the NPC being pressed against stops colliding with the player's
	// controller (its controller body's no-collision flag, the probe's (나), the user's pick after r8b),
	// plays the vanilla bump toward the side the player comes from, and the player plays EVG Animated
	// Traversal's Squeeze gesture. Everything is put back once the player is past. Needs EVG and Offset
	// Movement Animation; without either it is off (the user's D17). docs/038 "Stage 1".
	class Squeeze final : public Module
	{
	public:
		static Squeeze* GetSingleton();

		// At plugin load, before Open Animation Replacer reads its submods: builds CIGAR's gesture
		// submod from the player's own EVG Animated Traversal (the user's D17, 2026-09-29: EVG is an
		// optional integration, and without it Squeeze is off). CIGAR ships no clip.
		static void PrepareClip();

		const char* Name() const override { return "Squeeze"; }
		void OnGameLoaded() override;
		void Tick() override {}
		void FastTick() override;
		void OnAccepted(std::uint16_t) override {}
		void OnHold(std::uint16_t a_eventID, bool a_down) override;
		void OnDeclined(std::uint16_t a_eventID) override;
		void OnDisabled() override;

	private:
		enum : std::uint16_t
		{
			kSqueeze = PromptID::kSqueezePast
		};

		using Clock = std::chrono::steady_clock;

		Squeeze();

		// The NPC in the way, or null; a_why names what ruled the nearest one out.
		RE::Actor* Blocker(RE::PlayerCharacter* a_player, float& a_ahead, float& a_side, std::string& a_why) const;
		void Begin(RE::PlayerCharacter* a_player, RE::Actor* a_npc, float a_ahead, float a_side);
		// Puts the NPC's filter word back and reads it back; a_why goes to the log.
		void End(RE::PlayerCharacter* a_player, std::string_view a_why);
		void PlayGesture(RE::PlayerCharacter* a_player, float a_side);
		void StopGesture(RE::PlayerCharacter* a_player, std::string_view a_why);

		PromptSlot prompt{ this, kSqueeze };

		// Blocked detection (game thread).
		RE::NiPoint3 lastPos{};
		Clock::time_point lastTick{};
		Clock::time_point blockedSince{};
		bool blocked{ false };

		// The hold: the key is down (pressing) and, after the ring, active.
		bool pressing{ false };
		bool active{ false };
		Clock::time_point pressedAt{};

		// The NPC currently squeezed past (at most one).
		struct Pass
		{
			RE::ActorHandle npc;
			std::string name;
			RE::hkRefPtr<RE::hkpRigidBody> body;
			std::uint32_t oldFilter{ 0 };
			RE::NiPoint3 playerStart{};
			RE::NiPoint3 npcStart{};
			Clock::time_point start{};
			Clock::time_point lastProgress{};
			float bestPast{ -1.0e9f };
			float moved{ 0.0f };
		};
		std::optional<Pass> pass;
		RE::FormID passCell{ 0 };

		// The gesture (Offset Movement Animation); without it the feature is off.
		bool gpmaChecked{ false };
		bool gpmaInstalled{ false };
		bool gesturePlaying{ false };
		Clock::time_point gestureStart{};
		float gestureSeconds{ 0.0f };

		// One bump per NPC, then a cooldown (the user's rule, 2026-09-28).
		RE::FormID lastBumped{ 0 };
		Clock::time_point lastBumpAt{};

		bool restoreWarned{ false };

		// PrepareClip's result, read on load.
		static inline bool clipReady{ false };
		static inline std::string clipNote{ "not prepared" };
	};
}
