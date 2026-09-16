#include "Dress.h"

#include "Util.h"

namespace CIGAR
{
	namespace
	{
		// How close the player must stay to the bed or wardrobe last aimed at.
		constexpr float kPlaceRange = 250.0f;
		constexpr std::uint32_t kRecordRemoved = 'DRES';
		constexpr std::uint32_t kRecordVersion = 1;
		constexpr std::uint32_t kMaxRemoved = 64;
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
		ready = true;
		Log("ready; pendingDress={} SI water={} bed={} wardrobe={}", removed.size(),
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
				a_player->GetPosition().GetDistance(ref->GetPosition()) <= kPlaceRange) {
				return placeKind;
			}
		}
		place = {};
		placeKind = "";
		return "";
	}

	void Dress::Tick()
	{
		auto* player = Util::Player();
		const std::string context = CurrentContext(player);
		const bool busy = Util::IsBusy(player);
		const std::size_t strippable = (!context.empty() || !removed.empty()) && !busy ? Util::GetStrippable(player).size() : 0;

		// Dressed again by hand after leaving: the remembered set is stale.
		if (context.empty() && !removed.empty() && strippable > 0) {
			Log("player dressed without the prompt; forgetting {} item(s)", removed.size());
			removed.clear();
		}

		LogGate(std::format("context={} strippable={} busy={} pendingDress={}", context.empty() ? "-" : context, strippable, busy, removed.size()));
		if (context != lastContext) {
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

		undress.Update(!context.empty() && !busy && strippable > 0, [] { return "탈의하기"s; });
		dress.Update(context.empty() && !busy && !removed.empty() && strippable == 0, [] { return "착용하기"s; });
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
		auto* equip = RE::ActorEquipManager::GetSingleton();
		std::string names;
		for (auto* armor : Util::GetStrippable(a_player)) {
			equip->UnequipObject(a_player, armor, nullptr, 1, nullptr, true, false, false);
			if (std::ranges::find(removed, armor->GetFormID()) == removed.end() && removed.size() < kMaxRemoved) {
				removed.push_back(armor->GetFormID());
			}
			names += " " + Util::NameOf(armor);
		}
		Log("undress removed{} pendingDress={}", names, removed.size());
	}

	void Dress::DressUp(RE::PlayerCharacter* a_player)
	{
		auto* equip = RE::ActorEquipManager::GetSingleton();
		std::string names;
		for (const auto formID : removed) {
			auto* armor = RE::TESForm::LookupByID<RE::TESObjectARMO>(formID);
			if (armor && Util::ItemCount(a_player, armor) > 0) {
				equip->EquipObject(a_player, armor, nullptr, 1, nullptr, true, false, false);
				names += " " + Util::NameOf(armor);
			}
		}
		removed.clear();
		Log("dress equipped{}", names);
	}

	void Dress::Save(SKSE::SerializationInterface* a_intfc) const
	{
		if (!a_intfc->OpenRecord(kRecordRemoved, kRecordVersion)) {
			logs::error("could not open co-save record DRES");
			return;
		}
		const auto count = static_cast<std::uint32_t>(removed.size());
		a_intfc->WriteRecordData(count);
		for (const auto formID : removed) {
			a_intfc->WriteRecordData(formID);
		}
	}

	void Dress::Load(SKSE::SerializationInterface* a_intfc)
	{
		std::uint32_t count = 0;
		if (a_intfc->ReadRecordData(count) != sizeof(count) || count > kMaxRemoved) {
			logs::warn("co-save record DRES is damaged; ignoring it");
			return;
		}
		removed.clear();
		for (std::uint32_t i = 0; i < count; ++i) {
			RE::FormID saved = 0;
			RE::FormID current = 0;
			if (a_intfc->ReadRecordData(saved) != sizeof(saved)) {
				logs::warn("co-save record DRES is truncated");
				break;
			}
			if (a_intfc->ResolveFormID(saved, current)) {
				removed.push_back(current);
			} else {
				logs::warn("dropping removed item {:08X}: its plugin is no longer loaded", saved);
			}
		}
		Log("loaded pendingDress={}", removed.size());
	}

	void Dress::Revert()
	{
		removed.clear();
		ready = false;
	}
}
