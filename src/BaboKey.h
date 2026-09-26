#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// BaboDialogue's interaction hotkey, offered as a prompt only while the kidnap event is at a
	// point where the key does something (optional; idle when BaboDialogue is absent).
	// Accepting it calls BaboDialogue's own OnKeyDown handler, so all of its checks still apply.
	class BaboKey final : public Module
	{
	public:
		static BaboKey* GetSingleton();

		const char* Name() const override { return "BaboKey"; }
		void OnGameLoaded() override;
		void Tick() override;
		void OnAccepted(std::uint16_t a_eventID) override;

	private:
		enum : std::uint16_t
		{
			kAct = PromptID::kBaboAct
		};

		BaboKey() { act.SetRepeat(true); }

		bool Resolve();
		std::int32_t NotificationKey() const;
		bool KeyIsLive(std::string& a_gate);

		PromptSlot act{ this, kAct };

		// BaboDialogue, read from its monitor quest's script at load so only that quest's ID is fixed.
		RE::TESQuest* monitor{ nullptr };
		RE::TESQuest* configQuest{ nullptr };
		RE::TESQuest* kidnap{ nullptr };
		RE::TESFaction* npcAnimating{ nullptr };
		RE::TESGlobal* tiedUp{ nullptr };
		RE::TESGlobal* scenario{ nullptr };
		RE::BGSRefAlias* centerMarker{ nullptr };

		bool warnedBroken{ false };
	};
}
