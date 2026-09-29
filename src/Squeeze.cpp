#include "Squeeze.h"

#include "Text.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		// Blocked: moving input, a non-hostile humanoid ahead, the speed collapsed for kBlockedFor.
		// 0.3 s is the user's value (2026-09-28). The user's reach was 70; r8b measured contacts at up to 73
		// ahead, centre to centre, so 80 (mine). The corridor and the speed are mine.
		constexpr float kBlockAhead = 80.0f;
		constexpr float kBlockCorridor = 50.0f;
		constexpr float kBlockedSpeed = 50.0f;  // units per second; walking is about 80, running 300
		constexpr auto kBlockedFor = 300ms;
		// The ring: the key must be down this long before anything is changed (docs/042; Deflate's value).
		constexpr auto kRingFill = 500ms;
		// Past the NPC: this far beyond it along the player's heading, and this far from it (the probe's).
		constexpr float kClearPast = 30.0f;
		constexpr float kClearDistance = 70.0f;
		// Undone when the player makes no forward progress for kNoProgress (r8b Q5: a flat 3 s cut a
		// side pass short), and after kPassCap in any case. Mine.
		constexpr auto kNoProgress = 3s;
		constexpr float kProgressStep = 5.0f;
		constexpr auto kPassCap = 10s;
		constexpr auto kBumpCooldown = 1500ms;  // the user's value (2026-09-28)

		// The four IDLE children of BumpedIntoRoot (Skyrim.esm 03DE4E); r5: accepted ones displace the
		// NPC 50-84 units with no line, no hostility and no crime.
		constexpr const char* kBumpFront = "NPC_BumpFromFront";
		constexpr const char* kBumpBack = "NPC_BumpedFromBack";
		constexpr const char* kBumpLeft = "NPC_BumpedFromLeft";
		constexpr const char* kBumpRight = "NPC_BumpedFromRight";

#ifndef CIGAR_RELEASE
		// Author build only, until the user picks the public clip (D17): EVG Squeeze from the user's own
		// install, in the author-only OAR mod `CIGAR Push Test` (tools/push_test_assets.py). 3805 plays the
		// upper body, 3806 the left arm only; the same clip, 2.87 s.
		constexpr std::int32_t kGestureFull = 3805;
		constexpr std::int32_t kGestureLeft = 3806;
		constexpr float kGestureSeconds = 2.87f;
		constexpr float kGestureStraight = 30.0f;  // within this of the line ahead the NPC is straight ahead
#endif
		constexpr auto kClipVariable = "iGPMAAnimationType";
		constexpr auto kArmVariable = "iGPMAOffsetType";

		float Flat(const RE::NiPoint3& a_a, const RE::NiPoint3& a_b)
		{
			return std::hypot(a_a.x - a_b.x, a_a.y - a_b.y);
		}

		RE::NiPoint3 Forward(float a_heading)
		{
			return { std::sin(a_heading), std::cos(a_heading), 0.0f };
		}

		RE::NiPoint3 Right(float a_heading)
		{
			return { std::cos(a_heading), -std::sin(a_heading), 0.0f };
		}

		bool Moving(const RE::PlayerCharacter* a_player)
		{
			const auto* state = a_player->AsActorState();
			return state && (state->IsWalking() || state->IsRunning() || state->IsSprinting());
		}

		// The bump idle for the side of the NPC the player comes from.
		const char* BumpEvent(RE::Actor* a_npc, const RE::NiPoint3& a_player)
		{
			const auto to = a_player - a_npc->GetPosition();
			const float heading = a_npc->GetAngleZ();
			const auto forward = Forward(heading);
			const auto right = Right(heading);
			const float f = to.x * forward.x + to.y * forward.y;
			const float r = to.x * right.x + to.y * right.y;
			if (std::abs(f) >= std::abs(r)) {
				return f >= 0.0f ? kBumpFront : kBumpBack;
			}
			return r >= 0.0f ? kBumpRight : kBumpLeft;
		}

		// Why this NPC is not squeezed past, or empty.
		std::string Refusal(RE::Actor* a_npc, RE::PlayerCharacter* a_player)
		{
			if (a_npc->IsDead()) {
				return "dead";
			}
			if (!a_npc->HasKeywordString("ActorTypeNPC")) {
				return "not humanoid";
			}
			if (a_npc->IsHostileToActor(a_player) || a_npc->IsInCombat()) {
				return "hostile or in combat";
			}
			if (a_npc->IsOnMount() || a_npc->IsAMount()) {
				return "mounted";
			}
			// r5: the graph refuses the bump in dialogue and scenes; the prompt leaves those NPCs alone.
			if (a_npc->GetCurrentScene()) {
				return "in a scene";
			}
			if (const auto* topics = RE::MenuTopicManager::GetSingleton(); topics && topics->speaker.get().get() == a_npc) {
				return "talking to the player";
			}
			const auto* state = a_npc->AsActorState();
			if (a_npc->GetOccupiedFurniture().get().get() || (state && state->GetSitSleepState() != RE::SIT_SLEEP_STATE::kNormal)) {
				return "in furniture";
			}
			if (Util::InScene(a_npc)) {
				return "in a scene framework";
			}
			return {};
		}
	}

	Squeeze::Squeeze()
	{
		// A hold that lasts while held: key down and up come to OnHold, SkyPrompt draws the ring.
		prompt.SetHoldMode(true);
		prompt.SetPromptType(SkyPromptAPI::kHoldAndKeep);
	}

	Squeeze* Squeeze::GetSingleton()
	{
		static Squeeze singleton;
		return &singleton;
	}

	void Squeeze::OnGameLoaded()
	{
		if (pass) {
			End(Util::Player(), "game loaded");
		}
		prompt.Reset();
		lastGate.clear();
		pressing = false;
		active = false;
		blocked = false;
		gesturePlaying = false;
		lastTick = {};
		lastBumped = 0;
		lastBumpAt = {};
		restoreWarned = false;
#ifdef CIGAR_RELEASE
		Log("ready: squeeze past on (no gesture in this build)");
#else
		bool gpma = false;
		auto* player = Util::Player();
		const bool read = player && player->GetGraphVariableBool("bGPMAInstalled", gpma);
		Log("ready: squeeze past on; gesture {} (bGPMAInstalled {}, read {})", gpma ? "on" : "off", gpma, read);
#endif
	}

	RE::Actor* Squeeze::Blocker(RE::PlayerCharacter* a_player, float& a_ahead, float& a_side, std::string& a_why) const
	{
		auto* lists = RE::ProcessLists::GetSingleton();
		if (!lists) {
			return nullptr;
		}
		const auto pos = a_player->GetPosition();
		const float heading = a_player->GetAngleZ();
		const auto forward = Forward(heading);
		const auto right = Right(heading);
		RE::Actor* best = nullptr;
		float bestAhead = kBlockAhead + 1.0f;
		for (auto& handle : lists->highActorHandles) {
			const auto actor = handle.get();
			if (!actor || actor.get() == a_player || !actor->Is3DLoaded()) {
				continue;
			}
			const auto to = actor->GetPosition() - pos;
			const float ahead = to.x * forward.x + to.y * forward.y;
			const float side = to.x * right.x + to.y * right.y;
			if (ahead <= 0.0f || ahead > kBlockAhead || std::abs(side) > kBlockCorridor || std::abs(to.z) > 100.0f || ahead >= bestAhead) {
				continue;
			}
			best = actor.get();
			bestAhead = ahead;
			a_ahead = ahead;
			a_side = side;
		}
		if (!best) {
			a_why = "none ahead";
			return nullptr;
		}
		a_why = Refusal(best, a_player);
		if (!a_why.empty()) {
			a_why = std::format("{}: {}", Util::NameOf(best), a_why);
			return nullptr;
		}
		return best;
	}

	void Squeeze::FastTick()
	{
		auto* player = Util::Player();
		if (!player) {
			return;
		}
		const auto now = Clock::now();
		const auto pos = player->GetPosition();
		const float dt = std::chrono::duration<float>(now - lastTick).count();
		const float speed = dt > 0.0f && dt < 1.0f ? Flat(pos, lastPos) / dt : 0.0f;
		lastPos = pos;
		lastTick = now;

		if (gesturePlaying && now - gestureStart >= std::chrono::duration<float>(gestureSeconds)) {
			StopGesture(player, "clip length");
		}
		if (pressing && !active && now - pressedAt >= kRingFill) {
			active = true;
			Log("ring filled: squeezing past while held");
		}

		const auto* state = player->AsActorState();
		const bool combat = player->IsInCombat();
		const bool drawn = state && state->IsWeaponDrawn();
		const bool seated = state && state->GetSitSleepState() != RE::SIT_SLEEP_STATE::kNormal;
		const bool swimming = state && state->IsSwimming();
		const bool mounted = player->IsOnMount();
		const auto* camera = RE::PlayerCamera::GetSingleton();
		const bool firstPerson = camera && camera->IsInFirstPerson();
		const bool busy = Util::IsBusy(player);
		const bool scene = Util::InScene(player);
		const bool moving = Moving(player);
		// Non-combat, third person first (docs/038).
		const bool free = !combat && !drawn && !seated && !swimming && !mounted && !firstPerson && !busy && !scene;

		if (pass) {
			auto npc = pass->npc.get();
			const auto* cell = player->GetParentCell();
			const float heading = player->GetAngleZ();
			const auto forward = Forward(heading);
			if (!npc || npc->IsDead() || !npc->Is3DLoaded()) {
				End(player, "the NPC is gone");
			} else if ((cell ? cell->GetFormID() : 0) != passCell) {
				End(player, "cell changed");
			} else if (combat) {
				End(player, "combat");
			} else {
				const auto npcPos = npc->GetPosition();
				// Along the player's heading, not the start line: a side pass walks diagonally to it (r8b Q5).
				const float past = (pos.x - npcPos.x) * forward.x + (pos.y - npcPos.y) * forward.y;
				pass->moved = Flat(pos, pass->playerStart);
				if (past > pass->bestPast + kProgressStep) {
					pass->bestPast = past;
					pass->lastProgress = now;
				}
				if (past >= kClearPast && Flat(pos, npcPos) >= kClearDistance) {
					End(player, "past the NPC");
				} else if (now - pass->lastProgress >= kNoProgress) {
					End(player, "no forward progress for 3 s");
				} else if (now - pass->start >= kPassCap) {
					End(player, "10 s cap");
				}
			}
		}

		float ahead = 0.0f;
		float side = 0.0f;
		std::string why;
		auto* npc = moving && free ? Blocker(player, ahead, side, why) : nullptr;
		if (!moving) {
			why = "not moving";
		} else if (!free) {
			why = "not free";
		}
		const bool mine = npc && pass && pass->npc.get().get() == npc;
		const bool slow = speed < kBlockedSpeed;
		if (npc && slow && !mine) {
			if (!blocked) {
				blocked = true;
				blockedSince = now;
			}
		} else {
			blocked = false;
		}
		const bool blockedLong = blocked && now - blockedSince >= kBlockedFor;

		LogGate(std::format("moving={} slow={} npc={} combat={} drawn={} seated={} swim={} mount={} firstPerson={} busy={} scene={} "
							"blocked={} pressing={} active={} passing={}",
			moving, slow, npc ? Util::NameOf(npc) : "- (" + why + ")", combat, drawn, seated, swimming, mounted, firstPerson, busy, scene,
			blockedLong, pressing, active, pass ? pass->name : "-"s));

		if (active && npc && !mine && free) {
			if (pass) {
				End(player, "the next NPC");
			}
			Begin(player, npc, ahead, side);
		}
		prompt.Update(free && (blockedLong || pressing || active), [] { return std::string(Text::L("비켜 지나가기 (누르고 있기)", "Squeeze Past (hold)")); });
	}

	void Squeeze::Begin(RE::PlayerCharacter* a_player, RE::Actor* a_npc, float a_ahead, float a_side)
	{
		auto* controller = a_npc->GetCharController();
		auto* body = controller ? controller->GetRigidBody() : nullptr;
		if (!body) {
			Log("WARN {} {:08X} has no controller body: not squeezed", Util::NameOf(a_npc), a_npc->GetFormID());
			return;
		}
		auto& filter = body->GetCollidableRW()->broadPhaseHandle.collisionFilterInfo;
		Pass p;
		p.npc = a_npc->GetHandle();
		p.name = std::format("{} {:08X}", Util::NameOf(a_npc), a_npc->GetFormID());
		p.body = RE::hkRefPtr<RE::hkpRigidBody>(body);
		p.oldFilter = filter.filter;
		p.playerStart = a_player->GetPosition();
		p.npcStart = a_npc->GetPosition();
		p.start = Clock::now();
		p.lastProgress = p.start;
		filter.SetNoCollision(true);
		const bool set = filter.QNoCollision();
		const auto* cell = a_player->GetParentCell();
		passCell = cell ? cell->GetFormID() : 0;

		std::string bump = "cooldown";
		const auto id = a_npc->GetFormID();
		if (id != lastBumped || p.start - lastBumpAt >= kBumpCooldown) {
			const auto* event = BumpEvent(a_npc, p.playerStart);
			const bool accepted = a_npc->NotifyAnimationGraph(event);
			bump = std::format("{} accepted {}", event, accepted);
			lastBumped = id;
			lastBumpAt = p.start;
		}
		Log("start: {} at {:.0f} ahead, {:+.0f} to the side; NPC body filter {:08X} -> {:08X} (no collision {}), NPC z {:.0f}; bump {}",
			p.name, a_ahead, a_side, p.oldFilter, filter.filter, set, p.npcStart.z, bump);
		pass = std::move(p);
		if (!set) {
			End(a_player, "the no-collision flag did not stick");
			return;
		}
		PlayGesture(a_player, a_side);
	}

	void Squeeze::End(RE::PlayerCharacter* a_player, std::string_view a_why)
	{
		if (!pass) {
			return;
		}
		std::string restore = "no body";
		bool restored = false;
		if (auto* body = pass->body.get()) {
			auto& filter = body->GetCollidableRW()->broadPhaseHandle.collisionFilterInfo;
			const auto before = filter.filter;
			filter.filter = pass->oldFilter;
			restored = filter.filter == pass->oldFilter;
			restore = std::format("NPC body filter {:08X} -> {:08X} (read back {:08X})", before, pass->oldFilter, filter.filter);
		}
		const auto npc = pass->npc.get();
		std::string where = "the NPC is gone";
		bool passed = false;
		if (npc && a_player) {
			const auto pos = a_player->GetPosition();
			const auto npcPos = npc->GetPosition();
			const auto forward = Forward(a_player->GetAngleZ());
			const float past = (pos.x - npcPos.x) * forward.x + (pos.y - npcPos.y) * forward.y;
			const float apart = Flat(pos, npcPos);
			passed = past >= kClearPast && apart >= kClearDistance;
			// Close and not past at the undo: the physics pushes the two apart (the probe's POPPED).
			where = std::format("{:.0f} from the NPC, {:+.0f} past it; the NPC moved {:.0f} (z {:+.0f}){}", apart, past,
				Flat(npcPos, pass->npcStart), npcPos.z - pass->npcStart.z, apart < 40.0f ? "; STILL OVERLAPPING" : "");
		}
		Log("RESULT squeeze on {} ({}): {}, moved {:.0f} in {:.1f} s; {}; {}", pass->name, a_why, passed ? "PASSED" : "DID NOT PASS", pass->moved,
			std::chrono::duration<float>(Clock::now() - pass->start).count(), where, restore);
		if (!restored && pass->body.get()) {
			Log("WARN the NPC's collision filter did not go back");
			if (!restoreWarned) {
				restoreWarned = true;
				Util::Notify(Text::L("CIGAR: 비켜 지나가기 복원 실패. 로그 확인", "CIGAR: Squeeze Past could not restore an NPC. See the log"));
			}
		}
		pass.reset();
	}

	void Squeeze::PlayGesture([[maybe_unused]] RE::PlayerCharacter* a_player, [[maybe_unused]] float a_side)
	{
#ifndef CIGAR_RELEASE
		if (gesturePlaying) {
			return;
		}
		bool installed = false;
		std::int32_t current = 0;
		if (!a_player->GetGraphVariableBool("bGPMAInstalled", installed) || !installed) {
			Log("gesture: skipped (Offset Movement Animation not in the graph)");
			return;
		}
		// Helmet uses the same layer; never cut its clip.
		if (a_player->GetGraphVariableInt(kClipVariable, current) && current != 0) {
			Log("gesture: skipped ({}={} already playing)", kClipVariable, current);
			return;
		}
		const bool straight = std::abs(a_side) <= kGestureStraight;
		const bool left = !straight && a_side < 0.0f;
		const std::int32_t value = left ? kGestureLeft : kGestureFull;
		const std::int32_t arms = left ? 2 : 0;
		const bool arm = a_player->SetGraphVariableInt(kArmVariable, arms);
		const bool clip = a_player->SetGraphVariableInt(kClipVariable, value);
		const bool sent = a_player->NotifyAnimationGraph("OffsetGPMA");
		gesturePlaying = sent;
		gestureStart = Clock::now();
		gestureSeconds = kGestureSeconds;
		Log("gesture {} ({}): {}={} set {}, {}={} set {}, OffsetGPMA accepted {}", value,
			straight ? "straight ahead" : left ? "on the left" : "on the right", kArmVariable, arms, arm, kClipVariable, value, clip, sent);
		if (!sent) {
			a_player->SetGraphVariableInt(kClipVariable, 0);
			a_player->SetGraphVariableInt(kArmVariable, 0);
		}
#endif
	}

	void Squeeze::StopGesture(RE::PlayerCharacter* a_player, std::string_view a_why)
	{
		if (!gesturePlaying || !a_player) {
			gesturePlaying = false;
			return;
		}
		gesturePlaying = false;
		const bool sent = a_player->NotifyAnimationGraph("OffsetGPMAStop");
		const bool clip = a_player->SetGraphVariableInt(kClipVariable, 0);
		const bool arm = a_player->SetGraphVariableInt(kArmVariable, 0);
		Log("gesture stop ({}): OffsetGPMAStop accepted {}, {}=0 set {}, {}=0 set {}", a_why, sent, kClipVariable, clip, kArmVariable, arm);
	}

	void Squeeze::OnHold(std::uint16_t a_eventID, bool a_down)
	{
		if (a_eventID != kSqueeze) {
			return;
		}
		if (a_down) {
			if (!pressing) {
				pressing = true;
				pressedAt = Clock::now();
			}
			return;
		}
		if (pressing && !active) {
			Log("released before the ring filled: nothing changed");
		} else if (active) {
			// The NPC being passed is let go by its own rule (past it, no progress, the cap), not here:
			// restoring while the two overlap would push them apart.
			Log("released: no further NPC is squeezed past{}", pass ? std::format("; {} is let go once passed", pass->name) : "");
		}
		pressing = false;
		active = false;
	}

	void Squeeze::OnDeclined(std::uint16_t a_eventID)
	{
		if (a_eventID == kSqueeze) {
			pressing = false;
			active = false;
		}
	}

	void Squeeze::OnDisabled()
	{
		pressing = false;
		active = false;
		auto* player = Util::Player();
		End(player, "module switched off");
		StopGesture(player, "module switched off");
	}
}
