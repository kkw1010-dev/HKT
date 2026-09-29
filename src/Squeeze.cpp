#include "Squeeze.h"

#include <cstring>
#include <expected>
#include <unordered_map>

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

		// The gesture: EVG Animated Traversal's Squeeze, played on Offset Movement Animation's layer through
		// CIGAR's own OAR submod while iGPMAAnimationType holds kGestureValue (Helmet uses 2 and 6, the old
		// probe 3801-3806). iGPMAOffsetType 0 plays it on the upper body, 2 on the left arm only.
		constexpr std::int32_t kGestureValue = 3810;
		// Used when the clip's own length cannot be read (EVG 2.1's Squeeze is 2.87 s).
		constexpr float kGestureFallbackSeconds = 2.87f;
		// The prepared clip's length, from its hkaAnimation (the winner may be another mod's Squeeze:
		// in the user's order EVG Animations Replacer's, 3.13 s).
		float clipSeconds = kGestureFallbackSeconds;
		constexpr float kGestureStraight = 30.0f;  // within this of the line ahead the NPC is straight ahead

		// PrepareClip's files, Data-relative (through MO2's VFS; new files land in its overwrite).
		constexpr auto kEvgClip =
			"Data/meshes/actors/character/animations/OpenAnimationReplacer/EVG Animated Traversal/Squeeze/mt_leverfloorpull.hkx";
		constexpr auto kModFolder = "Data/meshes/OpenAnimationReplacer/CIGAR Squeeze";
		constexpr auto kSubmodFolder = "Data/meshes/OpenAnimationReplacer/CIGAR Squeeze/Squeeze";
		constexpr auto kClipOut =
			"Data/meshes/OpenAnimationReplacer/CIGAR Squeeze/Squeeze/Actors/Character/Animations/GPMAOffsetAnimation.hkx";

		// CIGAR's own OAR configs (the push-test format that played in r5 and r8b). 3810 is kGestureValue.
		constexpr std::string_view kModConfig = R"({
    "name": "CIGAR Squeeze",
    "author": "CIGAR",
    "description": "Written by CIGAR at load. The clip is the player's own EVG Animated Traversal Squeeze, annotations cleared."
}
)";
		constexpr std::string_view kSubmodConfig = R"({
    "name": "Squeeze",
    "priority": 753810,
    "conditions": [
        {
            "condition": "CompareValues",
            "requiredVersion": "1.0.0.0",
            "Value A": { "graphVariable": "iGPMAAnimationType", "graphVariableType": "Int" },
            "Comparison": "==",
            "Value B": { "value": 3810.0 }
        }
    ]
}
)";

		std::uint32_t U32(const std::vector<char>& a_b, std::size_t a_at)
		{
			std::uint32_t v = 0;
			std::memcpy(&v, a_b.data() + a_at, 4);
			return v;
		}

		// Clears every annotation of every animation in a 64-bit Havok 2010.2 packfile, in place, and
		// returns how many there were, or an error. EVG's Squeeze carries Animation Motion Revolution's
		// `animmotion` root motion and furniture events (IdleStop, IdleFurnitureExit): on a walking player
		// they would move or stop it (the stage 0b copy was stripped with hkanno for the same reason).
		// Layout (checked on EVG 2.1's file: 86 annotations, all in track 0 of 99): hkaAnimation holds its
		// annotationTracks hkArray at +40 (pointer) and +48 (size); a track is 24 bytes with its
		// annotations' size at +16. Only sizes are written; pointers and string data stay.
		std::expected<int, std::string> StripAnnotations(std::vector<char>& a_b)
		{
			const auto size = a_b.size();
			if (size < 0x100 || U32(a_b, 0) != 0x57E0E057 || U32(a_b, 4) != 0x10C0C010) {
				return std::unexpected("not a Havok packfile"s);
			}
			if (static_cast<std::uint8_t>(a_b[0x10]) != 8) {
				return std::unexpected("not a 64-bit packfile"s);
			}
			if (std::string_view(a_b.data() + 0x28, 14) != "hk_2010.2.0-r1") {
				return std::unexpected("not Havok 2010.2.0-r1"s);
			}
			struct Section
			{
				std::uint32_t start, local, global, virt, exports;
			};
			std::optional<Section> classes, data;
			for (std::size_t i = 0; i < 3; ++i) {
				const std::size_t at = 0x40 + 0x30 * i;
				const std::string_view tag(a_b.data() + at, strnlen(a_b.data() + at, 19));
				const Section s{ U32(a_b, at + 20), U32(a_b, at + 24), U32(a_b, at + 28), U32(a_b, at + 32), U32(a_b, at + 36) };
				if (tag == "__classnames__") {
					classes = s;
				} else if (tag == "__data__") {
					data = s;
				}
			}
			if (!classes || !data || static_cast<std::size_t>(data->start) + data->exports > size || classes->start >= size) {
				return std::unexpected("sections not found"s);
			}
			const std::size_t ds = data->start;
			std::unordered_map<std::uint32_t, std::uint32_t> local;
			for (std::size_t p = ds + data->local; p + 8 <= ds + data->global; p += 8) {
				const auto src = U32(a_b, p);
				if (src != 0xFFFFFFFF) {
					local[src] = U32(a_b, p + 4);
				}
			}
			int cleared = 0;
			int animations = 0;
			for (std::size_t p = ds + data->virt; p + 12 <= ds + data->exports; p += 12) {
				const auto object = U32(a_b, p);
				if (object == 0xFFFFFFFF) {
					continue;
				}
				const std::size_t nameAt = classes->start + static_cast<std::size_t>(U32(a_b, p + 8));
				if (nameAt >= size) {
					return std::unexpected("class name out of range"s);
				}
				const std::string_view name(a_b.data() + nameAt, strnlen(a_b.data() + nameAt, size - nameAt));
				if (!name.starts_with("hka") || !name.ends_with("Animation")) {
					continue;
				}
				++animations;
				if (ds + object + 56 > size) {
					return std::unexpected("animation out of range"s);
				}
				float duration = 0.0f;
				std::memcpy(&duration, a_b.data() + ds + object + 20, 4);
				if (duration > 0.1f && duration < 30.0f) {
					clipSeconds = duration;
				}
				const auto tracks = U32(a_b, ds + object + 48);
				if (tracks == 0) {
					continue;
				}
				const auto found = local.find(object + 40);
				if (found == local.end() || ds + found->second + 24ull * tracks > size) {
					return std::unexpected("annotation tracks not found"s);
				}
				for (std::uint32_t k = 0; k < tracks; ++k) {
					const std::size_t field = ds + found->second + 24ull * k + 16;
					const auto n = U32(a_b, field);
					if (n > 100000) {
						return std::unexpected("implausible annotation count"s);
					}
					cleared += static_cast<int>(n);
					std::memset(a_b.data() + field, 0, 4);
				}
			}
			if (animations == 0) {
				return std::unexpected("no animation in the file"s);
			}
			return cleared;
		}

		// Writes a_bytes to a_path unless it already holds exactly them; false when the write failed.
		bool WriteIfChanged(const std::filesystem::path& a_path, std::string_view a_bytes, bool& a_wrote)
		{
			a_wrote = false;
			std::error_code ec;
			if (std::filesystem::file_size(a_path, ec) == a_bytes.size() && !ec) {
				std::ifstream in(a_path, std::ios::binary);
				const std::string old((std::istreambuf_iterator<char>(in)), {});
				if (old == a_bytes) {
					return true;
				}
			}
			std::filesystem::create_directories(a_path.parent_path(), ec);
			{
				std::ofstream out(a_path, std::ios::binary | std::ios::trunc);
				out.write(a_bytes.data(), static_cast<std::streamsize>(a_bytes.size()));
				if (!out.good()) {
					return false;
				}
			}
			a_wrote = true;
			return std::filesystem::file_size(a_path, ec) == a_bytes.size() && !ec;
		}
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

	void Squeeze::PrepareClip()
	{
		clipReady = false;
		std::ifstream in(kEvgClip, std::ios::binary);
		if (!in) {
			// Without EVG the feature is off (D17); a copy left from an earlier install goes too.
			std::error_code ec;
			const auto removed = std::filesystem::remove_all(kModFolder, ec);
			clipNote = std::format("EVG Animated Traversal's Squeeze clip not found (loose file {}){}", kEvgClip,
				removed > 0 && removed != static_cast<std::uintmax_t>(-1) ? std::format("; removed {} file(s) of CIGAR's old copy", removed) : "");
			logs::info("[Squeeze] off: {}", clipNote);
			return;
		}
		std::vector<char> clip((std::istreambuf_iterator<char>(in)), {});
		const auto stripped = StripAnnotations(clip);
		if (!stripped) {
			clipNote = std::format("EVG's Squeeze clip could not be read ({}, {} bytes)", stripped.error(), clip.size());
			logs::warn("[Squeeze] off: {}", clipNote);
			return;
		}
		// Read back: a second pass over the result must find nothing left.
		auto check = clip;
		const auto left = StripAnnotations(check);
		bool wroteMod = false;
		bool wroteSub = false;
		bool wroteClip = false;
		const bool ok = left && *left == 0 &&
		                WriteIfChanged(std::filesystem::path(kModFolder) / "config.json", kModConfig, wroteMod) &&
		                WriteIfChanged(std::filesystem::path(kSubmodFolder) / "config.json", kSubmodConfig, wroteSub) &&
		                WriteIfChanged(kClipOut, std::string_view(clip.data(), clip.size()), wroteClip);
		if (!ok) {
			clipNote = std::format("the gesture submod could not be written under {}", kModFolder);
			logs::warn("[Squeeze] off: {}", clipNote);
			return;
		}
		clipReady = true;
		clipNote = std::format("gesture submod {} ({} bytes, {:.2f} s, {} annotations cleared; {})", kModFolder, clip.size(), clipSeconds, *stripped,
			wroteMod || wroteSub || wroteClip ? "written" : "already current");
		logs::info("[Squeeze] clip ready: {}", clipNote);
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
		gpmaChecked = false;
		gpmaInstalled = false;
		// Without EVG the whole feature is off, with one line (the user's D17: no gesture-less fallback).
		if (!clipReady) {
			Log("off: {}", clipNote);
		} else {
			Log("ready: {}", clipNote);
		}
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
		if (!player || !clipReady) {
			return;
		}
		// Offset Movement Animation plays the gesture; the player's graph answers once it is loaded.
		if (!gpmaChecked && player->Is3DLoaded()) {
			bool installed = false;
			if (player->GetGraphVariableBool("bGPMAInstalled", installed)) {
				gpmaChecked = true;
				gpmaInstalled = installed;
				Log("{}", installed ? "Offset Movement Animation found: on"
				                    : "off: Offset Movement Animation is not in the behaviour (bGPMAInstalled false)");
			}
		}
		if (!gpmaInstalled) {
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

	void Squeeze::PlayGesture(RE::PlayerCharacter* a_player, float a_side)
	{
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
		const std::int32_t value = kGestureValue;
		const std::int32_t arms = left ? 2 : 0;
		const bool arm = a_player->SetGraphVariableInt(kArmVariable, arms);
		const bool clip = a_player->SetGraphVariableInt(kClipVariable, value);
		const bool sent = a_player->NotifyAnimationGraph("OffsetGPMA");
		gesturePlaying = sent;
		gestureStart = Clock::now();
		gestureSeconds = clipSeconds;
		Log("gesture {} ({}, {}): {}={} set {}, {}={} set {}, OffsetGPMA accepted {}", value, left ? "left arm" : "upper body",
			straight ? "straight ahead" : left ? "on the left" : "on the right", kArmVariable, arms, arm, kClipVariable, value, clip, sent);
		if (!sent) {
			a_player->SetGraphVariableInt(kClipVariable, 0);
			a_player->SetGraphVariableInt(kArmVariable, 0);
		}
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
