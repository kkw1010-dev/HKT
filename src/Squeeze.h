#pragma once

#include "Module.h"
#include "Prompt.h"

#include <deque>

namespace CIGAR
{
	// 비켜 지나가기: when a non-hostile humanoid blocks the way, a hold prompt lets the player squeeze
	// past. While it is held, the NPC being pressed against stops colliding with the player's
	// controller (its controller body's no-collision flag, the probe's (나), the user's pick after r8b),
	// a dull cloth sound plays and the player plays EVG Animated Traversal's Squeeze gesture. Standing
	// NPCs only, no bump, and a fall guard (r10: a moving NPC without collision drops through the floor). Everything is put back once the player is past. Needs EVG and Offset
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

		// The nearest actor ahead (within 150, in the corridor), whether it is in the block box and why it
		// is refused, for the gate and its trace line.
		struct Near
		{
			std::string name;
			float ahead{ 0.0f };
			float side{ 0.0f };
			float dz{ 0.0f };
			bool inBox{ false };
			std::string why;
		};
		RE::Actor* Nearest(RE::PlayerCharacter* a_player, Near& a_near) const;
		void Begin(RE::PlayerCharacter* a_player, RE::Actor* a_npc, float a_ahead, float a_side);
		// Puts the NPC's filter word back and reads it back; a_why goes to the log.
		void End(RE::PlayerCharacter* a_player, std::string_view a_why);
		void PlayGesture(RE::PlayerCharacter* a_player, float a_side);
		void StopGesture(RE::PlayerCharacter* a_player, std::string_view a_why);

		PromptSlot prompt{ this, kSqueeze };

		// Blocked detection (game thread).
		std::deque<std::pair<Clock::time_point, RE::NiPoint3>> trail;
		RE::FormID contactID{ 0 };
		RE::FormID failedID{ 0 };  // the last NPC Begin could not squeeze, logged once
		Clock::time_point contactSince{};
		Clock::time_point shownUntil{};
		std::string lastTrace;

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

		bool restoreWarned{ false };

		// PrepareClip's result, read on load.
		static inline bool clipReady{ false };
		static inline std::string clipNote{ "not prepared" };
	};
}
