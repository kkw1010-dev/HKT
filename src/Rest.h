#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// SI IdleActions' ground and lean actions. Looking at the floor while standing still offers
	// 앉기 and 눕기; a wall, table or rail in front offers 기대기; a fire in front offers 손 녹이기
	// (the fires are Survival Mode's heat-source list). Movement input gets the player up.
	// While resting, 시간 보내기 speeds the game clock for as long as its key is held (SI's pass
	// time). Uses the vanilla idle events SI sends.
	class Rest final :
		public Module,
		public RE::BSTEventSink<RE::BSAnimationGraphEvent>
	{
	public:
		static Rest* GetSingleton();

		const char* Name() const override { return "Rest"; }
		void OnGameLoaded() override;
		void Tick() override;
		void FastTick() override;
		void OnAccepted(std::uint16_t a_eventID) override;
		void OnDisabled() override;
		void OnHold(std::uint16_t a_eventID, bool a_down) override;
		// SKSE kSaveGame, before the file is written: puts the real timescale back so an
		// accelerated one is never saved.
		void BeforeSave();

		// True while CIGAR has the player sitting, lying or leaning (game thread).
		bool Resting() const { return pose != Pose::kStanding; }

		// Records the player's animation events around a rest, for the log (any thread).
		RE::BSEventNotifyControl ProcessEvent(
			const RE::BSAnimationGraphEvent* a_event,
			RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source) override;

	private:
		Rest();

		enum : std::uint16_t
		{
			kSit = PromptID::kSit,
			kLie = PromptID::kLieDown,
			kLean = PromptID::kLean,
			kPassTime = PromptID::kPassTime,
			kWarm = PromptID::kWarmHands
		};

		enum class Pose
		{
			kStanding,
			kSitting,
			kLying,
			kLeanWall,  // facing the wall: the enter animation turns the player around
			kLeanTable,
			kLeanRail,
			kWarmStanding,  // a fire at chest height (a brazier, a forge)
			kWarmCrouched   // a fire on the ground (a campfire)
		};

		using Clock = std::chrono::steady_clock;

		static const char* PoseName(Pose a_pose);
		static bool IsWarm(Pose a_pose) { return a_pose == Pose::kWarmStanding || a_pose == Pose::kWarmCrouched; }
		static bool IsLean(Pose a_pose)
		{
			return a_pose == Pose::kLeanWall || a_pose == Pose::kLeanTable || a_pose == Pose::kLeanRail;
		}
		void Enter(Pose a_pose);
		bool SendEnter(RE::PlayerCharacter* a_player, Pose a_pose);
		void GetUp(std::string_view a_reason);
		void StandingTick(RE::PlayerCharacter* a_player);
		// The warm-hands pose for the closest fire in reach and in front, or kStanding.
		Pose ScanFire(RE::PlayerCharacter* a_player);
		void RestingTick(RE::PlayerCharacter* a_player);
		void ListenToPlayer(RE::PlayerCharacter* a_player);
		void StartRecording(Clock::duration a_for);
		void FlushRecorded();
		void PassTimeTick();
		void StopPassTime(std::string_view a_reason);

		PromptSlot sit{ this, kSit };
		PromptSlot lie{ this, kLie };
		PromptSlot lean{ this, kLean };
		PromptSlot passTime{ this, kPassTime };
		PromptSlot warm{ this, kWarm };

		Pose pose{ Pose::kStanding };
		Clock::time_point poseSince{};
		Clock::time_point readySince{};
		// The lean the last scan found (kStanding for none), rescanned while the player stands ready.
		Pose leanFound{ Pose::kStanding };
		Pose leanShown{ Pose::kStanding };
		std::string leanScan;
		Clock::time_point leanScannedAt{};
		// Survival Mode's heat sources (Survival_WarmUpObjectsList), resolved at load; null without it.
		RE::BGSListForm* fires{ nullptr };
		Pose warmFound{ Pose::kStanding };
		std::string warmScan;
		// The closest fire-list reference that was turned down, logged when it changes.
		RE::FormID fireMissLogged{ 0 };
		// An enter request waiting for the third-person graph after a camera switch.
		Pose pendingPose{ Pose::kStanding };
		Clock::time_point pendingUntil{};
		bool confirmReported{ false };
		// Movement input arrived; the player gets up once the pose is reached.
		bool exitQueued{ false };
		// The game's sit state was on during this rest; it going off means the game stood the
		// player up (a jump, a drawn weapon, a script).
		bool seenSeated{ false };

		// Pass time: the key is down, since when, and the timescale to restore (0 while the clock
		// runs at its own speed).
		bool passHolding{ false };
		Clock::time_point passHeldSince{};
		float passBase{ 0.0f };
		float passHoursAtStart{ 0.0f };
		// Game speed (the global time multiplier) CIGAR set for pass time, 0 when it owns none, and
		// the ceiling read from the control panel when the key went down.
		float passSpeedSet{ 0.0f };
		float passSpeedMax{ 1.0f };

		// Animation events arrive on animation threads; FastTick() writes them to the log.
		std::mutex recordLock;
		std::vector<std::string> recorded;
		Clock::time_point recordUntil{};
		int recordedCount{ 0 };
		std::atomic_bool restConfirmed{ false };
		// A tag that means the player's idle ended (IdleStop, tailMTIdle, tailMTLocomotion) arrived
		// during the rest; the tag, for the log.
		std::atomic_bool idleEnded{ false };
		std::string idleEndedTag;
	};
}
