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
		// The enter animation plays before 일어나기 is offered, so a hold cannot cut it short.
		constexpr auto kGetUpDelay = 2s;
		// A pending enter waits this long for the third-person graph after a camera switch.
		constexpr auto kThirdPersonWait = 1s;
		// Animation events logged from entering until this long after getting up.
		constexpr auto kRecordAfterGetUp = 5s;
		constexpr int kRecordCap = 80;
		// SI waits for these tags to know the pose is reached (read from its DLL).
		constexpr auto kSatTag = "idleChairSitting"sv;
		constexpr auto kLayTag = "tailLayDown"sv;
		constexpr auto kConfirmWait = 6s;

		constexpr auto kSitEvent = "IdleSitCrossLeggedEnter"sv;
		constexpr auto kLieEvent = "IdleLayDownEnter"sv;
		constexpr auto kExitEvent = "IdleChairExitStart"sv;
		constexpr auto kStopEvent = "IdleStop"sv;
		constexpr auto kResetEvent = "IdleForceDefaultState"sv;

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
		getUp.SetPromptType(SkyPromptAPI::kHold);
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
		default:
			return "standing";
		}
	}

	void Rest::OnGameLoaded()
	{
		sit.Reset();
		lie.Reset();
		getUp.Reset();
		lastGate.clear();
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

		const bool can = floor && !moving && !combat && !drawn && !seated && !swimming && !sneaking && !airborne &&
		                 !mounted && !driven && controlsOn && !menu && !pending && !a_player->IsDead();
		if (!can) {
			readySince = now;
		}
		const auto still = now - readySince;
		const bool available = can && still >= kReadyDelay;

		LogGate(std::format(
			"pose=standing pitch={:.2f} floor={} moving={} combat={} drawn={} seated={} swim={} sneak={} air={} "
			"mount={} animDriven={} controls={} menu={} pending={} still={:.1f}s",
			pitch, floor, moving, combat, drawn, seated, swimming, sneaking, airborne, mounted, driven, controlsOn,
			menu, pending, std::chrono::duration<float>(still).count()));

		getUp.Update(false, {});
		sit.Update(available, [] { return "앉기 (길게)"s; });
		lie.Update(available, [] { return "눕기 (길게)"s; });
	}

	void Rest::RestingTick(RE::PlayerCharacter* a_player)
	{
		const auto now = Clock::now();
		const auto* state = a_player->AsActorState();
		const bool moving = a_player->IsMoving();
		const bool combat = a_player->IsInCombat();
		const bool drawn = state && state->IsWeaponDrawn();
		const bool dead = a_player->IsDead() || (state && state->IsBleedingOut());
		const bool driven = GraphBool(a_player, "bAnimationDriven");
		const auto since = now - poseSince;

		LogGate(std::format("pose={} for={:.0f}s confirmed={} moving={} combat={} drawn={} animDriven={}",
			PoseName(pose), std::chrono::duration<float>(since).count(), restConfirmed.load(), moving, combat,
			drawn, driven));

		if (!confirmReported && restConfirmed) {
			confirmReported = true;
			Log("{} reached ({} seen)", PoseName(pose), pose == Pose::kSitting ? kSatTag : kLayTag);
		} else if (!confirmReported && since >= kConfirmWait) {
			confirmReported = true;
			Log("WARN {} not confirmed: no '{}' event within {}s; see the recorded events above", PoseName(pose),
				pose == Pose::kSitting ? kSatTag : kLayTag,
				std::chrono::duration_cast<std::chrono::seconds>(kConfirmWait).count());
		}

		if (combat) {
			GetUp("combat started");
			return;
		}
		if (dead) {
			pose = Pose::kStanding;
			Log("rest ended: dead or bleeding out");
			return;
		}
		if (moving || drawn) {
			// Something else stood the player up; stop offering 일어나기.
			pose = Pose::kStanding;
			readySince = now;
			StartRecording(kRecordAfterGetUp);
			Log("rest ended by the game: moving={} drawn={}", moving, drawn);
			return;
		}

		sit.Update(false, {});
		lie.Update(false, {});
		getUp.Update(since >= kGetUpDelay, [] { return "일어나기 (길게)"s; });
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
		case kGetUp:
			if (pose != Pose::kStanding) {
				GetUp("prompt");
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
		ListenToPlayer(player);
		restConfirmed = false;
		confirmReported = false;
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
		const auto event = a_pose == Pose::kSitting ? kSitEvent : kLieEvent;
		if (!Notify(a_player, event)) {
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
		pose = Pose::kStanding;
		readySince = Clock::now();
		getUp.Withdraw();
		StartRecording(kRecordAfterGetUp);
		if (!player) {
			return;
		}
		const bool exit = Notify(player, kExitEvent);
		const bool stop = !exit && Notify(player, kStopEvent);
		const bool reset = !exit && !stop && Notify(player, kResetEvent);
		Log("get up from {} ({}): {}={} {}={} {}={}", PoseName(was), a_reason, kExitEvent, exit, kStopEvent, stop,
			kResetEvent, reset);
		if (!exit && !stop && !reset) {
			Util::Notify("CIGAR: 일어나기 실패. 로그 확인");
		}
	}

	void Rest::OnDisabled()
	{
		pendingPose = Pose::kStanding;
		if (pose != Pose::kStanding) {
			GetUp("module switched off");
		}
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
