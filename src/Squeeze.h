#pragma once

#include "Module.h"
#include "Prompt.h"

#include <deque>

namespace CIGAR
{
	// 비켜 지나가기: when a non-hostile humanoid blocks the way, a prompt lets the player squeeze past,
	// from the press and for as long as it is held. The player's controller takes that NPC's collision
	// system group for the pass (way (A), the user's pick after r11), so the two do not collide while both
	// keep the floor; a cloth sound plays and the player plays EVG Animated Traversal's Squeeze gesture. Everything is put back once the player is past. Needs EVG and Offset
	// Movement Animation; without either it is off (the user's D17). docs/038 "Stage 1".
	class Squeeze final : public Module
	{
	public:
		static Squeeze* GetSingleton();

		// At plugin load, before Open Animation Replacer reads its submods: builds CIGAR's gesture
		// submod from the player's own EVG Animated Traversal (the user's D17, 2026-09-29: EVG is an
		// optional integration, and without it Squeeze is off). CIGAR ships no clip.
		static void PrepareClip();

		// Author-build probe (the user, r11): after the player is through, move the NPC aside a little, so the
		// verdict can be read from the log (how far aside, and z). The NPC keeps its collision in (A).
		enum class Nudge : int
		{
			kNone = 0,
			kSlide8 = 1,
			kSlide15 = 2,
			kBump = 3
		};
		static void SetNudge(Nudge a_nudge);
		static Nudge GetNudge() { return static_cast<Nudge>(nudge.load()); }

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
		void StartNudge(RE::PlayerCharacter* a_player, RE::Actor* a_npc, float a_side);
		void NudgeTick(RE::Actor* a_npc);
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
			// The player's filter words switched to the NPC's group (each once), with their words before;
			// the phantom and the body are kept alive while switched.
			struct Switched
			{
				RE::CFilter* filter;
				std::uint32_t old;
				const char* what;
			};
			std::vector<Switched> switched;
			RE::hkRefPtr<RE::hkpShapePhantom> phantom;
			RE::hkRefPtr<RE::hkpRigidBody> playerBody;
			// The nudge probe.
			bool nudging{ false };
			float nudgeLeft{ 0.0f };
			RE::NiPoint3 nudgeDir{};
			RE::NiPoint3 nudgeFrom{};
			Clock::time_point nudgeAt{};
			float stuck{ 0.0f };
			int selfBumps{ 0 };
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
		Clock::time_point popWatchUntil{};
		RE::NiPoint3 popStart{};
		static inline std::atomic<int> nudge{ 1 };  // slide 8: the user's pick in r12

		// PrepareClip's result, read on load.
		static inline bool clipReady{ false };
		static inline std::string clipNote{ "not prepared" };
	};
}
