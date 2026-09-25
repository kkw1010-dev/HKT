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
		constexpr float kFOVPerSecond = 20.0f;
		constexpr float kMinFOV = 15.0f;
		constexpr float kMoveInput = 0.2f;
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
		stillSince = Clock::now();
		target = {};
		Log("ready: idle {}s, distance {}, fov -{} at {}/s", std::chrono::duration_cast<std::chrono::seconds>(kIdleTime).count(),
			kMaxDistance, kFOVOffset, kFOVPerSecond);
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
			const float dt = std::chrono::duration<float>(now - lastStep).count();
			lastStep = now;
			const float goal = std::max(kMinFOV, baseFOV - kFOVOffset);
			currentFOV = std::max(goal, currentFOV - kFOVPerSecond * dt);
			auto& data = camera->GetRuntimeData2();
			(firstPerson ? data.firstPersonFOV : data.worldFOV) = currentFOV;
			const auto name = targetName;
			look.Update(true, [name] { return std::format("살펴보기 (누르고 있기): {}", name); });
			return;
		}

		if (moveInput || drawn || combat) {
			stillSince = now;
		}
		RE::ObjectRefHandle aimed;
		if (auto* pick = RE::CrosshairPickData::GetSingleton()) {
			aimed = pick->GetActiveTarget();
		}
		const auto ref = aimed.get();
		if (!ref || ref.get() == player) {
			target = {};
		} else if (aimed != target) {
			target = aimed;
			targetName = Util::NameOf(ref.get());
		}
		const bool inReach = ref && player->GetPosition().GetDistance(ref->GetPosition()) <= kMaxDistance;
		const bool named = ref && !targetName.empty() && targetName != "-";
		const auto* controlMap = RE::ControlMap::GetSingleton();
		const bool looking = controlMap && controlMap->IsLookingControlsEnabled() && controlMap->IsMovementControlsEnabled();
		const bool still = now - stillSince >= kIdleTime;
		const bool ready = still && inReach && named && looking;

		LogGate(std::format("target={} near={} still5s={} drawn={} combat={} looking={} ready={}", named ? targetName : "-"s,
			inReach, still, drawn, combat, looking, ready));
		const auto name = targetName;
		look.Update(ready, [name] { return std::format("살펴보기 (누르고 있기): {}", name); });
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
		firstPerson = camera->IsInFirstPerson();
		auto& data = camera->GetRuntimeData2();
		baseFOV = firstPerson ? data.firstPersonFOV : data.worldFOV;
		currentFOV = baseFOV;
		lastStep = Clock::now();
		zooming = true;
		Log("observing {}: fov {:.1f} -> {:.1f} ({})", targetName, baseFOV, std::max(kMinFOV, baseFOV - kFOVOffset),
			firstPerson ? "first person" : "third person");
	}

	void Observe::Restore(std::string_view a_reason)
	{
		if (!zooming) {
			return;
		}
		zooming = false;
		if (auto* camera = RE::PlayerCamera::GetSingleton()) {
			auto& data = camera->GetRuntimeData2();
			(firstPerson ? data.firstPersonFOV : data.worldFOV) = baseFOV;
		}
		stillSince = Clock::now();
		Log("observe ended ({}): fov back to {:.1f}", a_reason, baseFOV);
	}

	void Observe::OnDisabled()
	{
		Restore("module switched off");
	}
}
