#include "QuestTrack.h"

#include "Settings.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		constexpr auto kOfferWindow = 15s;
		constexpr auto kQuestScript = "Quest"sv;

		std::uint64_t PackObjective(RE::FormID a_questID, std::uint16_t a_objectiveIndex)
		{
			return (static_cast<std::uint64_t>(a_questID) << 16) | a_objectiveIndex;
		}

		std::string QuestLabel(RE::TESQuest* a_quest, std::uint16_t a_objectiveIndex)
		{
			if (const char* name = a_quest->GetName(); name && *name) {
				return name;
			}
			for (const auto* objective : a_quest->objectives) {
				if (objective && objective->index == a_objectiveIndex && !objective->displayText.empty()) {
					return objective->displayText.c_str();
				}
			}
			return "퀘스트";
		}

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

	QuestTrack::QuestTrack()
	{
		track.SetPromptType(SkyPromptAPI::kHold);
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
		offeredLabel.clear();
		expiresAt = {};
		pendingObjective = 0;
		eventTaskQueued = false;
		Log("ready");
	}

	void QuestTrack::Tick() {}

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
			offeredLabel.clear();
		}
		track.Update(available, [this] { return std::format("추적하기 (길게): {}", offeredLabel); });
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
		offeredLabel.clear();
		pendingObjective = 0;
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

		pendingObjective = PackObjective(a_event->objective->ownerQuest->GetFormID(), a_event->objective->index);
		if (!eventTaskQueued.exchange(true)) {
			SKSE::GetTaskInterface()->AddTask([] {
				auto* self = QuestTrack::GetSingleton();
				self->eventTaskQueued = false;
				const auto pending = self->pendingObjective.exchange(0);
				if (pending != 0) {
					self->ReceiveDisplayedObjective(
						static_cast<RE::FormID>(pending >> 16), static_cast<std::uint16_t>(pending));
				}
			});
		}
		return RE::BSEventNotifyControl::kContinue;
	}

	void QuestTrack::ReceiveDisplayedObjective(RE::FormID a_questID, std::uint16_t a_objectiveIndex)
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
		offeredLabel = QuestLabel(quest, a_objectiveIndex);
		expiresAt = Clock::now() + kOfferWindow;
		Log("new objective: quest={} ({:08X}) objective={} label='{}', offering for {}s",
			Util::NameOf(quest), a_questID, a_objectiveIndex, offeredLabel,
			std::chrono::duration_cast<std::chrono::seconds>(kOfferWindow).count());
	}
}
