#include "Rest.h"

#include "Util.h"

namespace CIGAR
{
	namespace
	{
		// How far down the player must look for the ground prompts, in radians (Skyrim pitch is
		// positive looking down). About 35 degrees; my choice, logged in the gate for tuning.
		constexpr float kFloorPitch = 0.6f;
		// SI's IdleActions.t_threshold: seconds of standing still before the prompts appear.
		constexpr auto kReadyDelay = 1s;
		// Pass time. SI waits passtime_delay (5 s) before its prompt; the user wants it at once.
		constexpr auto kPassTimeText = "시간 보내기 (누르고 있기)"sv;
		// While the key is held the timescale climbs from x1 to x kPassTimeMax over kPassTimeRamp
		// (SI reaches its maximum gradually too). Both numbers are my choice, not SI's: SI's
		// max_timemult is 2.0, which barely moves the clock. At the vanilla timescale 20, x60 is
		// 20 game minutes per real second.
		constexpr float kPassTimeMax = 60.0f;
		constexpr float kPassTimeRamp = 3.0f;
		// The whole game also runs faster, up to x kGameSpeedMax on the same ramp, so NPCs visibly
		// hurry (the user's request, 2026-09-22). Game speed also speeds the clock, so the timescale
		// is divided by it and the clock still totals x kPassTimeMax. 3 is the top of the 2-3 range
		// offered to the user: above about 4 physics and pathing start to break.
		constexpr float kGameSpeedMax = 3.0f;
		// A timescale above this at load is reported: it may be an accelerated value that was saved.
		constexpr float kTimescaleSuspicious = 100.0f;
		// Stick or key input at least this strong (0-1) counts as wanting to move.
		constexpr float kMoveInput = 0.2f;
		// A pending enter waits this long for the third-person graph after a camera switch.
		constexpr auto kThirdPersonWait = 1s;
		// Animation events logged from entering until this long after getting up.
		constexpr auto kRecordAfterGetUp = 5s;
		constexpr int kRecordCap = 80;
		// SI waits for these tags to know the pose is reached (read from its DLL). In game (2026-09-22)
		// idleChairSitting also arrives for the wall and table leans, about 2-3 s after the enter event.
		constexpr auto kSatTag = "idleChairSitting"sv;
		constexpr auto kLayTag = "tailLayDown"sv;
		constexpr auto kConfirmWait = 6s;

		constexpr auto kSitEvent = "IdleSitCrossLeggedEnter"sv;
		// Sitting on an edge with the legs hanging, which SI picks after a ray scan (RayCollector).
		constexpr auto kLedgeEvent = "IdleSitLedgeEnter"sv;
		constexpr auto kLieEvent = "IdleLayDownEnter"sv;
		// Lean events SI sends (its LeanWall, LeanTable and LeanEdge); the rail has its own exit.
		// IdleWallLeanStart turns the actor around before leaning back, so the wall lean is offered only
		// facing a wall. The user chose that over a back-to-wall lean (2026-09-22).
		constexpr auto kLeanWallEvent = "IdleWallLeanStart"sv;
		constexpr auto kLeanTableEvent = "IdleLeanTableEnter"sv;
		constexpr auto kLeanRailEvent = "IdleRailLeanEnter"sv;
		constexpr auto kRailExitEvent = "IdleRailLeanExit"sv;
		constexpr auto kExitEvent = "IdleChairExitStart"sv;
		constexpr auto kStopEvent = "IdleStop"sv;
		constexpr auto kResetEvent = "IdleForceDefaultState"sv;

		// Ledge test, in game units; my choices, logged on every sit for tuning. A ledge is a drop
		// of at least kLedgeDrop within kLedgeProbes ahead, with nothing solid in the way at knee
		// height.
		constexpr float kLedgeDrop = 40.0f;
		constexpr std::array kLedgeProbes{ 25.0f, 40.0f, 55.0f };
		constexpr float kRayTop = 40.0f;
		constexpr float kRayDepth = 250.0f;
		constexpr float kKneeHeight = 25.0f;

		// The fraction along a_from -> a_to where the first solid is hit, or nullopt. Uses the
		// line-of-sight layer in the player's own collision group, so the player is not hit.
		std::optional<float> Cast(RE::PlayerCharacter* a_player, const RE::NiPoint3& a_from, const RE::NiPoint3& a_to)
		{
			auto* cell = a_player->GetParentCell();
			auto* world = cell ? cell->GetbhkWorld() : nullptr;
			if (!world) {
				return std::nullopt;
			}
			const float scale = RE::bhkWorld::GetWorldScale();
			RE::bhkPickData pick{};
			pick.rayInput.from = RE::hkVector4(a_from.x * scale, a_from.y * scale, a_from.z * scale, 0.0f);
			pick.rayInput.to = RE::hkVector4(a_to.x * scale, a_to.y * scale, a_to.z * scale, 0.0f);
			RE::CFilter filter{};
			a_player->GetCollisionFilterInfo(filter);
			pick.rayInput.filterInfo.filter =
				(filter.filter & 0xFFFF0000) | static_cast<std::uint32_t>(RE::COL_LAYER::kLOS);
			{
				RE::BSReadLockGuard lock(world->worldLock);
				world->PickObject(pick);
			}
			if (!pick.rayOutput.HasHit()) {
				return std::nullopt;
			}
			return pick.rayOutput.hitFraction;
		}

		// The ground height below a_at (searched from kRayTop above it to kRayDepth below), or nullopt.
		std::optional<float> GroundZ(RE::PlayerCharacter* a_player, RE::NiPoint3 a_at)
		{
			const RE::NiPoint3 from{ a_at.x, a_at.y, a_at.z + kRayTop };
			const RE::NiPoint3 to{ a_at.x, a_at.y, a_at.z - kRayDepth };
			const auto hit = Cast(a_player, from, to);
			if (!hit) {
				return std::nullopt;
			}
			return from.z + (to.z - from.z) * *hit;
		}

		// Whether the player faces a drop to sit on, with the scan written to a_scan for the log.
		bool LedgeAhead(RE::PlayerCharacter* a_player, std::string& a_scan)
		{
			const auto origin = a_player->GetPosition();
			const float yaw = a_player->GetAngleZ();
			const RE::NiPoint3 forward{ std::sin(yaw), std::cos(yaw), 0.0f };

			const auto ground = GroundZ(a_player, origin);
			if (!ground) {
				a_scan = "no ground under the player";
				return false;
			}
			const RE::NiPoint3 knee{ origin.x, origin.y, *ground + kKneeHeight };
			const float reach = kLedgeProbes.back();
			const auto wall = Cast(a_player, knee, knee + forward * reach);
			a_scan = std::format("groundZ={:.0f}", *ground);
			if (wall) {
				a_scan += std::format(" blocked at {:.0f}", *wall * reach);
				return false;
			}
			bool ledge = false;
			for (const float distance : kLedgeProbes) {
				const auto at = origin + forward * distance;
				const auto below = GroundZ(a_player, RE::NiPoint3{ at.x, at.y, *ground });
				const float drop = below ? *ground - *below : kRayDepth;
				a_scan += std::format(" drop@{:.0f}={:.0f}{}", distance, drop, below ? "" : "+");
				ledge = ledge || drop >= kLedgeDrop;
			}
			return ledge;
		}

		// Lean test, in game units; my choices, logged on every lean for tuning. In front: the first
		// surface found looking down at kLeanProbes ahead, kTableMin-kTableMax above the ground a
		// table, up to kRailMax a rail. A wall is hit within kWallReach in front at waist and chest
		// height.
		constexpr std::array kLeanProbes{ 25.0f, 35.0f, 45.0f };
		constexpr float kLeanTop = 140.0f;
		constexpr float kLeanBottom = 20.0f;
		constexpr float kTableMin = 60.0f;
		constexpr float kTableMax = 95.0f;
		constexpr float kRailMax = 125.0f;
		constexpr float kWallReach = 40.0f;
		constexpr float kWallWaist = 60.0f;
		constexpr float kWallChest = 100.0f;
		constexpr auto kLeanRescan = 250ms;

		enum class LeanSpot
		{
			kNone,
			kWall,
			kTable,
			kRail
		};

		LeanSpot ScanLean(RE::PlayerCharacter* a_player, std::string& a_scan)
		{
			const auto origin = a_player->GetPosition();
			const float yaw = a_player->GetAngleZ();
			const RE::NiPoint3 forward{ std::sin(yaw), std::cos(yaw), 0.0f };

			const auto ground = GroundZ(a_player, origin);
			if (!ground) {
				a_scan = "no ground under the player";
				return LeanSpot::kNone;
			}
			a_scan = std::format("groundZ={:.0f}", *ground);
			for (const float distance : kLeanProbes) {
				const auto at = origin + forward * distance;
				const RE::NiPoint3 from{ at.x, at.y, *ground + kLeanTop };
				const RE::NiPoint3 to{ at.x, at.y, *ground + kLeanBottom };
				const auto hit = Cast(a_player, from, to);
				if (!hit || *hit <= 0.0f) {
					a_scan += std::format(" front@{:.0f}={}", distance, hit ? "inside" : "none");
					continue;
				}
				const float height = from.z + (to.z - from.z) * *hit - *ground;
				a_scan += std::format(" front@{:.0f}={:.0f}", distance, height);
				if (height >= kTableMin && height < kTableMax) {
					return LeanSpot::kTable;
				}
				if (height >= kTableMax && height <= kRailMax) {
					return LeanSpot::kRail;
				}
			}
			const RE::NiPoint3 waist{ origin.x, origin.y, *ground + kWallWaist };
			const RE::NiPoint3 chest{ origin.x, origin.y, *ground + kWallChest };
			const auto at = [](const std::optional<float>& a_hit) {
				return a_hit ? std::format("{:.0f}", *a_hit * kWallReach) : "none"s;
			};
			const auto reach = forward * kWallReach;
			const auto waistHit = Cast(a_player, waist, waist + reach);
			const auto chestHit = Cast(a_player, chest, chest + reach);
			a_scan += std::format(" wall waist={} chest={}", at(waistHit), at(chestHit));
			return waistHit && chestHit ? LeanSpot::kWall : LeanSpot::kNone;
		}

		// Whether a keyboard key (DirectInput scan code, as SKSE and SkyPrompt use) is held now.
		// Read through Win32: CommonLib's keyboard device class does not link into this plugin.
		bool KeyDown(std::uint32_t a_scanCode)
		{
			const UINT scan = a_scanCode >= 0x80 ? (0xE000 | (a_scanCode & 0x7F)) : a_scanCode;
			const UINT vk = ::MapVirtualKeyW(scan, MAPVK_VSC_TO_VK_EX);
			return vk != 0 && (::GetAsyncKeyState(static_cast<int>(vk)) & 0x8000) != 0;
		}

		bool GraphBool(RE::PlayerCharacter* a_player, const char* a_name)
		{
			bool value = false;
			return a_player->GetGraphVariableBool(a_name, value) && value;
		}

		bool Notify(RE::PlayerCharacter* a_player, std::string_view a_event)
		{
			return a_player->NotifyAnimationGraph(RE::BSFixedString{ a_event });
		}
	}

	Rest::Rest()
	{
		sit.SetPromptType(SkyPromptAPI::kHold);
		lie.SetPromptType(SkyPromptAPI::kHold);
		lean.SetPromptType(SkyPromptAPI::kHold);
		// The clock runs fast while the key is down (hold mode reports down and up). HoldAndKeep
		// draws SkyPrompt's ring, and the text and progress show the multiplier live.
		passTime.SetHoldMode(true);
		passTime.SetPromptType(SkyPromptAPI::kHoldAndKeep);
	}

	Rest* Rest::GetSingleton()
	{
		static Rest singleton;
		return &singleton;
	}

	const char* Rest::PoseName(Pose a_pose)
	{
		switch (a_pose) {
		case Pose::kSitting:
			return "sitting";
		case Pose::kLying:
			return "lying";
		case Pose::kLeanWall:
			return "leaning on a wall";
		case Pose::kLeanTable:
			return "leaning on a table";
		case Pose::kLeanRail:
			return "leaning on a rail";
		default:
			return "standing";
		}
	}

	void Rest::OnGameLoaded()
	{
		sit.Reset();
		lie.Reset();
		lean.Reset();
		passTime.Reset();
		lastGate.clear();
		// The loaded save carries its own timescale; nothing of ours is left to restore. Game speed is
		// not saved, so one we set before the load must still be put back.
		passHolding = false;
		passBase = 0.0f;
		if (passSpeedSet > 0.0f) {
			if (auto* timer = RE::BSTimer::GetSingleton()) {
				timer->SetGlobalTimeMultiplier(1.0f, true);
			}
			Log("game speed x{:.1f} from pass time reset to x1 at load", passSpeedSet);
			passSpeedSet = 0.0f;
		}
		if (const auto* calendar = RE::Calendar::GetSingleton(); calendar && calendar->timeScale) {
			const float timescale = calendar->timeScale->value;
			if (timescale > kTimescaleSuspicious) {
				Log("WARN timescale {:.0f} at load; an accelerated pass time may have been saved", timescale);
				Util::Notify(std::format("CIGAR: 시간 배율 비정상 ({:.0f}). 로그 확인", timescale));
			}
		}
		leanFound = Pose::kStanding;
		leanShown = Pose::kStanding;
		pose = Pose::kStanding;
		pendingPose = Pose::kStanding;
		readySince = Clock::now();
		confirmReported = false;
		restConfirmed = false;
		{
			std::scoped_lock lock(recordLock);
			recorded.clear();
			recordUntil = {};
			recordedCount = 0;
		}
		if (auto* player = Util::Player()) {
			ListenToPlayer(player);
		}
		Util::WarnIfSIModuleOn("IdleActions.enabled", "/MCP/modules/IdleActions/enabled");
		Log("ready; floor pitch>={:.2f} rad, still for {}s", kFloorPitch,
			std::chrono::duration_cast<std::chrono::seconds>(kReadyDelay).count());
	}

	void Rest::Tick()
	{
		Util::WarnIfSIModuleOn("IdleActions.enabled", "/MCP/modules/IdleActions/enabled");
	}

	void Rest::ListenToPlayer(RE::PlayerCharacter* a_player)
	{
		// The graph is rebuilt when the player's 3D is, so the sink is re-added before each rest.
		a_player->RemoveAnimationGraphEventSink(this);
		const bool added = a_player->AddAnimationGraphEventSink(this);
		if (!added) {
			Log("WARN animation event sink not added; rest confirmation will not be logged");
		}
	}

	void Rest::FastTick()
	{
		FlushRecorded();
		auto* player = Util::Player();
		if (!player) {
			return;
		}

		if (pendingPose != Pose::kStanding) {
			const auto wanted = pendingPose;
			if (SendEnter(player, wanted)) {
				pendingPose = Pose::kStanding;
			} else if (Clock::now() >= pendingUntil) {
				pendingPose = Pose::kStanding;
				Log("{} failed: the graph refused the enter event after the camera switch", PoseName(wanted));
				Util::Notify("CIGAR: 앉기/눕기 실패. 로그 확인");
			}
		}

		if (pose == Pose::kStanding) {
			StandingTick(player);
		} else {
			RestingTick(player);
		}
	}

	void Rest::StandingTick(RE::PlayerCharacter* a_player)
	{
		const auto now = Clock::now();
		auto* ui = RE::UI::GetSingleton();
		const auto* controls = RE::ControlMap::GetSingleton();
		const auto* state = a_player->AsActorState();

		const float pitch = a_player->GetAngleX();
		const bool floor = pitch >= kFloorPitch;
		const bool moving = a_player->IsMoving();
		const bool combat = a_player->IsInCombat();
		const bool drawn = state && state->IsWeaponDrawn();
		const bool seated = state && state->GetSitSleepState() != RE::SIT_SLEEP_STATE::kNormal;
		const bool swimming = state && state->IsSwimming();
		const bool sneaking = a_player->IsSneaking();
		const bool airborne = a_player->IsInMidair();
		const bool mounted = a_player->IsOnMount();
		const bool driven = GraphBool(a_player, "bAnimationDriven");
		const bool controlsOn = controls && controls->IsMovementControlsEnabled() && controls->IsLookingControlsEnabled();
		const bool menu = !ui || ui->IsApplicationMenuOpen() || ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME);
		const bool pending = pendingPose != Pose::kStanding;

		const bool can = !moving && !combat && !drawn && !seated && !swimming && !sneaking && !airborne &&
		                 !mounted && !driven && controlsOn && !menu && !pending && !a_player->IsDead();
		if (!can) {
			readySince = now;
		}
		const auto still = now - readySince;
		const bool ready = can && still >= kReadyDelay;
		const bool available = ready && floor;

		if (!ready) {
			leanFound = Pose::kStanding;
		} else if (now - leanScannedAt >= kLeanRescan) {
			leanScannedAt = now;
			switch (ScanLean(a_player, leanScan)) {
			case LeanSpot::kWall:
				leanFound = Pose::kLeanWall;
				break;
			case LeanSpot::kTable:
				leanFound = Pose::kLeanTable;
				break;
			case LeanSpot::kRail:
				leanFound = Pose::kLeanRail;
				break;
			default:
				leanFound = Pose::kStanding;
				break;
			}
		}

		LogGate(std::format(
			"pose=standing pitch={:.2f} floor={} lean={} moving={} combat={} drawn={} seated={} swim={} sneak={} "
			"air={} mount={} animDriven={} controls={} menu={} pending={} ready={}",
			pitch, floor, leanFound == Pose::kStanding ? "none" : PoseName(leanFound), moving, combat, drawn, seated,
			swimming, sneaking, airborne, mounted, driven, controlsOn, menu, pending, ready));

		passTime.Update(false, {});
		sit.Update(available, [] { return "앉기 (길게)"s; });
		lie.Update(available, [] { return "눕기 (길게)"s; });
		if (leanShown != leanFound) {
			// The text names the surface, so a different surface is a new prompt.
			lean.Withdraw();
			leanShown = leanFound;
		}
		lean.Update(leanFound != Pose::kStanding, [this] {
			switch (leanShown) {
			case Pose::kLeanTable:
				return "탁자에 기대기 (길게)"s;
			case Pose::kLeanRail:
				return "난간에 기대기 (길게)"s;
			default:
				return "벽에 기대기 (길게)"s;
			}
		});
	}

	void Rest::RestingTick(RE::PlayerCharacter* a_player)
	{
		const auto now = Clock::now();
		const auto* state = a_player->AsActorState();
		const auto* controls = RE::PlayerControls::GetSingleton();
		// Movement input, not IsMoving(): the wall lean reports moving=true for its whole loop.
		const bool moveInput = controls && (controls->data.moveInputVec.Length() >= kMoveInput || controls->data.autoMove);
		const bool combat = a_player->IsInCombat();
		const bool drawn = state && state->IsWeaponDrawn();
		const bool dead = a_player->IsDead() || (state && state->IsBleedingOut());
		const bool driven = GraphBool(a_player, "bAnimationDriven");
		const bool seated = state && state->GetSitSleepState() != RE::SIT_SLEEP_STATE::kNormal;
		const auto since = now - poseSince;
		const bool confirmed = restConfirmed.load();
		// Until the pose is reached the enter animation is still playing and an exit event could be
		// refused or cut it short, so a movement exit waits for it (or for kConfirmWait).
		const bool settled = confirmed || since >= kConfirmWait;
		const auto tag = pose == Pose::kLying ? kLayTag : kSatTag;
		seenSeated = seenSeated || seated;

		LogGate(std::format(
			"pose={} for={:.0f}s confirmed={} settled={} moveInput={} exitQueued={} passTime={} combat={} drawn={} "
			"seated={} animDriven={}",
			PoseName(pose), std::chrono::duration<float>(since).count(), confirmed, settled, moveInput, exitQueued,
			passHolding, combat, drawn, seated, driven));

		if (!confirmReported && confirmed) {
			confirmReported = true;
			Log("{} reached ({} seen)", PoseName(pose), tag);
		} else if (!confirmReported && since >= kConfirmWait) {
			confirmReported = true;
			Log("WARN {} not confirmed: no '{}' event within {}s; see the recorded events above", PoseName(pose), tag,
				std::chrono::duration_cast<std::chrono::seconds>(kConfirmWait).count());
		}

		sit.Update(false, {});
		lie.Update(false, {});
		lean.Update(false, {});

		if (combat) {
			GetUp("combat started");
			return;
		}
		if (dead) {
			StopPassTime("dead or bleeding out");
			passTime.Withdraw();
			pose = Pose::kStanding;
			Log("rest ended: dead or bleeding out");
			return;
		}
		if (settled && ((seenSeated && !seated) || drawn)) {
			// The game already stood the player up (a jump exits at once); send nothing.
			StopPassTime("rest ended by the game");
			passTime.Withdraw();
			pose = Pose::kStanding;
			readySince = now;
			StartRecording(kRecordAfterGetUp);
			Log("rest ended by the game: seated={} (was on: {}) drawn={}", seated, seenSeated, drawn);
			return;
		}
		if (moveInput && !exitQueued) {
			exitQueued = true;
			Log("movement input while {}; getting up{}", PoseName(pose), settled ? "" : " once the pose is reached");
		}
		if (exitQueued && settled) {
			GetUp("movement input");
			return;
		}

		// Offered as soon as the pose is entered (the user's call, 2026-09-22); a double press hides it.
		passTime.Update(!exitQueued, [] { return std::string{ kPassTimeText }; });
		PassTimeTick();
	}

	void Rest::OnAccepted(std::uint16_t a_eventID)
	{
		switch (a_eventID) {
		case kSit:
			if (pose == Pose::kStanding) {
				Enter(Pose::kSitting);
			}
			break;
		case kLie:
			if (pose == Pose::kStanding) {
				Enter(Pose::kLying);
			}
			break;
		case kLean:
			if (pose == Pose::kStanding && leanShown != Pose::kStanding) {
				Log("lean scan: {} -> {}", leanScan, PoseName(leanShown));
				Enter(leanShown);
			}
			break;
		default:
			break;
		}
	}

	void Rest::Enter(Pose a_pose)
	{
		auto* player = Util::Player();
		if (!player) {
			return;
		}
		sit.Withdraw();
		lie.Withdraw();
		lean.Withdraw();
		leanFound = Pose::kStanding;
		ListenToPlayer(player);
		restConfirmed = false;
		confirmReported = false;
		exitQueued = false;
		seenSeated = false;
		StartRecording(Clock::duration::max());

		auto* camera = RE::PlayerCamera::GetSingleton();
		if (camera && camera->IsInFirstPerson()) {
			// The idles live in the third-person graph.
			camera->ForceThirdPerson();
			Log("switched to third person for {}", PoseName(a_pose));
		}
		if (!SendEnter(player, a_pose)) {
			pendingPose = a_pose;
			pendingUntil = Clock::now() + kThirdPersonWait;
			Log("{}: enter event refused, retrying for {}ms", PoseName(a_pose),
				std::chrono::duration_cast<std::chrono::milliseconds>(kThirdPersonWait).count());
		}
	}

	bool Rest::SendEnter(RE::PlayerCharacter* a_player, Pose a_pose)
	{
		auto event = kLieEvent;
		switch (a_pose) {
		case Pose::kSitting:
			event = kSitEvent;
			break;
		case Pose::kLeanWall:
			event = kLeanWallEvent;
			break;
		case Pose::kLeanTable:
			event = kLeanTableEvent;
			break;
		case Pose::kLeanRail:
			event = kLeanRailEvent;
			break;
		default:
			break;
		}
		if (a_pose == Pose::kSitting) {
			std::string scan;
			const bool ledge = LedgeAhead(a_player, scan);
			Log("ledge scan: {} -> {}", scan, ledge ? "ledge" : "floor");
			if (ledge) {
				if (Notify(a_player, kLedgeEvent)) {
					event = kLedgeEvent;
				} else {
					Log("{} refused; sitting on the floor instead", kLedgeEvent);
				}
			}
		}
		if (event != kLedgeEvent && !Notify(a_player, event)) {
			return false;
		}
		pose = a_pose;
		poseSince = Clock::now();
		Log("{}: sent {} (accepted)", PoseName(a_pose), event);
		return true;
	}

	void Rest::GetUp(std::string_view a_reason)
	{
		auto* player = Util::Player();
		const auto was = pose;
		StopPassTime(a_reason);
		passTime.Withdraw();
		pose = Pose::kStanding;
		readySince = Clock::now();
		exitQueued = false;
		StartRecording(kRecordAfterGetUp);
		if (!player) {
			return;
		}
		// SI's GetUp sends IdleRailLeanExit for the rail and IdleChairExitStart otherwise.
		const auto exitEvent = was == Pose::kLeanRail ? kRailExitEvent : kExitEvent;
		const bool exit = Notify(player, exitEvent);
		const bool stop = !exit && Notify(player, kStopEvent);
		const bool reset = !exit && !stop && Notify(player, kResetEvent);
		Log("get up from {} ({}): {}={} {}={} {}={}", PoseName(was), a_reason, exitEvent, exit, kStopEvent, stop,
			kResetEvent, reset);
		if (!exit && !stop && !reset) {
			Util::Notify("CIGAR: 일어나기 실패. 로그 확인");
		}
	}

	void Rest::OnDisabled()
	{
		StopPassTime("module switched off");
		pendingPose = Pose::kStanding;
		if (pose != Pose::kStanding) {
			GetUp("module switched off");
		}
	}

	void Rest::OnHold(std::uint16_t a_eventID, bool a_down)
	{
		if (a_eventID != kPassTime) {
			return;
		}
		if (!a_down) {
			StopPassTime("key released");
			return;
		}
		if (pose == Pose::kStanding || passHolding) {
			return;
		}
		passHolding = true;
		passHeldSince = Clock::now();
	}

	void Rest::PassTimeTick()
	{
		if (!passHolding) {
			return;
		}
		// Key up may not be reported for every prompt type; the key's own state is the backstop.
		if (const auto key = passTime.Key(); key != 0 && !KeyDown(key) && Clock::now() - passHeldSince > 300ms) {
			StopPassTime("key no longer down");
			return;
		}
		auto* calendar = RE::Calendar::GetSingleton();
		auto* timescale = calendar ? calendar->timeScale : nullptr;
		if (!timescale) {
			passHolding = false;
			Log("WARN pass time: the calendar timescale global is unavailable");
			Util::Notify("CIGAR: 시간 보내기 실패. 로그 확인");
			return;
		}
		auto* timer = RE::BSTimer::GetSingleton();
		if (passBase <= 0.0f) {
			passBase = timescale->value;
			passHoursAtStart = calendar->GetHoursPassed();
			const float speed = RE::BSTimer::QGlobalTimeMultiplier();
			// Game speed is left alone when something else already changed it (Surrender's slow motion).
			const bool ownSpeed = timer && std::abs(speed - 1.0f) < 0.01f;
			passSpeedSet = ownSpeed ? 1.0f : 0.0f;
			Log("pass time: key held, timescale {:.1f}, clock rising to x{:.0f} and game speed to x{:.0f} over {:.0f}s{}",
				passBase, kPassTimeMax, kGameSpeedMax, kPassTimeRamp,
				ownSpeed ? "" : std::format(" (game speed x{:.2f} set elsewhere; left alone)", speed));
		}
		const float held = std::chrono::duration<float>(Clock::now() - passHeldSince).count();
		const float ramp = std::min(1.0f, held / kPassTimeRamp);
		const float multiplier = 1.0f + (kPassTimeMax - 1.0f) * ramp;
		float speed = 1.0f;
		if (passSpeedSet > 0.0f && timer) {
			speed = 1.0f + (kGameSpeedMax - 1.0f) * ramp;
			if (std::abs(speed - passSpeedSet) > 0.01f) {
				timer->SetGlobalTimeMultiplier(speed, true);
				passSpeedSet = speed;
			}
		}
		timescale->value = passBase * multiplier / speed;
		passTime.SetLive(std::format("시간 보내기 ×{:.0f}", multiplier), ramp);
	}

	void Rest::StopPassTime(std::string_view a_reason)
	{
		const bool wasHolding = passHolding;
		passHolding = false;
		passTime.SetLive(std::string{ kPassTimeText }, 0.0f);
		if (passSpeedSet > 0.0f) {
			const float now = RE::BSTimer::QGlobalTimeMultiplier();
			if (std::abs(now - passSpeedSet) > 0.01f) {
				Log("pass time: game speed is x{:.2f}, changed elsewhere; left as is", now);
			} else if (auto* timer = RE::BSTimer::GetSingleton()) {
				timer->SetGlobalTimeMultiplier(1.0f, true);
			}
			passSpeedSet = 0.0f;
		}
		if (passBase <= 0.0f) {
			if (wasHolding) {
				Log("pass time: key up before the clock sped up ({})", a_reason);
			}
			return;
		}
		auto* calendar = RE::Calendar::GetSingleton();
		if (calendar && calendar->timeScale) {
			calendar->timeScale->value = passBase;
			Log("pass time stopped ({}): held {:.1f}s, {:.1f} game hours passed, timescale back to {:.1f}", a_reason,
				std::chrono::duration<float>(Clock::now() - passHeldSince).count(),
				calendar->GetHoursPassed() - passHoursAtStart, passBase);
		}
		passBase = 0.0f;
	}

	void Rest::BeforeSave()
	{
		if (passBase <= 0.0f) {
			return;
		}
		auto* calendar = RE::Calendar::GetSingleton();
		if (calendar && calendar->timeScale) {
			calendar->timeScale->value = passBase;
			Log("save while passing time: timescale {:.1f} restored before writing", passBase);
		}
		// Still held: the next tick takes this as the base and speeds up again.
		passBase = 0.0f;
	}

	void Rest::StartRecording(Clock::duration a_for)
	{
		std::scoped_lock lock(recordLock);
		const auto now = Clock::now();
		recordUntil = a_for == Clock::duration::max() ? Clock::time_point::max() : now + a_for;
		recordedCount = 0;
	}

	void Rest::FlushRecorded()
	{
		std::vector<std::string> lines;
		{
			std::scoped_lock lock(recordLock);
			lines.swap(recorded);
		}
		for (const auto& line : lines) {
			Log("anim event {}", line);
		}
	}

	RE::BSEventNotifyControl Rest::ProcessEvent(
		const RE::BSAnimationGraphEvent* a_event,
		RE::BSTEventSource<RE::BSAnimationGraphEvent>*)
	{
		if (!a_event || !a_event->holder || !a_event->holder->IsPlayerRef()) {
			return RE::BSEventNotifyControl::kContinue;
		}
		const std::string_view tag{ a_event->tag.c_str() };
		if (Util::EqualsNoCase(tag, kSatTag) || Util::EqualsNoCase(tag, kLayTag)) {
			restConfirmed = true;
		}
		std::scoped_lock lock(recordLock);
		if (Clock::now() < recordUntil && recordedCount < kRecordCap) {
			++recordedCount;
			const std::string_view payload{ a_event->payload.c_str() };
			recorded.push_back(payload.empty() ? std::string{ tag } : std::format("{} ({})", tag, payload));
			if (recordedCount == kRecordCap) {
				recorded.push_back(std::format("... cap of {} reached", kRecordCap));
			}
		}
		return RE::BSEventNotifyControl::kContinue;
	}
}
