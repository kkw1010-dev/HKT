#include "ItemEquip.h"

#include "Settings.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		constexpr auto kOfferWindow = 15s;
		constexpr RE::FormID kPlayerRef = 0x14;
		// ccBGSSSE001_FishingPoleKW (ccbgssse001-fish.esm), on every Creation Club fishing rod.
		constexpr auto kFishingRodKeyword = "ccBGSSSE001_FishingPoleKW"sv;
		constexpr RE::FormID kWoodAxesID = 0x10ACCC;  // woodChoppingAxes, Skyrim.esm
		constexpr auto kSexLabPlugin = "SexLab.esm"sv;
		constexpr RE::FormID kSexLabAnimatingID = 0xE50F;  // SexLabAnimatingFaction
	}

	ItemEquip::ItemEquip()
	{
		equip.SetPromptType(SkyPromptAPI::kHold);
	}

	ItemEquip* ItemEquip::GetSingleton()
	{
		static ItemEquip singleton;
		return &singleton;
	}

	void ItemEquip::RegisterEvents()
	{
		auto* source = RE::ScriptEventSourceHolder::GetSingleton();
		if (!source) {
			Log("WARN container-change event source is unavailable");
			return;
		}
		source->AddEventSink<RE::TESContainerChangedEvent>(this);
		Log("container-change event sink registered");
	}

	void ItemEquip::OnGameLoaded()
	{
		equip.Reset();
		lastGate.clear();
		offeredItem = 0;
		expiresAt = {};
		woodAxes = RE::TESForm::LookupByID<RE::BGSListForm>(kWoodAxesID);
		auto* handler = RE::TESDataHandler::GetSingleton();
		sexlabAnimating = handler && handler->LookupModByName(kSexLabPlugin)
		                      ? handler->LookupForm<RE::TESFaction>(kSexLabAnimatingID, kSexLabPlugin)
		                      : nullptr;
		Util::WarnIfSIModuleOn("ItemUse.enabled_equip_weapon", "/MCP/modules/ItemUse/enabled_equip_weapon");
		Util::WarnIfSIModuleOn("ItemUse.enabled_equip_armor", "/MCP/modules/ItemUse/enabled_equip_armor");
		Log("ready: woodChoppingAxes={} sexlab={}", woodAxes != nullptr, sexlabAnimating != nullptr);
	}

	void ItemEquip::Tick()
	{
		Util::WarnIfSIModuleOn("ItemUse.enabled_equip_weapon", "/MCP/modules/ItemUse/enabled_equip_weapon");
		Util::WarnIfSIModuleOn("ItemUse.enabled_equip_armor", "/MCP/modules/ItemUse/enabled_equip_armor");
	}

	bool ItemEquip::AlreadyEquipped(RE::PlayerCharacter* a_player, RE::TESBoundObject* a_item)
	{
		if (auto* book = a_item->As<RE::TESObjectBOOK>()) {
			// A spell tome is "done" once the spell is known; a quest note once it has been read.
			auto* spell = book->TeachesSpell() ? book->GetSpell() : nullptr;
			return spell ? a_player->HasSpell(spell) : book->IsRead();
		}
		if (a_item->Is(RE::FormType::Weapon)) {
			return a_player->GetEquippedObject(false) == a_item || a_player->GetEquippedObject(true) == a_item;
		}
		if (a_item->Is(RE::FormType::Armor)) {
			return a_player->GetWornArmor(a_item->GetFormID()) != nullptr;
		}
		return false;
	}

	bool ItemEquip::Live(RE::PlayerCharacter* a_player, RE::TESBoundObject* a_item, std::string& a_reason) const
	{
		if (!a_player || !a_item) {
			a_reason = "missing player/item";
			return false;
		}
		if (!a_item->GetPlayable()) {
			a_reason = "not playable";
			return false;
		}
		if (!a_item->Is(RE::FormType::Weapon) && !a_item->Is(RE::FormType::Armor) && !a_item->Is(RE::FormType::Book)) {
			a_reason = "not weapon/armor/book";
			return false;
		}
		if (Clock::now() >= expiresAt) {
			a_reason = "expired";
			return false;
		}
		if (Util::ItemCount(a_player, a_item) <= 0) {
			a_reason = "not carried";
			return false;
		}
		if (AlreadyEquipped(a_player, a_item)) {
			a_reason = "already equipped";
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

	void ItemEquip::FastTick()
	{
		auto* player = Util::Player();
		auto* item = offeredItem ? RE::TESForm::LookupByID<RE::TESBoundObject>(offeredItem) : nullptr;
		std::string reason = offeredItem ? "unknown"s : "no acquired gear"s;
		const bool available = offeredItem != 0 && Live(player, item, reason);

		LogGate(std::format("item={} name='{}' state={}",
			offeredItem ? std::format("{:08X}", offeredItem) : "-"s,
			item ? Util::NameOf(item) : "-"s, reason));

		if (!available && (reason == "expired" || reason == "not carried" || reason == "already equipped" ||
				reason == "missing player/item" || reason == "not playable" || reason == "not weapon/armor/book")) {
			offeredItem = 0;
		}
		equip.Update(available, [item] {
			return std::format("{} (길게): {}", item && item->Is(RE::FormType::Book) ? "읽기" : "장착하기", Util::NameOf(item));
		});
	}

	void ItemEquip::OnAccepted(std::uint16_t a_eventID)
	{
		if (a_eventID != kEquip || offeredItem == 0) {
			return;
		}
		auto* player = Util::Player();
		auto* item = RE::TESForm::LookupByID<RE::TESBoundObject>(offeredItem);
		std::string reason;
		if (!Live(player, item, reason)) {
			Log("accept ignored: item={:08X} state={}", offeredItem, reason);
			return;
		}

		const auto itemID = offeredItem;
		offeredItem = 0;
		equip.Withdraw();
		if (auto* book = item->As<RE::TESObjectBOOK>()) {
			ReadBook(player, book);
			return;
		}
		if (auto* manager = RE::ActorEquipManager::GetSingleton()) {
			manager->EquipObject(player, item);
			Log("equipped {} ({:08X})", Util::NameOf(item), itemID);
		} else {
			Log("equip failed: ActorEquipManager unavailable for {:08X}", itemID);
			Util::Notify("CIGAR: 장착 호출 실패. 로그 확인");
		}
	}

	void ItemEquip::ReadBook(RE::PlayerCharacter* a_player, RE::TESObjectBOOK* a_book)
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
			Log("spell tome {} ({:08X}): Read={} spell {} known={}", Util::NameOf(a_book), a_book->GetFormID(), read,
				Util::NameOf(spell), known);
			return;
		}
		const bool read = a_book->Read(a_player);
		RE::BookMenu::OpenMenuFromBaseForm(a_book);
		Log("read {} ({:08X}): Read={} page opened", Util::NameOf(a_book), a_book->GetFormID(), read);
	}

	void ItemEquip::OnDisabled()
	{
		offeredItem = 0;
		expiresAt = {};
	}

	RE::BSEventNotifyControl ItemEquip::ProcessEvent(
		const RE::TESContainerChangedEvent* a_event,
		RE::BSTEventSource<RE::TESContainerChangedEvent>*)
	{
		if (!a_event || a_event->itemCount <= 0 || a_event->newContainer != kPlayerRef ||
			a_event->oldContainer == kPlayerRef || a_event->baseObj == 0) {
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto itemID = a_event->baseObj;
		SKSE::GetTaskInterface()->AddTask([itemID] {
			ItemEquip::GetSingleton()->ReceiveAcquiredItem(itemID);
		});
		return RE::BSEventNotifyControl::kContinue;
	}

	void ItemEquip::ReceiveAcquiredItem(RE::FormID a_itemID)
	{
		if (!Settings::Enabled(Name())) {
			return;
		}
		auto* item = RE::TESForm::LookupByID<RE::TESBoundObject>(a_itemID);
		if (auto* book = item ? item->As<RE::TESObjectBOOK>() : nullptr) {
			// Books: a spell tome whose spell is not known yet, or a quest item (SI's quest note
			// prompt, docs/029-si-leftovers.md). Other books are loot.
			auto* player = Util::Player();
			auto* spell = book->TeachesSpell() ? book->GetSpell() : nullptr;
			bool quest = false;
			if (player && !spell) {
				for (const auto& [object, data] : player->GetInventory([book](RE::TESBoundObject& a_object) { return &a_object == book; })) {
					quest = data.second && data.second->IsQuestObject();
				}
			}
			if (!(spell && player && !player->HasSpell(spell)) && !quest) {
				return;
			}
			if (offeredItem != a_itemID) {
				equip.Withdraw();
				equip.Reset();
			}
			offeredItem = a_itemID;
			expiresAt = Clock::now() + kOfferWindow;
			Log("acquired {} ({:08X}), a {}: offering to read for {}s", Util::NameOf(book), a_itemID,
				spell ? "spell tome" : "quest book", std::chrono::duration_cast<std::chrono::seconds>(kOfferWindow).count());
			return;
		}
		if (!item || !item->GetPlayable() ||
			(!item->Is(RE::FormType::Weapon) && !item->Is(RE::FormType::Armor))) {
			return;
		}
		// Fishing rods are equipped by the fishing itself (Streamlined Fishing, when the supplies are
		// activated); a prompt for them is noise (the user, 2026-09-24).
		if (item->Is(RE::FormType::Weapon)) {
			const auto* keywords = item->As<RE::BGSKeywordForm>();
			if (keywords && keywords->HasKeywordString(kFishingRodKeyword)) {
				Log("acquired {} ({:08X}), a fishing rod: not offered", Util::NameOf(item), a_itemID);
				return;
			}
		}
		// Torches Candlelight and Lanterns swaps lit and unlit lantern armors in and out of the
		// inventory on every toggle, which would offer them here each time. TCL is run by its own
		// hotkey (the user, 2026-09-25).
		if (const auto* file = item->GetFile(0); file && Util::ContainsNoCase(file->GetFilename(), "TorchesCandlelightLanterns")) {
			Log("acquired {} ({:08X}) from TCL: not offered", Util::NameOf(item), a_itemID);
			return;
		}
		// A woodcutter's axe is a tool carried for Woodcutting Tweaks' tree harvest and the chopping
		// block, never wielded (the user, 2026-09-24).
		if (woodAxes && woodAxes->HasForm(item)) {
			Log("acquired {} ({:08X}), a woodcutter's axe: not offered", Util::NameOf(item), a_itemID);
			return;
		}
		// A nameless armor is a body part or effect a script puts on the player (SLOVE's tongue
		// during a SexLab scene, 2026-09-25), never gear; the prompt would show its FormID.
		if (const char* name = item->GetName(); !name || !*name) {
			Log("acquired nameless {:08X}: not offered", a_itemID);
			return;
		}
		// Anything handed over during a SexLab scene belongs to the scene, as for the other modules'
		// SexLab gates.
		if (auto* player = Util::Player(); player && sexlabAnimating && player->IsInFaction(sexlabAnimating)) {
			Log("acquired {} ({:08X}) during a SexLab scene: not offered", Util::NameOf(item), a_itemID);
			return;
		}
		// Fill Her Up's leak and inflater armors are its visual state, put on by its scripts.
		if (const auto* file = item->GetFile(0); file && Util::ContainsNoCase(file->GetFilename(), "sr_FillHerUp")) {
			Log("acquired {} ({:08X}) from Fill Her Up: not offered", Util::NameOf(item), a_itemID);
			return;
		}
		if (offeredItem != a_itemID) {
			equip.Withdraw();
			equip.Reset();
		}
		offeredItem = a_itemID;
		expiresAt = Clock::now() + kOfferWindow;
		Log("acquired {} ({:08X}), offering for {}s", Util::NameOf(item), a_itemID,
			std::chrono::duration_cast<std::chrono::seconds>(kOfferWindow).count());
	}
}
