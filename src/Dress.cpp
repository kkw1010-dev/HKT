#include "Dress.h"

#include "Settings.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		constexpr std::uint32_t kRecordDress = 'DRES';
		// v1: removed-item list only. v2: outfit list + undressed-by-CIGAR flag.
		constexpr std::uint32_t kRecordVersion = 2;
		constexpr std::uint32_t kMaxOutfit = 64;
		// Unequips are queued, so the worn state lags an undress by a tick or two.
		constexpr int kSettleTicks = 3;
	}

	Dress* Dress::GetSingleton()
	{
		static Dress singleton;
		return &singleton;
	}

	void Dress::RegisterEvents()
	{
		if (auto* source = SKSE::GetCrosshairRefEventSource()) {
			source->AddEventSink(this);
			Log("crosshair events registered");
		} else {
			Log("WARN no crosshair event source; bed and wardrobe undress are off");
		}
	}

	void Dress::OnGameLoaded()
	{
		undress.Reset();
		dress.Reset();
		lastGate.clear();
		lastContext.clear();
		place = {};
		placeKind = "";
		lastLoggedFurniture = {};
		settleTicks = 0;
		ready = true;
		Log("ready; outfit={} undressedByCIGAR={} SI water={} bed={} wardrobe={}", outfit.size(), undressedByCIGAR,
			Util::SISetting("/MCP/modules/DressActions/enabled_water"),
			Util::SISetting("/MCP/modules/DressActions/enabled_bed"),
			Util::SISetting("/MCP/modules/DressActions/enabled_wardrobe"));
	}

	RE::BSEventNotifyControl Dress::ProcessEvent(const SKSE::CrosshairRefEvent* a_event, RE::BSTEventSource<SKSE::CrosshairRefEvent>*)
	{
		if (a_event && a_event->crosshairRef) {
			const auto handle = a_event->crosshairRef->CreateRefHandle();
			SKSE::GetTaskInterface()->AddTask([handle]() { GetSingleton()->OnAim(handle); });
		}
		return RE::BSEventNotifyControl::kContinue;
	}

	void Dress::OnAim(RE::ObjectRefHandle a_handle)
	{
		const auto ref = a_handle.get();
		if (!ready || !ref) {
			return;
		}
		const char* kind = PlaceKind(ref.get());
		if (*kind && a_handle != place) {
			place = a_handle;
			placeKind = kind;
			Log("aimed at {}: {}", kind, Util::NameOf(ref->GetBaseObject()));
		}
	}

	const char* Dress::PlaceKind(RE::TESObjectREFR* a_ref)
	{
		auto* base = a_ref->GetBaseObject();
		if (!base) {
			return "";
		}
		if (auto* furniture = base->As<RE::TESFurniture>()) {
			const bool sleep = furniture->furnFlags.any(RE::TESFurniture::ActiveMarker::kCanSleep);
			const auto handle = a_ref->GetHandle();
			if (handle != lastLoggedFurniture) {
				lastLoggedFurniture = handle;
				Log("furniture {} flags={:08X} sleep={}", Util::NameOf(base), furniture->furnFlags.underlying(), sleep);
			}
			return sleep ? "bed" : "";
		}
		if (auto* container = base->As<RE::TESObjectCONT>()) {
			const std::string_view editorID = container->GetFormEditorID();
			const char* model = container->GetModel();
			const std::string_view modelPath = model ? model : "";
			for (const auto word : { "wardrobe"sv, "dresser"sv }) {
				if (Util::ContainsNoCase(editorID, word) || Util::ContainsNoCase(modelPath, word)) {
					return "wardrobe";
				}
			}
		}
		return "";
	}

	const char* Dress::CurrentContext(RE::PlayerCharacter* a_player)
	{
		if (a_player->IsInWater()) {
			return "water";
		}
		if (const auto ref = place.get()) {
			if (ref->GetParentCell() == a_player->GetParentCell() &&
				a_player->GetPosition().GetDistance(ref->GetPosition()) <= Settings::PlaceRange()) {
				return placeKind;
			}
		}
		place = {};
		placeKind = "";
		return "";
	}

	void Dress::Remember(const std::vector<RE::TESObjectARMO*>& a_worn)
	{
		std::vector<RE::FormID> ids;
		for (auto* armor : a_worn) {
			if (ids.size() < kMaxOutfit) {
				ids.push_back(armor->GetFormID());
			}
		}
		if (!ids.empty() && ids != outfit) {
			outfit = std::move(ids);
			std::string names;
			for (auto* armor : a_worn) {
				names += " " + Util::NameOf(armor);
			}
			Log("remembered outfit:{}", names);
		}
	}

	bool Dress::OutfitAvailable(RE::PlayerCharacter* a_player) const
	{
		return std::ranges::any_of(outfit, [a_player](RE::FormID a_id) {
			auto* armor = RE::TESForm::LookupByID<RE::TESObjectARMO>(a_id);
			return armor && Util::ItemCount(a_player, armor) > 0;
		});
	}

	void Dress::Tick()
	{
		auto* player = Util::Player();
		const std::string context = CurrentContext(player);
		const bool busy = Util::IsBusy(player);
		const bool settling = settleTicks > 0;
		if (settling) {
			--settleTicks;
		}

		const bool counted = (!context.empty() || undressedByCIGAR) && !busy && !settling;
		const auto worn = counted ? Util::GetStrippable(player) : std::vector<RE::TESObjectARMO*>{};
		const bool dressed = !worn.empty();

		if (counted && dressed) {
			if (undressedByCIGAR) {
				Log("player is dressed again");
				undressedByCIGAR = false;
			}
			if (!context.empty()) {
				Remember(worn);
			}
		}

		const bool atPlace = context == "bed" || context == "wardrobe";
		const bool naked = counted && !dressed;
		const bool canDress = naked && ((atPlace) || (context.empty() && undressedByCIGAR)) && OutfitAvailable(player);

		LogGate(std::format("context={} strippable={} busy={} outfit={} undressedByCIGAR={}", context.empty() ? "-" : context,
			counted ? std::to_string(worn.size()) : "?"s, busy, outfit.size(), undressedByCIGAR));
		if (context != lastContext) {
			// A new place re-arms both prompts.
			undress.Update(false, {});
			dress.Update(false, {});
			if (!context.empty()) {
				Log("entered {}; worn:{}", context, Util::DescribeWorn(player));
				if (context == "water") {
					Util::WarnIfSIModuleOn("Water Undress", "/MCP/modules/DressActions/enabled_water");
				} else if (context == "bed") {
					Util::WarnIfSIModuleOn("Bed Undress", "/MCP/modules/DressActions/enabled_bed");
				} else {
					Util::WarnIfSIModuleOn("Wardrobe Undress", "/MCP/modules/DressActions/enabled_wardrobe");
				}
			}
			lastContext = context;
		}

		if (settling) {
			return;
		}
		undress.Update(!context.empty() && counted && dressed, [] { return "탈의하기"s; });
		dress.Update(canDress, [] { return "착용하기"s; });
	}

	void Dress::OnAccepted(std::uint16_t a_eventID)
	{
		auto* player = Util::Player();
		if (a_eventID == kUndress) {
			Undress(player);
		} else if (a_eventID == kDress) {
			DressUp(player);
		}
	}

	void Dress::Undress(RE::PlayerCharacter* a_player)
	{
		const auto worn = Util::GetStrippable(a_player);
		Remember(worn);
		auto* equip = RE::ActorEquipManager::GetSingleton();
		std::string names;
		for (auto* armor : worn) {
			equip->UnequipObject(a_player, armor, nullptr, 1, nullptr, true, false, false);
			names += " " + Util::NameOf(armor);
		}
		undressedByCIGAR = !worn.empty();
		settleTicks = kSettleTicks;
		Log("undress removed{}", names);
	}

	void Dress::DressUp(RE::PlayerCharacter* a_player)
	{
		auto* equip = RE::ActorEquipManager::GetSingleton();
		std::string names;
		std::string missing;
		for (const auto formID : outfit) {
			auto* armor = RE::TESForm::LookupByID<RE::TESObjectARMO>(formID);
			if (armor && Util::ItemCount(a_player, armor) > 0) {
				equip->EquipObject(a_player, armor, nullptr, 1, nullptr, true, false, false);
				names += " " + Util::NameOf(armor);
			} else {
				missing += std::format(" {:08X}", formID);
			}
		}
		undressedByCIGAR = false;
		settleTicks = kSettleTicks;
		Log("dress equipped{}{}", names, missing.empty() ? "" : " | not in inventory:" + missing);
	}

	void Dress::Save(SKSE::SerializationInterface* a_intfc) const
	{
		if (!a_intfc->OpenRecord(kRecordDress, kRecordVersion)) {
			logs::error("could not open co-save record DRES");
			return;
		}
		const auto count = static_cast<std::uint32_t>(outfit.size());
		a_intfc->WriteRecordData(count);
		for (const auto formID : outfit) {
			a_intfc->WriteRecordData(formID);
		}
		const std::uint8_t flag = undressedByCIGAR ? 1 : 0;
		a_intfc->WriteRecordData(flag);
	}

	void Dress::Load(SKSE::SerializationInterface* a_intfc, std::uint32_t a_version)
	{
		std::uint32_t count = 0;
		if (a_intfc->ReadRecordData(count) != sizeof(count) || count > kMaxOutfit) {
			logs::warn("co-save record DRES is damaged; ignoring it");
			return;
		}
		outfit.clear();
		for (std::uint32_t i = 0; i < count; ++i) {
			RE::FormID saved = 0;
			RE::FormID current = 0;
			if (a_intfc->ReadRecordData(saved) != sizeof(saved)) {
				logs::warn("co-save record DRES is truncated");
				return;
			}
			if (a_intfc->ResolveFormID(saved, current)) {
				outfit.push_back(current);
			} else {
				logs::warn("dropping outfit item {:08X}: its plugin is no longer loaded", saved);
			}
		}
		if (a_version >= 2) {
			std::uint8_t flag = 0;
			undressedByCIGAR = a_intfc->ReadRecordData(flag) == sizeof(flag) && flag != 0;
		} else {
			// v1 stored only items CIGAR removed and had not restored yet.
			undressedByCIGAR = !outfit.empty();
		}
		Log("loaded outfit={} undressedByCIGAR={} (record v{})", outfit.size(), undressedByCIGAR, a_version);
	}

	void Dress::Revert()
	{
		outfit.clear();
		undressedByCIGAR = false;
		settleTicks = 0;
		ready = false;
	}
}
