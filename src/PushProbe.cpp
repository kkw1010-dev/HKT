#include "PushProbe.h"

#include "Util.h"

namespace CIGAR::PushProbe
{
	namespace
	{
		using Clock = std::chrono::steady_clock;

		template <class... Args>
		void Log(std::format_string<Args...> a_fmt, Args&&... a_args)
		{
			logs::info("[PushProbe] {}", std::format(a_fmt, std::forward<Args>(a_args)...));
		}

		constexpr float kWatchRange = 400.0f;    // natural bumps are looked for this close to the player
		constexpr float kTopicRange = 1500.0f;   // dialogue lines are logged from this close
		constexpr float kDisplaced = 20.0f;      // an NPC that moved this far counts as displaced
		constexpr auto kWindow = 3000ms;         // how long one bump is followed
		constexpr std::array kSamplesAt{ 250ms, 500ms, 1000ms, 2000ms };
		constexpr int kMaxAnimEvents = 40;

		// The four IDLE children of BumpedIntoRoot (Skyrim.esm 03DE4E), chosen by GetLastBumpDirection.
		constexpr const char* kFrontEvent = "NPC_BumpFromFront";
		constexpr const char* kBackEvent = "NPC_BumpedFromBack";
		constexpr const char* kLeftEvent = "NPC_BumpedFromLeft";
		constexpr const char* kRightEvent = "NPC_BumpedFromRight";

		std::atomic<int> armed{ -1 };  // a Kind, or -1
		std::atomic_bool watching{ false };

		// Read by the event sinks (other threads); written on the game thread.
		std::atomic<const RE::TESObjectREFR*> tracked{ nullptr };  // compared only, never dereferenced
		std::atomic<const RE::TESObjectREFR*> gesturePlayer{ nullptr };  // the player while a gesture plays
		std::atomic<Clock::rep> gestureStartTicks{ 0 };
		std::atomic<int> gestureEvents{ 0 };
		std::atomic<Clock::rep> windowStart{ 0 };
		std::atomic<int> animEvents{ 0 };
		std::atomic<int> topicLines{ 0 };

		std::mutex statusLock;
		std::string status = "idle";

		void SetStatus(std::string a_text)
		{
			std::scoped_lock lock(statusLock);
			status = std::move(a_text);
		}

		float Since()
		{
			const auto start = Clock::time_point(Clock::duration(windowStart.load()));
			return std::chrono::duration<float>(Clock::now() - start).count();
		}

		const char* KindName(Kind a_kind)
		{
			switch (a_kind) {
			case Kind::kAction:
				return "ActionBumpedInto";
			case Kind::kFront:
				return kFrontEvent;
			case Kind::kBack:
				return kBackEvent;
			case Kind::kLeft:
				return kLeftEvent;
			case Kind::kRight:
				return kRightEvent;
			}
			return "?";
		}

		const char* BumpName(int a_state)
		{
			switch (a_state) {
			case -2:
				return "no high process";
			case -1:
				return "none";
			case 0:
				return "small";
			case 1:
				return "big";
			default:
				return "unknown";
			}
		}

		std::string Point(const RE::NiPoint3& a_p)
		{
			return std::format("({:.0f}, {:.0f}, {:.0f})", a_p.x, a_p.y, a_p.z);
		}

		// Everything read here is read only; the probe writes no engine value.
		struct Snapshot
		{
			RE::NiPoint3 pos;
			int bumpedState = -2;
			float bumpTimer = 0.0f;
			float lastBumpDirection = 0.0f;
			bool hostile = false;
			bool combat = false;
			std::uint32_t crimeGold = 0;
		};

		Snapshot Take(RE::Actor* a_actor, RE::PlayerCharacter* a_player)
		{
			Snapshot s;
			s.pos = a_actor->GetPosition();
			auto* process = a_actor->GetActorRuntimeData().currentProcess;
			if (const auto* high = process ? process->high : nullptr) {
				s.bumpedState = static_cast<int>(high->bumpedState);
				s.bumpTimer = high->bumpTimer.timeStamp;
				s.lastBumpDirection = high->lastBumpDirection;
			}
			s.hostile = a_actor->IsHostileToActor(a_player);
			s.combat = a_actor->IsInCombat();
			if (const auto* faction = a_actor->GetCrimeFaction()) {
				s.crimeGold = a_player->GetCrimeGoldValue(faction);
			}
			return s;
		}

		std::string Describe(const Snapshot& a_s)
		{
			return std::format("bump {} (direction {:.0f}, timer {:.2f}), hostile {}, combat {}, crime gold {}, at {}",
				BumpName(a_s.bumpedState), a_s.lastBumpDirection, a_s.bumpTimer, a_s.hostile, a_s.combat, a_s.crimeGold,
				Point(a_s.pos));
		}

		std::string Movement(RE::PlayerCharacter* a_player)
		{
			const auto* state = a_player->AsActorState();
			return state->IsSprinting() ? "sprinting" : state->IsRunning() ? "running" : state->IsWalking() ? "walking" : "standing";
		}

		class AnimSink final : public RE::BSTEventSink<RE::BSAnimationGraphEvent>
		{
		public:
			static AnimSink* Get()
			{
				static AnimSink sink;
				return &sink;
			}

			RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>*) override
			{
				if (a_event && a_event->holder && a_event->holder == tracked.load()) {
					if (const int n = ++animEvents; n <= kMaxAnimEvents) {
						Log("  anim event +{:.2f}s: {}{}", Since(), a_event->tag.c_str(),
							a_event->payload.empty() ? "" : std::format(" ({})", a_event->payload.c_str()));
					}
				}
				if (a_event && a_event->holder && a_event->holder == gesturePlayer.load()) {
					if (const int n = ++gestureEvents; n <= kMaxAnimEvents) {
						const auto start = Clock::time_point(Clock::duration(gestureStartTicks.load()));
						Log("  player anim event +{:.2f}s: {}{}", std::chrono::duration<float>(Clock::now() - start).count(),
							a_event->tag.c_str(), a_event->payload.empty() ? "" : std::format(" ({})", a_event->payload.c_str()));
					}
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		struct Window
		{
			RE::ActorHandle actor;
			std::string name;
			std::string cause;
			Clock::time_point start;
			Snapshot before;
			RE::NiPoint3 playerBefore;
			std::size_t nextSample = 0;
			float maxMove = 0.0f;
			bool open = false;
		};
		Window window;  // game thread

		void Close(RE::Actor* a_actor, RE::PlayerCharacter* a_player, std::string_view a_why)
		{
			Snapshot after = window.before;
			if (a_actor) {
				after = Take(a_actor, a_player);
				a_actor->RemoveAnimationGraphEventSink(AnimSink::Get());
			}
			tracked = nullptr;
			const float moved = after.pos.GetDistance(window.before.pos);
			std::string problems;
			if (after.hostile && !window.before.hostile) {
				problems += " HOSTILE";
			}
			if (after.combat && !window.before.combat) {
				problems += " COMBAT";
			}
			if (after.crimeGold > window.before.crimeGold) {
				problems += std::format(" CRIME(+{})", after.crimeGold - window.before.crimeGold);
			}
			const bool displaced = window.maxMove >= kDisplaced;
			Log("RESULT {} on {} ({}): moved {:.0f} units at the end, {:.0f} at most -> {}; anim events {}; dialogue lines {}; "
				"hostile {}->{}, combat {}->{}, crime gold {}->{}; {}",
				window.cause, window.name, a_why, moved, window.maxMove, displaced ? "DISPLACED" : "NOT DISPLACED", animEvents.load(),
				topicLines.load(), window.before.hostile, after.hostile, window.before.combat, after.combat, window.before.crimeGold,
				after.crimeGold, problems.empty() ? "no hostility, combat or crime" : "PROBLEM:" + problems);
			SetStatus(std::format("last: {} on {} -> {}, max {:.0f} units, {} anim events, {} lines{}", window.cause, window.name,
				displaced ? "displaced" : "not displaced", window.maxMove, animEvents.load(), topicLines.load(), problems));
			window.open = false;
		}

		void Open(RE::Actor* a_actor, RE::PlayerCharacter* a_player, std::string a_cause, const Snapshot& a_before)
		{
			if (window.open) {
				auto previous = window.actor.get();
				Close(previous.get(), a_player, "replaced by a new bump");
			}
			window = {};
			window.actor = a_actor->GetHandle();
			window.name = std::format("{} {:08X}", Util::NameOf(a_actor), a_actor->GetFormID());
			window.cause = std::move(a_cause);
			window.start = Clock::now();
			window.before = a_before;
			window.playerBefore = a_player->GetPosition();
			window.open = true;
			windowStart = window.start.time_since_epoch().count();
			animEvents = 0;
			topicLines = 0;
			tracked = a_actor;
			const bool sink = a_actor->AddAnimationGraphEventSink(AnimSink::Get());
			Log("{} on {}: {:.0f} units from the player ({}), before: {}; anim sink {}", window.cause, window.name,
				a_before.pos.GetDistance(window.playerBefore), Movement(a_player), Describe(a_before), sink ? "added" : "NOT added");
		}

		void Fire(Kind a_kind, RE::PlayerCharacter* a_player)
		{
			RE::ObjectRefHandle aimed;
			if (auto* pick = RE::CrosshairPickData::GetSingleton()) {
				aimed = pick->GetActiveTarget();
			}
			const auto ref = aimed.get();
			auto* actor = ref ? ref->As<RE::Actor>() : nullptr;
			if (!actor || actor == a_player || actor->IsDead()) {
				Log("{}: no living NPC under the crosshair; nothing sent", KindName(a_kind));
				SetStatus(std::format("{}: no NPC under the crosshair", KindName(a_kind)));
				return;
			}
			Open(actor, a_player, std::format("fired {}", KindName(a_kind)), Take(actor, a_player));
			if (a_kind != Kind::kAction) {
				const bool accepted = actor->NotifyAnimationGraph(KindName(a_kind));
				Log("graph event {} sent: accepted {}", KindName(a_kind), accepted);
				return;
			}
			auto* action = RE::BGSDefaultObjectManager::GetSingleton()->GetObject<RE::BGSAction>(RE::DEFAULT_OBJECT::kActionBumpedInto);
			if (!action) {
				Log("WARN default object ActionBumpedInto is missing; nothing sent");
				return;
			}
			auto* data = RE::TESActionData::Create();
			if (!data) {
				Log("WARN TESActionData::Create failed; nothing sent");
				return;
			}
			data->source = RE::NiPointer<RE::TESObjectREFR>(actor);
			data->target = RE::NiPointer<RE::TESObjectREFR>(a_player);
			data->action = action;
			const bool processed = data->Process();
			data->~TESActionData();
			RE::free(data);
			Log("action {:08X} performed with the NPC as source and the player as target: Process() returned {}", action->GetFormID(), processed);
		}

		void Sample(RE::PlayerCharacter* a_player)
		{
			const auto actor = window.actor.get();
			if (!actor) {
				Close(nullptr, a_player, "the NPC is gone");
				return;
			}
			const auto elapsed = Clock::now() - window.start;
			const float moved = actor->GetPosition().GetDistance(window.before.pos);
			window.maxMove = std::max(window.maxMove, moved);
			while (window.nextSample < kSamplesAt.size() && elapsed >= kSamplesAt[window.nextSample]) {
				Log("  +{}ms: moved {:.0f} units (max {:.0f}), player moved {:.0f}; {}", kSamplesAt[window.nextSample].count(), moved,
					window.maxMove, a_player->GetPosition().GetDistance(window.playerBefore), Describe(Take(actor.get(), a_player)));
				++window.nextSample;
			}
			if (elapsed >= kWindow) {
				Close(actor.get(), a_player, "window over");
			}
		}

		struct Seen
		{
			int state;
			float timer;
			RE::NiPoint3 pos;
		};
		std::unordered_map<RE::FormID, Seen> seen;  // game thread

		void Watch(RE::PlayerCharacter* a_player)
		{
			auto* lists = RE::ProcessLists::GetSingleton();
			if (!lists) {
				return;
			}
			const auto playerPos = a_player->GetPosition();
			for (auto& handle : lists->highActorHandles) {
				const auto actor = handle.get();
				if (!actor || actor.get() == a_player || actor->IsDead()) {
					continue;
				}
				const auto id = actor->GetFormID();
				if (actor->GetPosition().GetDistance(playerPos) > kWatchRange) {
					seen.erase(id);
					continue;
				}
				const auto now = Take(actor.get(), a_player);
				if (const auto it = seen.find(id); it != seen.end() && now.bumpedState != it->second.state) {
					Log("natural bump on {} {:08X}: state {} -> {}, direction {:.0f}, timer {:.2f} -> {:.2f}; player {}", Util::NameOf(actor.get()), id,
						BumpName(it->second.state), BumpName(now.bumpedState), now.lastBumpDirection, it->second.timer, now.bumpTimer,
						Movement(a_player));
					if (now.bumpedState >= 0 && !window.open) {
						Snapshot before = now;
						before.pos = it->second.pos;  // where it stood one tick earlier
						Open(actor.get(), a_player, std::format("natural {} bump", BumpName(now.bumpedState)), before);
					}
				}
				seen[id] = { now.bumpedState, now.bumpTimer, now.pos };
			}
		}

		// Stage 0b/0c: the clips of tools/push_test_assets.py; values and lengths must match it. A value of 0
		// is the automatic pick by the gap (stage 0c, the user's two options of 2026-09-28).
		constexpr std::array kGestures{
			Gesture{ 3801, 2, 2.67f, "3801 문 열기 · 왼팔" },
			Gesture{ 3802, 2, 0.33f, "3802 방패 밀치기 · 왼팔" },
			Gesture{ 3803, 1, 2.33f, "3803 받기 · 오른팔" },
			Gesture{ 3804, 1, 2.33f, "3804 건네기 · 오른팔" },
			Gesture{ 3805, 0, 2.87f, "3805 EVG 비집기 · 상체" },
			Gesture{ 3806, 2, 2.87f, "3806 EVG 비집기 · 왼팔" },
			Gesture{ 3805, 0, 0.30f, "3805 상체 · 0.3초에 끊기" },
			Gesture{ 3805, 0, 0.50f, "3805 상체 · 0.5초에 끊기" },
			Gesture{ 3805, 0, 0.80f, "3805 상체 · 0.8초에 끊기" },
			Gesture{ 0, 0, 0.0f, "간격별 자동 (정면 3805 · 왼쪽 3806)" },
		};
		constexpr int kFull3805 = 4;
		constexpr int kLeft3806 = 5;
		// Probe values (Claude's, not the user's): an NPC up to this far ahead and this far to either side
		// is in the way; within kAutoStraight of the line ahead it is straight ahead.
		constexpr float kAutoAhead = 120.0f;
		constexpr float kAutoCorridor = 80.0f;
		constexpr float kAutoStraight = 30.0f;
		constexpr float kKeptSpeed = 40.0f;  // average speed during the clip that still counts as walking on
		constexpr auto kClipVariable = "iGPMAAnimationType";
		constexpr auto kArmVariable = "iGPMAOffsetType";
		constexpr auto kArmWait = 20s;         // an armed gesture is dropped if the player does not move by then
		constexpr auto kAfterStop = 500ms;     // speed is measured this long after the stop

		std::atomic<int> gestureArmed{ -1 };  // an index into kGestures, or -1

		struct GestureRun
		{
			int index = -1;
			std::string note;  // why the automatic pick chose this clip
			Clock::time_point armed;
			Clock::time_point start;
			RE::NiPoint3 startPos;
			float speedBefore = 0.0f;
			float sumSpeed = 0.0f;
			int samples = 0;
			bool waiting = false;
			bool playing = false;
			bool stopped = false;
			Clock::time_point stopAt;
			RE::NiPoint3 stopPos;
		};
		GestureRun run;  // game thread
		RE::NiPoint3 lastPlayerPos;
		Clock::time_point lastPlayerTick;

		bool Moving(RE::PlayerCharacter* a_player)
		{
			const auto* state = a_player->AsActorState();
			return state->IsWalking() || state->IsRunning() || state->IsSprinting();
		}

		// The nearest living, non-hostile NPC in the way: ahead of the player, inside the corridor.
		RE::Actor* InTheWay(RE::PlayerCharacter* a_player, float& a_ahead, float& a_side)
		{
			auto* lists = RE::ProcessLists::GetSingleton();
			if (!lists) {
				return nullptr;
			}
			const auto pos = a_player->GetPosition();
			const float heading = a_player->GetAngleZ();
			const RE::NiPoint3 forward{ std::sin(heading), std::cos(heading), 0.0f };
			const RE::NiPoint3 right{ std::cos(heading), -std::sin(heading), 0.0f };
			RE::Actor* best = nullptr;
			for (auto& handle : lists->highActorHandles) {
				const auto actor = handle.get();
				if (!actor || actor.get() == a_player || actor->IsDead() || actor->IsHostileToActor(a_player) ||
					!actor->HasKeywordString("ActorTypeNPC")) {
					continue;
				}
				const auto to = actor->GetPosition() - pos;
				const float ahead = to.x * forward.x + to.y * forward.y;
				const float side = to.x * right.x + to.y * right.y;
				if (ahead > 0.0f && ahead <= kAutoAhead && std::abs(side) <= kAutoCorridor && (!best || ahead < a_ahead)) {
					best = actor.get();
					a_ahead = ahead;
					a_side = side;
				}
			}
			return best;
		}

		void Result(RE::PlayerCharacter* a_player, float a_speedAfter, std::string_view a_how)
		{
			const auto& g = kGestures[run.index];
			a_player->RemoveAnimationGraphEventSink(AnimSink::Get());
			gesturePlayer = nullptr;
			const float during = run.samples ? run.sumSpeed / static_cast<float>(run.samples) : 0.0f;
			const bool kept = during >= kKeptSpeed;
			const std::string how = run.note.empty() ? std::string(a_how) : std::format("[{}] {}", run.note, a_how);
			Log("RESULT gesture {} {}: speed before {:.0f}, during {:.0f}, after {:.0f} ({}); moved {:.0f} units while playing; "
				"player anim events {}; {}",
				g.label, how, run.speedBefore, during, a_speedAfter, Movement(a_player), run.stopPos.GetDistance(run.startPos),
				gestureEvents.load(), kept ? "KEPT MOVING" : "MOVEMENT DROPPED (average below walking)");
			SetStatus(std::format("gesture {}: {}, speed during {:.0f}", g.label, kept ? "kept moving" : "movement dropped", during));
			run = {};
		}

		void Stop(RE::PlayerCharacter* a_player, const RE::NiPoint3& a_pos, std::string_view a_why)
		{
			const auto& g = kGestures[run.index];
			const bool sent = a_player->NotifyAnimationGraph("OffsetGPMAStop");
			const bool clip = a_player->SetGraphVariableInt(kClipVariable, 0);
			const bool arm = a_player->SetGraphVariableInt(kArmVariable, 0);
			run.playing = false;
			run.stopped = true;
			run.stopAt = Clock::now();
			run.stopPos = a_pos;
			Log("gesture {} stop after {:.2f} s ({}): OffsetGPMAStop accepted {}, {}=0 set {}, {}=0 set {}", g.label,
				std::chrono::duration<float>(run.stopAt - run.start).count(), a_why, sent, kClipVariable, clip, kArmVariable, arm);
		}

		void Start(RE::PlayerCharacter* a_player, const RE::NiPoint3& a_pos, float a_speed, const Clock::time_point& a_now)
		{
			const auto& g = kGestures[run.index];
			bool installed = false;
			const bool read = a_player->GetGraphVariableBool("bGPMAInstalled", installed);
			const bool arm = a_player->SetGraphVariableInt(kArmVariable, g.offsetType);
			const bool clip = a_player->SetGraphVariableInt(kClipVariable, g.value);
			gesturePlayer = a_player;
			gestureStartTicks = a_now.time_since_epoch().count();
			gestureEvents = 0;
			const bool sink = a_player->AddAnimationGraphEventSink(AnimSink::Get());
			const bool sent = a_player->NotifyAnimationGraph("OffsetGPMA");
			run.waiting = false;
			run.playing = true;
			run.start = a_now;
			run.startPos = a_pos;
			run.speedBefore = a_speed;
			Log("gesture {} start ({}): bGPMAInstalled {} (read {}), {}={} set {}, {}={} set {}, OffsetGPMA accepted {}, sink {}, "
				"speed before {:.0f}", g.label, Movement(a_player), installed, read, kArmVariable, g.offsetType, arm, kClipVariable, g.value,
				clip, sent, sink, a_speed);
		}

		void GestureTick(RE::PlayerCharacter* a_player)
		{
			const auto now = Clock::now();
			const auto pos = a_player->GetPosition();
			// Speed from the last tick (units per second), whatever the state flags say.
			const float dt = std::chrono::duration<float>(now - lastPlayerTick).count();
			const float speed = dt > 0.0f && dt < 1.0f ? pos.GetDistance(lastPlayerPos) / dt : 0.0f;
			lastPlayerPos = pos;
			lastPlayerTick = now;

			if (const int index = gestureArmed.exchange(-1); index >= 0) {
				// r5 fault: a new arm used to drop a playing gesture without its stop or result.
				if (run.playing) {
					Stop(a_player, pos, "interrupted by a new arm");
				}
				if (run.stopped) {
					Result(a_player, speed, "cut short by a new arm");
				}
				run = {};
				run.index = index;
				run.armed = now;
				run.waiting = true;
				Log("gesture {} armed: plays when the player moves{}", kGestures[index].label,
					kGestures[index].value == 0 ? " and a non-hostile NPC is in the way" : "");
			}
			if (run.index < 0) {
				return;
			}
			if (run.waiting) {
				if (now - run.armed > kArmWait) {
					Log("gesture {} dropped: nothing to play within {} s", kGestures[run.index].label, kArmWait.count());
					SetStatus(std::format("gesture {}: dropped", kGestures[run.index].label));
					run = {};
					return;
				}
				if (!Moving(a_player)) {
					return;
				}
				if (kGestures[run.index].value == 0) {
					float ahead = 0.0f;
					float side = 0.0f;
					auto* npc = InTheWay(a_player, ahead, side);
					if (!npc) {
						return;
					}
					const bool straight = std::abs(side) <= kAutoStraight;
					const bool left = !straight && side < 0.0f;
					const auto* label = kGestures[run.index].label;
					run.index = left ? kLeft3806 : kFull3805;
					run.note = std::format("auto: {} {:.0f} ahead, {:+.0f} to the side, {}", Util::NameOf(npc), ahead, side,
						straight ? "straight ahead" : left ? "on the left" : "on the right (3805 stands in)");
					Log("gesture {} picked {}: {}", label, kGestures[run.index].label, run.note);
				}
				Start(a_player, pos, speed, now);
				return;
			}
			if (run.playing) {
				run.sumSpeed += speed;
				++run.samples;
				if (now - run.start >= std::chrono::duration<float>(kGestures[run.index].seconds)) {
					Stop(a_player, pos, "clip length");
				}
				return;
			}
			if (run.stopped && now - run.stopAt >= kAfterStop) {
				Result(a_player, speed, "complete");
			}
		}

		// Stage 0d: squeeze past a touching NPC. Probe values (Claude's, not the user's).
		constexpr float kShrinkFactor = 0.5f;     // (가) the player's capsule radius is multiplied by this
		constexpr float kContactAhead = 75.0f;    // an NPC this close ahead counts as touching when no bump is reported
		constexpr float kClearPast = 30.0f;       // the player is past the NPC once this far beyond it
		constexpr float kClearDistance = 70.0f;   // and this far from it: then the change is undone
		constexpr auto kSqueezeLimit = 3s;        // undone after this at the latest
		constexpr auto kSqueezeArmWait = 30s;
		constexpr auto kAfterRestore = 600ms;     // speed and position are followed this long after the undo
		constexpr float kDoorRange = 200.0f;
		constexpr int kAutoGesture = static_cast<int>(kGestures.size()) - 1;

		std::atomic<int> squeezeArmed{ -1 };

		const char* SqueezeName(int a_how)
		{
			return a_how == static_cast<int>(Squeeze::kShrink) ? "(가) 캡슐 축소" : "(나) 그 NPC 충돌 끄기";
		}

		struct SqueezeRun
		{
			int how = -1;
			bool waiting = false;
			bool active = false;
			bool after = false;
			Clock::time_point armed;
			Clock::time_point start;
			Clock::time_point restoredAt;
			RE::ActorHandle npc;
			std::string npcName;
			RE::NiPoint3 playerStart;
			RE::NiPoint3 npcStart;
			RE::NiPoint3 axis;  // horizontal unit vector from the player to the NPC at the start
			RE::NiPoint3 lastPos;
			Clock::time_point lastTick;
			RE::NiPoint3 restorePos;
			RE::NiPoint3 npcAtRestore;
			float speedAtRestore = 0.0f;
			float maxPast = -1.0e9f;
			bool passed = false;
			float slowAfter = 0.0f;  // seconds after the undo spent below 20 u/s while moving
			// (가): each capsule changed, kept alive by its bhkShape, with its old radius.
			std::vector<std::tuple<RE::NiPointer<RE::bhkShape>, RE::hkpConvexShape*, float>> radii;
			// (나): the NPC's controller body, kept alive, and its old filter word.
			RE::hkRefPtr<RE::hkpRigidBody> body;
			std::uint32_t oldFilter = 0;
		};
		SqueezeRun squeeze;  // game thread

		float Past(const RE::NiPoint3& a_player, const RE::NiPoint3& a_npc)
		{
			return (a_player.x - a_npc.x) * squeeze.axis.x + (a_player.y - a_npc.y) * squeeze.axis.y;
		}

		float Flat(const RE::NiPoint3& a_a, const RE::NiPoint3& a_b)
		{
			return std::hypot(a_a.x - a_b.x, a_a.y - a_b.y);
		}

		// The NPC the player's controller reports bumping, or, failing that, one right ahead.
		RE::Actor* Touching(RE::PlayerCharacter* a_player, std::string& a_signal)
		{
			auto* controller = a_player->GetCharController();
			if (controller && controller->bumpedCharCollisionObject.get()) {
				if (auto* ref = RE::TESHavokUtilities::FindCollidableRef(*controller->bumpedCharCollisionObject->GetCollidable())) {
					if (auto* actor = ref->As<RE::Actor>(); actor && actor != a_player && !actor->IsDead()) {
						a_signal = std::format("the controller's bumped character (force {:.2f})", controller->bumpedForce);
						return actor;
					}
				}
			}
			float ahead = 0.0f;
			float side = 0.0f;
			if (auto* actor = InTheWay(a_player, ahead, side); actor && ahead <= kContactAhead) {
				a_signal = std::format("{:.0f} ahead, {:+.0f} to the side (no controller bump reported)", ahead, side);
				return actor;
			}
			return nullptr;
		}

		std::string Surroundings(RE::PlayerCharacter* a_player)
		{
			float nearest = kDoorRange + 1.0f;
			std::string door;
			if (auto* tes = RE::TES::GetSingleton()) {
				const auto pos = a_player->GetPosition();
				tes->ForEachReferenceInRange(a_player, kDoorRange, [&](RE::TESObjectREFR* a_ref) {
					const auto* base = a_ref ? a_ref->GetBaseObject() : nullptr;
					if (base && base->Is(RE::FormType::Door)) {
						if (const float d = a_ref->GetPosition().GetDistance(pos); d < nearest) {
							nearest = d;
							door = Util::NameOf(base);
						}
					}
					return RE::BSContainer::ForEachResult::kContinue;
				});
			}
			const auto* cell = a_player->GetParentCell();
			return std::format("{} cell {}; nearest door {}", cell && cell->IsInteriorCell() ? "interior" : "exterior",
				cell ? Util::NameOf(cell) : "?"s,
				nearest <= kDoorRange ? std::format("'{}' {:.0f} away", door, nearest) : std::format("none within {:.0f}", kDoorRange));
		}

		std::string Shrink(RE::PlayerCharacter* a_player, RE::Actor* a_npc)
		{
			auto* controller = a_player->GetCharController();
			if (!controller) {
				return "no player controller: nothing changed";
			}
			const auto* npcController = a_npc->GetCharController();
			std::string out = std::format("controller radius {:.3f} destRadius {:.3f} scale {:.2f}", controller->radius,
				controller->destRadius, controller->scale);
			for (std::size_t i = 0; i < 2; ++i) {
				RE::NiPointer<RE::bhkShape> shape = controller->shapes[i];
				auto* hk = shape ? static_cast<RE::hkpShape*>(shape->referencedObject.get()) : nullptr;
				if (!hk) {
					out += std::format("; shape{} none", i);
					continue;
				}
				if (hk->type != RE::hkpShapeType::kCapsule) {
					out += std::format("; shape{} type {} (not a capsule, left alone)", i, static_cast<int>(hk->type));
					continue;
				}
				auto* capsule = static_cast<RE::hkpConvexShape*>(hk);
				const bool again = std::ranges::any_of(squeeze.radii, [&](const auto& a_entry) { return std::get<1>(a_entry) == capsule; });
				const bool shared = npcController && (npcController->shapes[0].get() == shape.get() || npcController->shapes[1].get() == shape.get());
				if (again) {
					out += std::format("; shape{} is shape0 again", i);
					continue;
				}
				const float old = capsule->radius;
				capsule->radius = old * kShrinkFactor;
				squeeze.radii.emplace_back(shape, capsule, old);
				out += std::format("; shape{} capsule radius {:.3f} -> {:.3f}{}", i, old, capsule->radius,
					shared ? " (the NPC's controller uses this shape too)" : "");
			}
			return out;
		}

		std::string Ghost(RE::Actor* a_npc)
		{
			auto* npcController = a_npc->GetCharController();
			auto* body = npcController ? npcController->GetRigidBody() : nullptr;
			if (!body) {
				return "the NPC's controller has no body: nothing changed";
			}
			auto& filter = body->GetCollidableRW()->broadPhaseHandle.collisionFilterInfo;
			squeeze.body = RE::hkRefPtr<RE::hkpRigidBody>(body);
			squeeze.oldFilter = filter.filter;
			filter.SetNoCollision(true);
			return std::format("NPC body filter {:08X} -> {:08X} (layer {}, system group {}), NPC z {:.0f}", squeeze.oldFilter, filter.filter,
				static_cast<int>(filter.GetCollisionLayer()), filter.GetSystemGroup(), a_npc->GetPosition().z);
		}

		std::string Undo()
		{
			std::string out;
			for (auto& [shape, capsule, old] : squeeze.radii) {
				out += std::format("{}capsule radius {:.3f} -> {:.3f}", out.empty() ? "" : "; ", capsule->radius, old);
				capsule->radius = old;
			}
			squeeze.radii.clear();
			if (auto* body = squeeze.body.get()) {
				auto& filter = body->GetCollidableRW()->broadPhaseHandle.collisionFilterInfo;
				out += std::format("{}NPC body filter {:08X} -> {:08X}", out.empty() ? "" : "; ", filter.filter, squeeze.oldFilter);
				filter.filter = squeeze.oldFilter;
				squeeze.body = {};
			}
			return out.empty() ? "nothing to undo" : out;
		}

		void SqueezeTick(RE::PlayerCharacter* a_player)
		{
			const auto now = Clock::now();
			const auto pos = a_player->GetPosition();
			const float dt = std::chrono::duration<float>(now - squeeze.lastTick).count();
			const float speed = dt > 0.0f && dt < 1.0f ? Flat(pos, squeeze.lastPos) / dt : 0.0f;
			squeeze.lastPos = pos;
			squeeze.lastTick = now;

			if (const int how = squeezeArmed.exchange(-1); how >= 0) {
				if (squeeze.active) {
					Log("squeeze {}: interrupted by a new arm; {}", SqueezeName(squeeze.how), Undo());
				}
				squeeze = {};
				squeeze.how = how;
				squeeze.waiting = true;
				squeeze.armed = now;
				squeeze.lastPos = pos;
				squeeze.lastTick = now;
				Log("squeeze {} armed: applies when the player touches an NPC while moving (within {} s)", SqueezeName(how), kSqueezeArmWait.count());
				SetStatus(std::format("squeeze {} armed: walk into an NPC", SqueezeName(how)));
				return;
			}
			if (squeeze.waiting) {
				if (now - squeeze.armed > kSqueezeArmWait) {
					Log("squeeze {} dropped: no NPC touched within {} s", SqueezeName(squeeze.how), kSqueezeArmWait.count());
					SetStatus(std::format("squeeze {}: dropped", SqueezeName(squeeze.how)));
					squeeze = {};
					return;
				}
				if (!Moving(a_player)) {
					return;
				}
				std::string signal;
				auto* npc = Touching(a_player, signal);
				if (!npc) {
					return;
				}
				squeeze.waiting = false;
				squeeze.active = true;
				squeeze.start = now;
				squeeze.npc = npc->GetHandle();
				squeeze.npcName = Util::NameOf(npc);
				squeeze.playerStart = pos;
				squeeze.npcStart = npc->GetPosition();
				const float d = std::max(Flat(squeeze.npcStart, pos), 1.0f);
				squeeze.axis = { (squeeze.npcStart.x - pos.x) / d, (squeeze.npcStart.y - pos.y) / d, 0.0f };
				const auto change = squeeze.how == static_cast<int>(Squeeze::kShrink) ? Shrink(a_player, npc) : Ghost(npc);
				Log("squeeze {} start: touching {} {:08X} at {:.0f} via {}; {}; {}", SqueezeName(squeeze.how), squeeze.npcName, npc->GetFormID(), d,
					signal, Surroundings(a_player), change);
				gestureArmed = kAutoGesture;
				SetStatus(std::format("squeeze {}: on {}", SqueezeName(squeeze.how), squeeze.npcName));
				return;
			}
			if (squeeze.active) {
				const auto npc = squeeze.npc.get();
				const auto npcPos = npc ? npc->GetPosition() : squeeze.npcStart;
				const float past = Past(pos, npcPos);
				const float gap = Flat(pos, npcPos);
				squeeze.maxPast = std::max(squeeze.maxPast, past);
				squeeze.passed = squeeze.passed || past >= kClearPast;
				const float t = std::chrono::duration<float>(now - squeeze.start).count();
				Log("  squeeze +{:.1f}s: {:.0f} from the NPC, {:+.0f} past it, speed {:.0f}{}", t, gap, past, speed, Moving(a_player) ? "" : " (not moving)");
				const bool clear = squeeze.passed && gap >= kClearDistance;
				if (!clear && now - squeeze.start < kSqueezeLimit) {
					return;
				}
				squeeze.active = false;
				squeeze.after = true;
				squeeze.restoredAt = now;
				squeeze.restorePos = pos;
				squeeze.npcAtRestore = npcPos;
				squeeze.speedAtRestore = speed;
				Log("squeeze {} undo ({}): {:.0f} from the NPC, {:+.0f} past it; {}", SqueezeName(squeeze.how),
					clear ? "clear of the NPC" : std::format("time limit {} s", kSqueezeLimit.count()), gap, past, Undo());
				return;
			}
			if (squeeze.after) {
				if (Moving(a_player) && speed < 20.0f) {
					squeeze.slowAfter += dt;
				}
				if (now - squeeze.restoredAt < kAfterRestore) {
					return;
				}
				const float span = std::chrono::duration<float>(now - squeeze.restoredAt).count();
				const float moved = Flat(pos, squeeze.restorePos);
				const float expected = squeeze.speedAtRestore * span;
				const auto npc = squeeze.npc.get();
				const auto npcPos = npc ? npc->GetPosition() : squeeze.npcAtRestore;
				Log("RESULT squeeze {} on {}: {}, moved {:.0f} while applied (max {:+.0f} past the NPC); after the undo the player moved {:.0f} in {:.2f} s "
					"against {:.0f} expected from speed {:.0f} ({}), {:.1f} s stuck while moving; the NPC moved {:.0f} (z {:+.0f}) from where it stood",
					SqueezeName(squeeze.how), squeeze.npcName, squeeze.passed ? "PASSED" : "DID NOT PASS", Flat(squeeze.restorePos, squeeze.playerStart),
					squeeze.maxPast, moved, span, expected, squeeze.speedAtRestore,
					moved > expected + 40.0f ? "POPPED" : "no pop", squeeze.slowAfter, Flat(npcPos, squeeze.npcStart), npcPos.z - squeeze.npcStart.z);
				SetStatus(std::format("squeeze {}: {}", SqueezeName(squeeze.how), squeeze.passed ? "passed" : "did not pass"));
				squeeze = {};
				squeeze.lastPos = pos;
				squeeze.lastTick = now;
			}
		}

		class TopicSink final : public RE::BSTEventSink<RE::TESTopicInfoEvent>
		{
		public:
			static TopicSink* Get()
			{
				static TopicSink sink;
				return &sink;
			}

			RE::BSEventNotifyControl ProcessEvent(const RE::TESTopicInfoEvent* a_event, RE::BSTEventSource<RE::TESTopicInfoEvent>*) override
			{
				const auto* speaker = a_event ? a_event->speakerRef.get() : nullptr;
				auto* player = Util::Player();
				if (!speaker || !player || (!watching && !tracked.load()) || speaker->GetPosition().GetDistance(player->GetPosition()) > kTopicRange) {
					return RE::BSEventNotifyControl::kContinue;
				}
				const bool bumped = speaker == tracked.load();
				if (bumped && a_event->type.all(RE::TESTopicInfoEvent::TopicInfoEventType::kTopicBegin)) {
					++topicLines;
				}
				const auto* info = RE::TESForm::LookupByID<RE::TESTopicInfo>(a_event->topicInfoFormID);
				const auto* topic = info ? info->parentTopic : nullptr;
				Log("  dialogue {} by {} {:08X}{}: info {:08X}, topic {:08X} '{}' subtype {}",
					a_event->type.all(RE::TESTopicInfoEvent::TopicInfoEventType::kTopicBegin) ? "begin" : "end", Util::NameOf(speaker),
					speaker->GetFormID(), bumped ? " (the bumped NPC)" : "", a_event->topicInfoFormID, topic ? topic->GetFormID() : 0,
					topic ? topic->GetFormEditorID() : "", topic ? topic->data.subtype.underlying() : 0);
				return RE::BSEventNotifyControl::kContinue;
			}
		};
	}

	void Arm(Kind a_kind)
	{
		armed = static_cast<int>(a_kind);
		Log("armed {}: fires on the NPC under the crosshair when the game runs again", KindName(a_kind));
		SetStatus(std::format("armed {}: close the menu", KindName(a_kind)));
	}

	void SetWatch(bool a_on)
	{
		watching = a_on;
		Log("watch for natural bumps {}", a_on ? "on" : "off");
	}

	bool Watching()
	{
		return watching.load();
	}

	std::span<const Gesture> Gestures()
	{
		return kGestures;
	}

	void ArmGesture(std::size_t a_index)
	{
		if (a_index >= kGestures.size()) {
			return;
		}
		gestureArmed = static_cast<int>(a_index);
		SetStatus(std::format("gesture {} armed: close the menu and walk", kGestures[a_index].label));
	}

	void ArmSqueeze(Squeeze a_how)
	{
		squeezeArmed = static_cast<int>(a_how);
		SetStatus(std::format("squeeze {} armed: close the menu and walk into an NPC", SqueezeName(static_cast<int>(a_how))));
	}

	void RegisterEvents()
	{
		if (auto* holder = RE::ScriptEventSourceHolder::GetSingleton()) {
			holder->AddEventSink<RE::TESTopicInfoEvent>(TopicSink::Get());
			Log("dialogue event sink registered");
		} else {
			Log("WARN no script event source; dialogue lines will not be logged");
		}
	}

	void Tick()
	{
		auto* player = Util::Player();
		if (!player) {
			return;
		}
		if (const int kind = armed.exchange(-1); kind >= 0) {
			Fire(static_cast<Kind>(kind), player);
		}
		if (window.open) {
			Sample(player);
		}
		if (watching) {
			Watch(player);
		} else if (!seen.empty()) {
			seen.clear();
		}
		SqueezeTick(player);
		GestureTick(player);
	}

	std::string Status()
	{
		std::scoped_lock lock(statusLock);
		return status;
	}
}
