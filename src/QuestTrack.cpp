#include "QuestTrack.h"

#include "Settings.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		constexpr auto kOfferWindow = 15s;
		constexpr auto kQuestScript = "Quest"sv;

		class TrackResult final : public RE::BSScript::IStackCallbackFunctor
		{
		public:
			explicit TrackResult(RE::FormID a_questID) :
				questID(a_questID) {}

			void operator()(RE::BSScript::Variable) override
			{
				const auto id = questID;
				SKSE::GetTaskInterface()->AddTask([id] {
					auto* quest = RE::TESForm::LookupByID<RE::TESQuest>(id);
					const bool active = quest && quest->IsActive();
					QuestTrack::GetSingleton()->Log("SetActive returned: quest={:08X} active={}", id, active);
					if (!active) {
						Util::Notify("CIGAR: 퀘스트 추적 실패. 로그 확인");
					}
				});
			}

			void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}

		private:
			RE::FormID questID;
		};
	}

	QuestTrack* QuestTrack::GetSingleton()
	{
		static QuestTrack singleton;
		return &singleton;
	}

	void QuestTrack::RegisterEvents()
	{
		auto* source = RE::ObjectiveState::GetEventSource();
		if (!source) {
			Log("WARN objective-state event source is unavailable");
			return;
		}
		source->AddEventSink(this);
		Log("objective-state event sink registered");
	}

	void QuestTrack::OnGameLoaded()
	{
		track.Reset();
		lastGate.clear();
		offeredQuest = 0;
		expiresAt = {};
		pendingQuest = 0;
		eventTaskQueued = false;
		Util::WarnIfSIModuleOn("QuestActions.enabled_track", "/MCP/modules/QuestActions/enabled_track");
		Log("ready");
	}

	void QuestTrack::Tick()
	{
		Util::WarnIfSIModuleOn("QuestActions.enabled_track", "/MCP/modules/QuestActions/enabled_track");
	}

	void QuestTrack::FastTick()
	{
		auto* quest = offeredQuest ? RE::TESForm::LookupByID<RE::TESQuest>(offeredQuest) : nullptr;
		const bool expired = offeredQuest != 0 && Clock::now() >= expiresAt;
		const bool active = quest && quest->IsActive();
		const bool available = quest && quest->IsEnabled() && !quest->IsCompleted() && !active && !expired;

		LogGate(std::format("quest={} name='{}' enabled={} complete={} active={} expired={}",
			offeredQuest ? std::format("{:08X}", offeredQuest) : "-"s,
			quest ? Util::NameOf(quest) : "-"s,
			quest && quest->IsEnabled(), quest && quest->IsCompleted(), active, expired));

		if (!available) {
			offeredQuest = 0;
		}
		track.Update(available, [quest] { return std::format("추적하기: {}", Util::NameOf(quest)); });
	}

	void QuestTrack::OnAccepted(std::uint16_t a_eventID)
	{
		if (a_eventID != kTrack || offeredQuest == 0) {
			return;
		}
		auto* quest = RE::TESForm::LookupByID<RE::TESQuest>(offeredQuest);
		if (!quest || !quest->IsEnabled() || quest->IsCompleted() || quest->IsActive()) {
			Log("accept ignored: quest={:08X} exists={} enabled={} complete={} active={}", offeredQuest,
				quest != nullptr, quest && quest->IsEnabled(), quest && quest->IsCompleted(), quest && quest->IsActive());
			return;
		}

		const auto questID = offeredQuest;
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		if (!vm) {
			Log("SetActive(true) not requested: Papyrus VM is unavailable");
			Util::Notify("CIGAR: 퀘스트 추적 호출 실패. 로그 확인");
			return;
		}
		auto* args = RE::MakeFunctionArguments(true);
		RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback{ new TrackResult(questID) };
		const bool queued = vm->DispatchMethodCall2(Util::Handle(quest), kQuestScript, "SetActive", args, callback);
		Log("SetActive(true) requested: quest={} ({:08X}) queued={}", Util::NameOf(quest), questID, queued);
		if (queued) {
			offeredQuest = 0;
		} else {
			Util::Notify("CIGAR: 퀘스트 추적 호출 실패. 로그 확인");
		}
	}

	void QuestTrack::OnDisabled()
	{
		offeredQuest = 0;
		pendingQuest = 0;
		expiresAt = {};
	}

	RE::BSEventNotifyControl QuestTrack::ProcessEvent(
		const RE::ObjectiveState::Event* a_event,
		RE::BSTEventSource<RE::ObjectiveState::Event>*)
	{
		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}
		if (a_event->newState != RE::QUEST_OBJECTIVE_STATE::kDisplayed ||
			a_event->oldState == RE::QUEST_OBJECTIVE_STATE::kDisplayed || !a_event->objective ||
			!a_event->objective->ownerQuest) {
			return RE::BSEventNotifyControl::kContinue;
		}

		pendingQuest = a_event->objective->ownerQuest->GetFormID();
		if (!eventTaskQueued.exchange(true)) {
			SKSE::GetTaskInterface()->AddTask([] {
				auto* self = QuestTrack::GetSingleton();
				self->eventTaskQueued = false;
				const auto questID = self->pendingQuest.exchange(0);
				if (questID != 0) {
					self->ReceiveDisplayedObjective(questID);
				}
			});
		}
		return RE::BSEventNotifyControl::kContinue;
	}

	void QuestTrack::ReceiveDisplayedObjective(RE::FormID a_questID)
	{
		if (!Settings::Enabled(Name())) {
			return;
		}
		auto* quest = RE::TESForm::LookupByID<RE::TESQuest>(a_questID);
		if (!quest || !quest->IsEnabled() || quest->IsCompleted() || quest->IsActive()) {
			Log("objective ignored: quest={:08X} exists={} enabled={} complete={} active={}", a_questID,
				quest != nullptr, quest && quest->IsEnabled(), quest && quest->IsCompleted(), quest && quest->IsActive());
			return;
		}
		if (offeredQuest != a_questID) {
			track.Withdraw();
			track.Reset();
		}
		offeredQuest = a_questID;
		expiresAt = Clock::now() + kOfferWindow;
		Log("new objective: quest={} ({:08X}), offering for {}s", Util::NameOf(quest), a_questID,
			std::chrono::duration_cast<std::chrono::seconds>(kOfferWindow).count());
	}
}
