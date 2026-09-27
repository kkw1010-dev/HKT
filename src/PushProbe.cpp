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
	}

	std::string Status()
	{
		std::scoped_lock lock(statusLock);
		return status;
	}
}
