#include "Observe.h"

#include "Util.h"

namespace CIGAR
{
	namespace
	{
		// SI's Observer defaults (settings.json, Modules/Observer).
		constexpr auto kIdleTime = 5s;
		constexpr float kMaxDistance = 5000.0f;
		constexpr float kFOVOffset = 40.0f;
		constexpr float kMinFOV = 15.0f;
		constexpr float kMoveInput = 0.2f;
		// A full zoom in takes 2 s, as SI's 20 degrees a second over 40 did; the way back is quicker.
		// A partial zoom takes its share of the time.
		constexpr float kZoomInSeconds = 2.0f;
		constexpr float kZoomOutSeconds = 0.7f;
		// The frame driver's poll; the task it posts runs on the next frame, at most one per frame.
		constexpr auto kFramePoll = 4ms;

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
		stillSince = Clock::now();
		target = {};
		StartFrameDriver();
		Log("ready: idle {}s, distance {}, fov -{} eased over {}s in / {}s out, furniture ignored",
			std::chrono::duration_cast<std::chrono::seconds>(kIdleTime).count(), kMaxDistance, kFOVOffset, kZoomInSeconds,
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
			look.Update(true, [name] { return std::format("주시하기 (누르고 있기): {}", name); });
			return;
		}

		if (moveInput || drawn || combat) {
			stillSince = now;
		}
		RE::ObjectRefHandle aimed;
		if (auto* pick = RE::CrosshairPickData::GetSingleton()) {
			aimed = pick->GetActiveTarget();
		}
		auto ref = aimed.get();
		// Furniture is for sitting or crafting, not for watching (the user, 2026-09-25).
		const auto* base = ref ? ref->GetBaseObject() : nullptr;
		const bool furniture = base && base->Is(RE::FormType::Furniture);
		if (!ref || ref.get() == player || furniture) {
			target = {};
			targetName.clear();
			ref = nullptr;
		} else if (aimed != target) {
			target = aimed;
			targetName = Util::NameOf(ref.get());
		}
		const bool inReach = ref && player->GetPosition().GetDistance(ref->GetPosition()) <= kMaxDistance;
		const bool named = ref && !targetName.empty() && targetName != "-";
		const auto* controlMap = RE::ControlMap::GetSingleton();
		const bool looking = controlMap && controlMap->IsLookingControlsEnabled() && controlMap->IsMovementControlsEnabled();
		const bool still = now - stillSince >= kIdleTime;
		const bool ready = still && inReach && named && looking && !easing;

		LogGate(std::format("target={} furniture={} near={} still5s={} drawn={} combat={} looking={} easing={} ready={}",
			named ? targetName : "-"s, furniture, inReach, still, drawn, combat, looking, easing.load(), ready));
		const auto name = targetName;
		look.Update(ready, [name] { return std::format("주시하기 (누르고 있기): {}", name); });
	}

	void Observe::OnHold(std::uint16_t a_eventID, bool a_down)
	{
		if (a_eventID != kLook) {
			return;
		}
		if (!a_down) {
			if (zooming) {
				Restore("key released");
			}
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
		Log("observing {}: fov {:.1f} -> {:.1f} over {:.2f}s ({})", targetName, baseFOV, goal, ease.length,
			firstPerson ? "first person" : "third person");
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
