#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// Fill Her Up's deflation hotkey as a hold prompt (optional; idle when FHU is absent). Key down
	// and key up are forwarded to FHU's own OnKeyDown / OnKeyUp, so holding the prompt key
	// expels until it is released, stamina runs low or the pool is empty, as the original key does.
	class Deflate final : public Module
	{
	public:
		static Deflate* GetSingleton();

		const char* Name() const override { return "Deflate"; }
		void OnGameLoaded() override;
		void Tick() override;
		void FastTick() override;
		void OnAccepted(std::uint16_t) override {}
		void OnHold(std::uint16_t a_eventID, bool a_down) override;
		void OnDisabled() override;

		// Prompt-only: FHU's deflate key (sr_inflateConfig.defKey) is set to -1, so no keyboard key
		// triggers it; the prompt still works, because the ability only compares the code it is
		// sent with defKey and CIGAR sends defKey. Switching prompt-only off gives the key back.
		// Game thread.
		void ApplyKeyMode();
		// The key FHU listens to now, or -1.
		// The panel reads this on the render thread: a copy the game thread refreshes, never the VM
		// (review 2026-09-30; Grapple, Surrender and Execute already work this way).
		std::int32_t Key() const { return shownKey.load(); }

		// Result of FHU's GetMostRecentInflationType(player), from a VM thread.
		void SetInflationType(std::int32_t a_type);

	private:
		enum : std::uint16_t
		{
			kDeflate = PromptID::kDeflate
		};

		Deflate();

		bool Resolve();
		std::int32_t DeflateKey() const;
		void QueryInflationType();
		void SendKey(const char* a_event, bool a_down);

		PromptSlot deflate{ this, kDeflate };

		// Fill Her Up, read from its player alias script at load so only the quest ID is fixed.
		RE::TESQuest* inflater{ nullptr };
		RE::TESQuest* config{ nullptr };
		RE::BGSRefAlias* playerAlias{ nullptr };
		RE::TESFaction* inflateFaction{ nullptr };
		RE::TESFaction* oralFaction{ nullptr };
		RE::TESFaction* animatingFaction{ nullptr };
		RE::TESFaction* sexlabAnimating{ nullptr };

		std::atomic<std::int32_t> inflationType{ -1 };
		std::atomic_bool queryPending{ false };
		std::atomic<std::int32_t> shownKey{ -1 };
		bool holding{ false };
		// The key is down but not yet for kRingFill: nothing has gone to FHU (a tap or a double-tap
		// decline must not start its push loop or its 5 s cooldown).
		bool pressing{ false };
		std::chrono::steady_clock::time_point pressedAt{};
		std::chrono::steady_clock::time_point holdStart{};
		std::chrono::steady_clock::time_point busyUntil{};
	};
}
