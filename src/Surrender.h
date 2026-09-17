#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// Acheron's surrender key as a hold prompt when the player drops below 20% health in combat
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

	private:
		// An event ID no other module uses: SkyPrompt gives each event ID its own key slot, so
		// this prompt never shares a key with 록온 / 그래플.
		enum : std::uint16_t
		{
			kSurrender = 7
		};

		using Clock = std::chrono::steady_clock;

		Surrender();

		bool Blocked(RE::PlayerCharacter* a_player, std::string& a_why) const;
		void StartSlow();
		void EndSlow(const char* a_reason);
		void Pulse();

		PromptSlot surrender{ this, kSurrender };

		bool active{ false };
		std::int64_t surrenderKey{ -1 };
		RE::BGSKeyword* defeated{ nullptr };
		RE::TESFaction* sexlabAnimating{ nullptr };
		Clock::time_point quietUntil{};
		bool warned{ false };

		bool wasOffered{ false };
		bool slowedThisEpisode{ false };
		bool slowOwned{ false };
		Clock::time_point slowUntil{};
		Clock::time_point pulseStart{};
	};
}
