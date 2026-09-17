#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// Acheron's surrender key as a hold prompt when the player drops below 40% health in combat
	// (optional; idle when Acheron is absent). Like Streamlined Interactions' low-health prompts,
	// the moment is marked with a short slow motion, and the prompt text pulses while it is up.
	// Acheron picks the consequence quest, so Yamete Kudasai's surrender consequences apply when
	// it is installed. Accepting presses Acheron's own key through the input event source.
	class Surrender final : public Module
	{
	public:
		static Surrender* GetSingleton();

		const char* Name() const override { return "Surrender"; }
		void OnGameLoaded() override;
		void Tick() override {}
		void FastTick() override;
		void OnAccepted(std::uint16_t a_eventID) override;
		void OnDisabled() override;
		// Acheron's surrender key, or -1 (control panel conflict check; any thread).
		std::int64_t SurrenderKey() const { return surrenderKey.load(); }
		// Applies the control panel's prompt-only switch to Acheron's key (game thread).
		void ApplyKeyMode();
		// The control panel's key check: asks Acheron for its current key, which its MCM changes in
		// memory at once but writes to Settings.yaml only on a save (game thread; the answer arrives
		// later through OnKeyChecked).
		void CheckKey();
		void OnKeyChecked(std::optional<std::int64_t> a_key);

	private:
		enum : std::uint16_t
		{
			kSurrender = PromptID::kSurrender
		};

		using Clock = std::chrono::steady_clock;

		Surrender();

		bool Blocked(RE::PlayerCharacter* a_player, std::string& a_why) const;
		bool BaboSuspendedAcheron() const;
		void SetAcheronKey(std::int64_t a_key);
		bool AllEnemiesTimedOut(RE::PlayerCharacter* a_player) const;
		void StartSlow();
		void EndSlow(const char* a_reason);
		void Pulse();

		PromptSlot surrender{ this, kSurrender };

		bool active{ false };
		bool acheronPresent{ false };
		// One key check at a time, so repeated clicks never queue several.
		std::atomic<bool> checkPending{ false };
		std::atomic<std::int64_t> surrenderKey{ -1 };
		RE::BGSKeyword* defeated{ nullptr };
		RE::EffectSetting* ykTimeout{ nullptr };
		RE::TESFaction* sexlabAnimating{ nullptr };
		RE::BSTSmartPointer<RE::BSScript::Object> baboController;
		Clock::time_point quietUntil{};
		bool warned{ false };

		bool wasOffered{ false };
		bool slowedThisEpisode{ false };
		bool slowOwned{ false };
		Clock::time_point slowUntil{};
		Clock::time_point pulseStart{};
	};
}
