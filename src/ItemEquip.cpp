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

		// The four parts GearSwap judged by (head, body, hands, feet); an armor is compared with what is
		// worn on the first of them it covers, or on any slot it shares when it covers none of them.
		using Slot = RE::BGSBipedObjectForm::BipedObjectSlot;
		constexpr std::array kParts{ Slot::kHead, Slot::kBody, Slot::kHands, Slot::kFeet };

		RE::InventoryEntryData* EntryOf(const RE::TESObjectREFR::InventoryItemMap& a_inventory, RE::TESBoundObject* a_item)
		{
			const auto it = a_inventory.find(a_item);
			return it != a_inventory.end() ? it->second.second.get() : nullptr;
		}
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
		if (a_item->Is(RE::FormType::Weapon)) {
			return a_player->GetEquippedObject(false) == a_item || a_player->GetEquippedObject(true) == a_item;
		}
		if (a_item->Is(RE::FormType::Armor)) {
			return a_player->GetWornArmor(a_item->GetFormID()) != nullptr;
		}
		return false;
	}

	// Acquired armor is offered only if it beats what it would replace (the user, 2026-09-25, when
	// GearSwap was folded in here): never over an enchanted piece, otherwise only with a higher armor
	// rating as the inventory shows it (tempering and perks included). An empty part is offered.
	bool ItemEquip::BetterArmor(RE::PlayerCharacter* a_player, RE::TESObjectARMO* a_armor, std::string& a_reason)
	{
		RE::TESObjectARMO* worn = nullptr;
		for (const auto slot : kParts) {
			if (a_armor->HasPartOf(slot)) {
				worn = a_player->GetWornArmor(slot);
				break;
			}
		}
		for (std::uint32_t bit = 0; !worn && bit < 32; ++bit) {
			const auto slot = static_cast<Slot>(1u << bit);
			if (a_armor->HasPartOf(slot)) {
				worn = a_player->GetWornArmor(slot);
			}
		}
		if (!worn || worn == a_armor) {
			return true;
		}
		const auto mine = a_player->GetInventory([](RE::TESBoundObject& a_object) { return a_object.IsArmor(); });
		auto* wornEntry = EntryOf(mine, worn);
		if (wornEntry && wornEntry->IsEnchanted()) {
			a_reason = std::format("worn {} enchanted", Util::NameOf(worn));
			return false;
		}
		auto* newEntry = EntryOf(mine, a_armor);
		if (!wornEntry || !newEntry) {
			return true;
		}
		const float have = a_player->GetArmorValue(wornEntry);
		const float got = a_player->GetArmorValue(newEntry);
		if (got <= have) {
			a_reason = std::format("not better than {} ({:.0f} <= {:.0f})", Util::NameOf(worn), got, have);
			return false;
		}
		return true;
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
		if (!a_item->Is(RE::FormType::Weapon) && !a_item->Is(RE::FormType::Armor)) {
			a_reason = "not weapon/armor";
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
		if (auto* armor = a_item->As<RE::TESObjectARMO>(); armor && !BetterArmor(a_player, armor, a_reason)) {
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
				reason == "missing player/item" || reason == "not playable" || reason == "not weapon/armor")) {
			offeredItem = 0;
		}
		equip.Update(available, [item] { return std::format("장착하기 (길게): {}", Util::NameOf(item)); });
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
		if (auto* manager = RE::ActorEquipManager::GetSingleton()) {
			manager->EquipObject(player, item);
			Log("equipped {} ({:08X})", Util::NameOf(item), itemID);
		} else {
			Log("equip failed: ActorEquipManager unavailable for {:08X}", itemID);
			Util::Notify("CIGAR: 장착 호출 실패. 로그 확인");
		}
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
		// Books are BookRead's (split out on 2026-09-25).
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
