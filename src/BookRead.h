#pragma once

#include "Module.h"
#include "Prompt.h"

namespace CIGAR
{
	// 책 읽기: a newly acquired spell tome whose spell is not known, or a book that is a quest item
	// (SI's spellbook equip and quest note prompts), gets a 15 s hold prompt. Split out of ItemEquip
	// on 2026-09-25 (the user) so each has its own switch and its own prompt.
	// docs/029-si-leftovers.md.
	class BookRead final :
		public Module,
		public RE::BSTEventSink<RE::TESContainerChangedEvent>
	{
	public:
		static BookRead* GetSingleton();

		const char* Name() const override { return "BookRead"; }
		void RegisterEvents();
		void OnGameLoaded() override;
		void Tick() override {}
		void FastTick() override;
		void OnAccepted(std::uint16_t a_eventID) override;
		void OnDisabled() override;

		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESContainerChangedEvent* a_event,
			RE::BSTEventSource<RE::TESContainerChangedEvent>* a_source) override;

	private:
		BookRead();

		enum : std::uint16_t
		{
			kRead = PromptID::kReadBook
		};

		using Clock = std::chrono::steady_clock;

		void Receive(RE::FormID a_itemID);
		bool Live(RE::PlayerCharacter* a_player, RE::TESObjectBOOK* a_book, std::string& a_reason) const;
		static bool Done(RE::PlayerCharacter* a_player, RE::TESObjectBOOK* a_book);
		static void Read(RE::PlayerCharacter* a_player, RE::TESObjectBOOK* a_book);

		PromptSlot read{ this, kRead };
		RE::FormID offeredBook{ 0 };
		Clock::time_point expiresAt{};
	};
}
