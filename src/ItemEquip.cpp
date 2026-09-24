#include "ItemEquip.h"

#include "Settings.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		constexpr auto kOfferWindow = 15s;
		constexpr RE::FormID kPlayerRef = 0x14;
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
		Util::WarnIfSIModuleOn("ItemUse.enabled_equip_weapon", "/MCP/modules/ItemUse/enabled_equip_weapon");
		Util::WarnIfSIModuleOn("ItemUse.enabled_equip_armor", "/MCP/modules/ItemUse/enabled_equip_armor");
		Log("ready");
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
		if (!item || !item->GetPlayable() ||
			(!item->Is(RE::FormType::Weapon) && !item->Is(RE::FormType::Armor))) {
			return;
		}
		// Lanterns are lit and put away through TCL by 불 밝히기 (Light); TCL also swaps lit and unlit
		// lantern armors in and out of the inventory, which would offer them here every time.
		if (const auto* file = item->GetFile(0); file && Util::ContainsNoCase(file->GetFilename(), "TorchesCandlelightLanterns")) {
			Log("acquired {} ({:08X}) from TCL: left to 불 밝히기, not offered", Util::NameOf(item), a_itemID);
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
