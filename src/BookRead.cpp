#include "BookRead.h"

#include "Settings.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		constexpr auto kOfferWindow = 15s;
		constexpr RE::FormID kPlayerRef = 0x14;
	}

	BookRead::BookRead()
	{
		read.SetPromptType(SkyPromptAPI::kHold);
	}

	BookRead* BookRead::GetSingleton()
	{
		static BookRead singleton;
		return &singleton;
	}

	void BookRead::RegisterEvents()
	{
		auto* source = RE::ScriptEventSourceHolder::GetSingleton();
		if (!source) {
			Log("WARN container-change event source is unavailable");
			return;
		}
		source->AddEventSink<RE::TESContainerChangedEvent>(this);
		Log("container-change event sink registered");
	}

	void BookRead::OnGameLoaded()
	{
		read.Reset();
		lastGate.clear();
		offeredBook = 0;
		expiresAt = {};
		Util::WarnIfSIModuleOn("ItemUse.enabled_equip_spellbook", "/MCP/modules/ItemUse/enabled_equip_spellbook");
		Log("ready");
	}

	bool BookRead::Done(RE::PlayerCharacter* a_player, RE::TESObjectBOOK* a_book)
	{
		// A spell tome is done once the spell is known; a quest note once it has been read.
		auto* spell = a_book->TeachesSpell() ? a_book->GetSpell() : nullptr;
		return spell ? a_player->HasSpell(spell) : a_book->IsRead();
	}

	bool BookRead::Live(RE::PlayerCharacter* a_player, RE::TESObjectBOOK* a_book, std::string& a_reason) const
	{
		if (!a_player || !a_book) {
			a_reason = "missing player/book";
			return false;
		}
		if (Clock::now() >= expiresAt) {
			a_reason = "expired";
			return false;
		}
		if (Util::ItemCount(a_player, a_book) <= 0) {
			a_reason = "not carried";
			return false;
		}
		if (Done(a_player, a_book)) {
			a_reason = "already read";
			return false;
		}
		if (a_player->IsInCombat()) {
			a_reason = "combat";
			return false;
		}
		const auto* controls = RE::ControlMap::GetSingleton();
		if (!controls || !controls->IsMovementControlsEnabled()) {
			a_reason = "movement disabled";
			return false;
		}
		a_reason = "ready";
		return true;
	}

	void BookRead::FastTick()
	{
		auto* player = Util::Player();
		auto* book = offeredBook ? RE::TESForm::LookupByID<RE::TESObjectBOOK>(offeredBook) : nullptr;
		std::string reason = offeredBook ? "unknown"s : "no new book"s;
		const bool available = offeredBook != 0 && Live(player, book, reason);

		LogGate(std::format("book={} name='{}' state={}", offeredBook ? std::format("{:08X}", offeredBook) : "-"s,
			book ? Util::NameOf(book) : "-"s, reason));

		if (!available && (reason == "expired" || reason == "not carried" || reason == "already read" ||
				reason == "missing player/book")) {
			offeredBook = 0;
		}
		read.Update(available, [book] { return std::format("읽기 (길게): {}", Util::NameOf(book)); });
	}

	void BookRead::OnAccepted(std::uint16_t a_eventID)
	{
		if (a_eventID != kRead || offeredBook == 0) {
			return;
		}
		auto* player = Util::Player();
		auto* book = RE::TESForm::LookupByID<RE::TESObjectBOOK>(offeredBook);
		std::string reason;
		if (!Live(player, book, reason)) {
			Log("accept ignored: book={:08X} state={}", offeredBook, reason);
			return;
		}
		offeredBook = 0;
		read.Withdraw();
		Read(player, book);
	}

	void BookRead::Read(RE::PlayerCharacter* a_player, RE::TESObjectBOOK* a_book)
	{
		// SI's Equipper::ReadBook, rebuilt: a spell tome teaches its spell and is used up, as when
		// read from the inventory; any other book (a quest note) is read and its page opened.
		if (auto* spell = a_book->TeachesSpell() ? a_book->GetSpell() : nullptr) {
			const bool read = a_book->Read(a_player);
			if (!a_player->HasSpell(spell)) {
				a_player->AddSpell(spell);
			}
			const bool known = a_player->HasSpell(spell);
			if (known) {
				a_player->RemoveItem(a_book, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				Util::Notify(std::format("{} 습득", Util::NameOf(spell)));
			}
			GetSingleton()->Log("spell tome {} ({:08X}): Read={} spell {} known={}", Util::NameOf(a_book),
				a_book->GetFormID(), read, Util::NameOf(spell), known);
			return;
		}
		const bool read = a_book->Read(a_player);
		RE::BookMenu::OpenMenuFromBaseForm(a_book);
		GetSingleton()->Log("read {} ({:08X}): Read={} page opened", Util::NameOf(a_book), a_book->GetFormID(), read);
	}

	void BookRead::OnDisabled()
	{
		offeredBook = 0;
		expiresAt = {};
	}

	RE::BSEventNotifyControl BookRead::ProcessEvent(
		const RE::TESContainerChangedEvent* a_event,
		RE::BSTEventSource<RE::TESContainerChangedEvent>*)
	{
		if (!a_event || a_event->itemCount <= 0 || a_event->newContainer != kPlayerRef ||
			a_event->oldContainer == kPlayerRef || a_event->baseObj == 0) {
			return RE::BSEventNotifyControl::kContinue;
		}
		const auto itemID = a_event->baseObj;
		SKSE::GetTaskInterface()->AddTask([itemID] { BookRead::GetSingleton()->Receive(itemID); });
		return RE::BSEventNotifyControl::kContinue;
	}

	void BookRead::Receive(RE::FormID a_itemID)
	{
		if (!Settings::Enabled(Name())) {
			return;
		}
		auto* book = RE::TESForm::LookupByID<RE::TESObjectBOOK>(a_itemID);
		auto* player = Util::Player();
		if (!book || !player) {
			return;
		}
		// A spell tome whose spell is not known yet, or a quest item (SI's quest note prompt). Other
		// books are loot.
		auto* spell = book->TeachesSpell() ? book->GetSpell() : nullptr;
		bool quest = false;
		if (!spell) {
			for (const auto& [object, data] : player->GetInventory([book](RE::TESBoundObject& a_object) { return &a_object == book; })) {
				quest = data.second && data.second->IsQuestObject();
			}
		}
		if (!(spell && !player->HasSpell(spell)) && !quest) {
			return;
		}
		if (offeredBook != a_itemID) {
			read.Withdraw();
			read.Reset();
		}
		offeredBook = a_itemID;
		expiresAt = Clock::now() + kOfferWindow;
		Log("acquired {} ({:08X}), a {}: offering to read for {}s", Util::NameOf(book), a_itemID,
			spell ? "spell tome" : "quest book", std::chrono::duration_cast<std::chrono::seconds>(kOfferWindow).count());
	}
}
