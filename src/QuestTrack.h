#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// Offers the quest that just displayed a new objective and tracks it through the native
	// Papyrus Quest.SetActive function when accepted.
	class QuestTrack final :
		public Module,
		public RE::BSTEventSink<RE::ObjectiveState::Event>
	{
	public:
		static QuestTrack* GetSingleton();

		const char* Name() const override { return "QuestTrack"; }
		void RegisterEvents();
		void OnGameLoaded() override;
		void Tick() override;
		void FastTick() override;
		void OnAccepted(std::uint16_t a_eventID) override;
		void OnDisabled() override;

		RE::BSEventNotifyControl ProcessEvent(
			const RE::ObjectiveState::Event* a_event,
			RE::BSTEventSource<RE::ObjectiveState::Event>* a_source) override;

	private:
		enum : std::uint16_t
		{
			kTrack = PromptID::kTrackQuest
		};

		using Clock = std::chrono::steady_clock;

		void ReceiveDisplayedObjective(RE::FormID a_questID);

		PromptSlot track{ this, kTrack };
		RE::FormID offeredQuest{ 0 };
		Clock::time_point expiresAt{};
		std::atomic<RE::FormID> pendingQuest{ 0 };
		std::atomic_bool eventTaskQueued{ false };
	};
}
