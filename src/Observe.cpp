#include "Observe.h"

#include "Util.h"

namespace CIGAR
{
	namespace
	{
		// Stand still this long, within this distance; zoom by up to this many degrees.
		constexpr auto kIdleTime = 5s;
		constexpr float kMaxDistance = 5000.0f;
		constexpr float kFOVOffset = 40.0f;
		constexpr float kMinFOV = 15.0f;
		constexpr float kMoveInput = 0.2f;
		// Rest's floor pitch (radians, positive is down): looking this far down offers 앉기/눕기, so
		// scenery is only watched above it and the two never share a moment.
		constexpr float kFloorPitch = 0.6f;

		std::string Label(const std::string& a_name)
		{
			return a_name.empty() ? std::string(Text::L("주시하기 (누르고 있기)", "Observe (hold)")) :
			                       Text::F("주시하기 (누르고 있기): {}", "Observe (hold): {}", a_name);
		}
		// A full zoom in takes 2 s (20 degrees a second over 40); the way back is quicker.
		// A partial zoom takes its share of the time.
		constexpr float kZoomInSeconds = 2.0f;
		constexpr float kZoomOutSeconds = 0.7f;
		// The frame driver's poll; the task it posts runs on the next frame, at most one per frame.
		constexpr auto kFramePoll = 4ms;
		// A declined prompt returns once the player is this far from where it was declined.
		constexpr float kLeaveDistance = 300.0f;
		// A press shorter than this is a tap; two taps this close together are a double tap (decline).
		constexpr auto kTapLength = 250ms;
		constexpr auto kDoubleTapGap = 600ms;

		float Smootherstep(float a_t)
		{
			a_t = std::clamp(a_t, 0.0f, 1.0f);
			return a_t * a_t * a_t * (a_t * (a_t * 6.0f - 15.0f) + 10.0f);
		}
	}

	Observe::Observe()
	{
		look.SetHoldMode(true);
		look.SetPromptType(SkyPromptAPI::kHoldAndKeep);
	}

	Observe* Observe::GetSingleton()
	{
		static Observe singleton;
		return &singleton;
	}

	void Observe::OnGameLoaded()
	{
		look.Reset();
		lastGate.clear();
		zooming = false;
		easing = false;
		dismissed = false;
		stillSince = Clock::now();
		target = {};
		StartFrameDriver();
		Log("ready: idle {}s, actors within {} or scenery above pitch {}, fov -{} eased over {}s in / {}s out",
			std::chrono::duration_cast<std::chrono::seconds>(kIdleTime).count(), kMaxDistance, kFloorPitch, kFOVOffset, kZoomInSeconds,
			kZoomOutSeconds);
	}

	void Observe::StartFrameDriver()
	{
		static std::once_flag started;
		std::call_once(started, [] { std::thread([] {
			auto* self = GetSingleton();
			for (;;) {
				std::this_thread::sleep_for(kFramePoll);
				if (self->easing && !self->frameQueued.exchange(true)) {
					SKSE::GetTaskInterface()->AddTask([self] {
						self->frameQueued = false;
						self->Frame();
					});
				}
			}
		}).detach(); });
	}

	void Observe::SetFOV(float a_fov) const
	{
		if (auto* camera = RE::PlayerCamera::GetSingleton()) {
			auto& data = camera->GetRuntimeData2();
			(firstPerson ? data.firstPersonFOV : data.worldFOV) = a_fov;
		}
	}

	void Observe::StartEase(float a_to, float a_seconds)
	{
		ease.from = currentFOV;
		ease.to = a_to;
		ease.start = Clock::now();
		ease.length = std::max(a_seconds, 0.05f);
		easing = true;
	}

	void Observe::Frame()
	{
		if (!easing) {
			return;
		}
		const float t = std::chrono::duration<float>(Clock::now() - ease.start).count() / ease.length;
		currentFOV = ease.from + (ease.to - ease.from) * Smootherstep(t);
		SetFOV(currentFOV);
		if (t >= 1.0f) {
			easing = false;
			if (!zooming) {
				Log("fov back to {:.1f}", currentFOV);
			}
		}
	}

	void Observe::FastTick()
	{
		auto* player = Util::Player();
		auto* camera = RE::PlayerCamera::GetSingleton();
		if (!player || !camera) {
			return;
		}
		const auto now = Clock::now();
		const auto* controls = RE::PlayerControls::GetSingleton();
		const bool moveInput = controls && (controls->data.moveInputVec.Length() >= kMoveInput || controls->data.autoMove);
		const auto* state = player->AsActorState();
		const bool drawn = state && state->IsWeaponDrawn();
		const bool combat = player->IsInCombat();

		if (zooming) {
			if (moveInput || combat) {
				Restore(moveInput ? "moved" : "combat");
				return;
			}
			const auto name = targetName;
			look.Update(true, [name] { return Label(name); });
			return;
		}

		if (moveInput || drawn || combat) {
			stillSince = now;
		}
		if (dismissed && player->GetPosition().GetDistance(dismissedAt) > kLeaveDistance) {
			dismissed = false;
			Log("left the spot: 주시하기 may show again");
		}
		RE::ObjectRefHandle aimed;
		if (auto* pick = RE::CrosshairPickData::GetSingleton()) {
			aimed = pick->GetActiveTarget();
		}
		// What is watched (the user, 2026-09-25: observation and scouting): an actor under the crosshair,
		// or nothing at all, which is scenery (a valley, a distant camp beyond the crosshair's reach).
		// An object under the crosshair (an item, a door, furniture) is at arm's length and is not.
		const auto ref = aimed.get();
		auto* actor = ref && ref.get() != player ? ref->As<RE::Actor>() : nullptr;
		const auto* base = ref ? ref->GetBaseObject() : nullptr;
		const char* kind = !ref || ref.get() == player ? "scenery" : actor ? "actor" : base && base->Is(RE::FormType::Furniture) ? "furniture" : "object";
		if (actor) {
			if (aimed != target) {
				target = aimed;
				targetName = Util::NameOf(actor);
			}
		} else {
			target = {};
			targetName.clear();
		}
		const float pitch = player->GetAngleX();
		const bool actorOK = actor && player->GetPosition().GetDistance(actor->GetPosition()) <= kMaxDistance &&
		                     !targetName.empty() && targetName != "-";
		const bool sceneryOK = kind == "scenery"sv && pitch < kFloorPitch;
		const auto* controlMap = RE::ControlMap::GetSingleton();
		const bool looking = controlMap && controlMap->IsLookingControlsEnabled() && controlMap->IsMovementControlsEnabled();
		const bool still = now - stillSince >= kIdleTime;
		// Between the two taps of a double tap the view is still easing back: keep the prompt up so the
		// second tap reaches it.
		const bool tapWindow = now - lastTapAt < kDoubleTapGap;
		const bool ready = !dismissed && ((still && (actorOK || sceneryOK) && looking && !easing) || tapWindow);

		LogGate(std::format("kind={} target={} pitch={:.2f} still5s={} drawn={} combat={} looking={} easing={} dismissed={} ready={}",
			kind, actor ? targetName : "-"s, pitch, still, drawn, combat, looking, easing.load(), dismissed, ready));
		const auto name = targetName;
		look.Update(ready, [name] { return Label(name); });
	}

	void Observe::OnHold(std::uint16_t a_eventID, bool a_down)
	{
		if (a_eventID != kLook) {
			return;
		}
		const auto now = Clock::now();
		if (!a_down) {
			if (zooming) {
				Restore("key released");
			}
			if (now - downAt < kTapLength) {
				if (now - lastTapAt < kDoubleTapGap) {
					lastTapAt = {};
					Dismiss("double tap");
				} else {
					lastTapAt = now;
				}
			}
			return;
		}
		downAt = now;
		if (dismissed) {
			return;
		}
		auto* camera = RE::PlayerCamera::GetSingleton();
		if (!camera || zooming) {
			return;
		}
		if (!easing) {
			// Pressed again while the last zoom is still easing out: keep that zoom's base, so the
			// half-restored FOV is never taken for the player's own.
			firstPerson = camera->IsInFirstPerson();
			auto& data = camera->GetRuntimeData2();
			baseFOV = firstPerson ? data.firstPersonFOV : data.worldFOV;
			currentFOV = baseFOV;
		}
		zooming = true;
		const float goal = std::max(kMinFOV, baseFOV - kFOVOffset);
		StartEase(goal, kZoomInSeconds * (currentFOV - goal) / kFOVOffset);
		Log("observing {}: fov {:.1f} -> {:.1f} over {:.2f}s ({})", targetName.empty() ? "scenery"s : targetName, baseFOV, goal, ease.length,
			firstPerson ? "first person" : "third person");
	}

	void Observe::OnDeclined(std::uint16_t a_eventID)
	{
		if (a_eventID == kLook) {
			Dismiss("declined");
		}
	}

	void Observe::Dismiss(std::string_view a_how)
	{
		Restore(a_how);
		dismissed = true;
		if (auto* player = Util::Player()) {
			dismissedAt = player->GetPosition();
		}
		look.Withdraw();
		Log("주시하기 dismissed ({}): hidden until the player moves {:.0f} units away", a_how, kLeaveDistance);
	}

	void Observe::Restore(std::string_view a_reason)
	{
		if (!zooming) {
			return;
		}
		zooming = false;
		const float span = std::max(baseFOV - std::max(kMinFOV, baseFOV - kFOVOffset), 1.0f);
		StartEase(baseFOV, kZoomOutSeconds * (baseFOV - currentFOV) / span);
		stillSince = Clock::now();
		Log("observe ended ({}): easing fov {:.1f} back to {:.1f}", a_reason, currentFOV, baseFOV);
	}

	void Observe::OnDisabled()
	{
		if (zooming || easing) {
			zooming = false;
			easing = false;
			currentFOV = baseFOV;
			SetFOV(baseFOV);
			Log("observe ended (module switched off): fov back to {:.1f}", baseFOV);
		}
	}
}
