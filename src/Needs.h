#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// Private Needs - Orgasm's toilet actions as prompts (optional; idle without the mod). 소변 보기
	// and 대변 보기 show while PNO's bladder or bowel level is at or above the control panel's stage,
	// with the fill in percent, and call PNO's own UrinateAndDefecate, the function its hotkeys and
	// its needs menu call. Prompt-only mode (default on) unbinds PNO's six hotkeys, so its default
	// Y (menu) and U (check needs) are free; the percentage in the prompt replaces the needs check.
	class Needs final :
		public Module,
		public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
	public:
		static Needs* GetSingleton();

		// PNO binds its keys again whenever its MCM initialises or loads a profile, so the keys are
		// checked again shortly after the journal menu (where the MCM lives) closes (kDataLoaded).
		void RegisterEvents();
		RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

		const char* Name() const override { return "Needs"; }
		void OnGameLoaded() override;
		void Tick() override;
		void OnAccepted(std::uint16_t a_eventID) override;

		// Applies the control panel's prompt-only switch to PNO's hotkeys: unbinds them (remembering
		// each bound key) or restores them. Also the panel's key check (game thread).
		void ApplyKeyMode();
		// PNO's bound hotkeys as "name=key" pairs, for the control panel (any thread).
		std::string KeySummary() const;

		// The bladder/bowel level (PNO's 1-5) the prompts start at (control panel).
		static constexpr int kMinStageLow = 1;
		static constexpr int kMinStageHigh = 5;
		// PNO refuses to excrete at level 0, so level 1 is the lowest that can act.
		static constexpr int kMinStageDefault = 1;

	private:
		enum : std::uint16_t
		{
			kUrinate = PromptID::kUrinate,
			kDefecate = PromptID::kDefecate
		};

		using Clock = std::chrono::steady_clock;

		struct State
		{
			bool running{ false };
			bool bladderOn{ false };
			bool bowelOn{ false };
			int bladderLevel{ -1 };
			int bowelLevel{ -1 };
			float bladderPercent{ 0.0f };
			float bowelPercent{ 0.0f };
			bool excreting{ false };
			bool canUrinate{ false };
			bool canDefecate{ false };
		};

		Needs();

		State Read(RE::PlayerCharacter* a_player, std::string& a_gate) const;
		void CheckAfterStart(const State& a_state);

		PromptSlot urinate{ this, kUrinate };
		PromptSlot defecate{ this, kDefecate };

		bool active{ false };
		RE::TESQuest* configQuest{ nullptr };
		RE::TESQuest* mainQuest{ nullptr };
		RE::BSTSmartPointer<RE::BSScript::Object> config;
		RE::BSTSmartPointer<RE::BSScript::Object> utility;
		RE::BSTSmartPointer<RE::BSScript::Object> main;
		RE::EffectSetting* excreteEffect{ nullptr };
		RE::TESFaction* sexlabAnimating{ nullptr };

		int offeredBladder{ -1 };
		int offeredBowel{ -1 };
		Clock::time_point quietUntil{};
		bool warnedStopped{ false };

		// After an accept: did PNO start (its excrete effect appeared), and did the fill drop after it ended.
		bool checking{ false };
		bool sawEffect{ false };
		Clock::time_point checkDeadline{};
		std::uint16_t checkedPrompt{ 0 };
		float fillBefore{ 0.0f };

		// Set by the journal menu closing (UI thread); Tick() checks the keys once it has passed.
		std::atomic<std::int64_t> keyCheckAt{ 0 };

		mutable std::mutex keyLock;
		std::string keySummary;
	};
}
