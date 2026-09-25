#include "Helmet.h"

#include "Util.h"

namespace CIGAR
{
	namespace
	{
		// Skyrim.esm: ArmorHelmet, ClothingHead, ClothingCirclet.
		constexpr RE::FormID kArmorHelmetID = 0x06C0EE;
		constexpr RE::FormID kClothingHeadID = 0x10CD11;
		constexpr RE::FormID kClothingCircletID = 0x10CD08;
		// Location types that make an interior a dungeon. The user's rule (2026-09-25): the helmet is
		// off by default everywhere, dungeons excepted.
		constexpr std::array kDungeonTypes{ "LocTypeDungeon"sv, "LocTypeClearable"sv, "LocTypeDraugrCrypt"sv,
			"LocTypeDwarvenAutomatons"sv, "LocTypeFalmerHive"sv, "LocTypeVampireLair"sv, "LocTypeDragonPriestLair"sv,
			"LocTypeAnimalDen"sv, "LocTypeBanditCamp"sv, "LocTypeForswornCamp"sv, "LocTypeWarlockLair"sv,
			"LocTypeHagravenNest"sv, "LocTypeWerewolfLair"sv, "LocTypeWerebearLair"sv, "LocTypeSprigganGrove"sv,
			"LocTypeGiantCamp"sv, "LocTypeDragonLair"sv, "DLC2LocTypeRieklingCamp"sv, "DLC2LocTypeAshSpawn"sv };
		// Helmet Toggle 2's OAR clips replace GPMAOffsetAnimation while iGPMAAnimationType holds their
		// number; OffsetGPMA / OffsetGPMAStop start and end it (HT_PlayerAlias.ManageHelmet).
		constexpr auto kClipVariable = "iGPMAAnimationType";
		constexpr auto kClipStart = "OffsetGPMA";
		constexpr auto kClipStop = "OffsetGPMAStop";
		constexpr int kHelmetUnequipClip = 2;
		constexpr int kHoodUnequipClip = 6;
		// HT_PlayerAlias: the head is reached 0.7 s into the take-off clip, which ends 1.85 s later.
		constexpr auto kReachHead = 700ms;
		constexpr auto kClipTail = 1850ms;
		constexpr std::uint32_t kRecordHelmet = 'HELM';
		constexpr std::uint32_t kRecordVersion = 1;
		constexpr std::uint32_t kMaxStowed = 8;
	}

	Helmet::Helmet()
	{
		off.SetPromptType(SkyPromptAPI::kHold);
		// 투구 쓰기 comes up in combat, so it is one press (the input policy's combat exception).
	}

	Helmet* Helmet::GetSingleton()
	{
		static Helmet singleton;
		return &singleton;
	}

	void Helmet::OnGameLoaded()
	{
		off.Reset();
		on.Reset();
		lastGate.clear();
		step = Step::kIdle;
		removing.clear();
		clipRunning = false;
		location = 0;
		offDismissed = false;
		onDismissed = false;
		wasInCombat = false;

		armorHelmet = RE::TESForm::LookupByID<RE::BGSKeyword>(kArmorHelmetID);
		clothingHead = RE::TESForm::LookupByID<RE::BGSKeyword>(kClothingHeadID);
		clothingCirclet = RE::TESForm::LookupByID<RE::BGSKeyword>(kClothingCircletID);
		dungeonKeywords.clear();
		for (const auto editorID : kDungeonTypes) {
			if (auto* keyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>(editorID)) {
				dungeonKeywords.push_back(keyword);
			}
		}
		Util::WarnIfSIModuleOn("HelmetToggle.enabled", "/MCP/modules/HelmetToggle/enabled");
		if (auto* handler = RE::TESDataHandler::GetSingleton(); handler && handler->LookupModByName("Helmet Toggle 2.esp")) {
			Log("WARN Helmet Toggle 2 is loaded: its scripts re-hide or re-equip headgear on every change");
			Util::Notify("CIGAR: Helmet Toggle 2가 켜져 있음. 투구 전환이 충돌할 수 있음");
		}
		Log("ready: keywords helmet={} head={} circlet={}, dungeon types {} of {}, stowed {}", armorHelmet != nullptr,
			clothingHead != nullptr, clothingCirclet != nullptr, dungeonKeywords.size(), kDungeonTypes.size(), stowed.size());
	}

	std::vector<RE::TESObjectARMO*> Helmet::WornHeadgear(RE::PlayerCharacter* a_player) const
	{
		std::vector<RE::TESObjectARMO*> result;
		using Slot = RE::BGSBipedObjectForm::BipedObjectSlot;
		for (const auto slot : { Slot::kHead, Slot::kHair }) {
			auto* armor = a_player->GetWornArmor(slot);
			if (!armor || std::ranges::find(result, armor) != result.end() ||
				(clothingCirclet && armor->HasKeyword(clothingCirclet))) {
				continue;
			}
			if ((armorHelmet && armor->HasKeyword(armorHelmet)) || (clothingHead && armor->HasKeyword(clothingHead))) {
				result.push_back(armor);
			}
		}
		return result;
	}

	bool Helmet::InDungeon(RE::PlayerCharacter* a_player, std::string& a_why) const
	{
		const auto* cell = a_player->GetParentCell();
		if (!cell || !cell->IsInteriorCell()) {
			a_why = "exterior";
			return false;
		}
		auto* loc = a_player->GetCurrentLocation();
		if (!loc) {
			a_why = "interior, no location";
			return false;
		}
		for (auto* keyword : dungeonKeywords) {
			if (loc->HasKeyword(keyword)) {
				a_why = std::format("interior {}", keyword->GetFormEditorID());
				return true;
			}
		}
		a_why = "interior, not a dungeon";
		return false;
	}

	bool Helmet::Carried(RE::PlayerCharacter* a_player, RE::FormID a_id) const
	{
		auto* item = RE::TESForm::LookupByID<RE::TESBoundObject>(a_id);
		return item && Util::ItemCount(a_player, item) > 0;
	}

	void Helmet::Clip(RE::PlayerCharacter* a_player, int a_type, const char* a_event)
	{
		const bool set = a_player->SetGraphVariableInt(kClipVariable, a_type);
		const bool sent = a_player->NotifyAnimationGraph(a_event);
		Log("clip {} {}={}: variable set={} event accepted={}", a_event, kClipVariable, a_type, set, sent);
	}

	void Helmet::TakeOff(RE::PlayerCharacter* a_player)
	{
		removing = WornHeadgear(a_player);
		if (removing.empty()) {
			Log("take off: nothing worn");
			return;
		}
		const auto* state = a_player->AsActorState();
		const bool seated = state && state->GetSitSleepState() != RE::SIT_SLEEP_STATE::kNormal;
		const bool drawn = state && state->IsWeaponDrawn();
		if (seated || drawn) {
			// Helmet Toggle's clip is a standing, empty-handed one; seated or armed it is skipped.
			Log("take off without the clip (seated={} drawn={})", seated, drawn);
			step = Step::kUnequip;
			stepAt = Clock::now();
			clipRunning = false;
			return;
		}
		const bool hood = !(armorHelmet && removing.front()->HasKeyword(armorHelmet));
		Clip(a_player, hood ? kHoodUnequipClip : kHelmetUnequipClip, kClipStart);
		clipRunning = true;
		step = Step::kUnequip;
		stepAt = Clock::now() + kReachHead;
	}

	void Helmet::PutOn(RE::PlayerCharacter* a_player)
	{
		auto* manager = RE::ActorEquipManager::GetSingleton();
		std::size_t equipped = 0;
		for (const auto id : stowed) {
			auto* armor = RE::TESForm::LookupByID<RE::TESObjectARMO>(id);
			if (!armor || !Carried(a_player, id) || !manager) {
				Log("put on: {:08X} no longer carried", id);
				continue;
			}
			manager->EquipObject(a_player, armor);
			++equipped;
			Log("put on {} ({:08X})", Util::NameOf(armor), id);
		}
		if (equipped == 0) {
			Util::Notify("CIGAR: 쓸 투구가 인벤토리에 없음");
		}
		stowed.clear();
	}

	void Helmet::FastTick()
	{
		if (step == Step::kIdle || Clock::now() < stepAt) {
			return;
		}
		auto* player = Util::Player();
		if (!player) {
			step = Step::kIdle;
			return;
		}
		if (step == Step::kUnequip) {
			auto* manager = RE::ActorEquipManager::GetSingleton();
			for (auto* armor : removing) {
				const bool done = manager && manager->UnequipObject(player, armor);
				if (std::ranges::find(stowed, armor->GetFormID()) == stowed.end() && stowed.size() < kMaxStowed) {
					stowed.push_back(armor->GetFormID());
				}
				Log("took off {} ({:08X}) unequip={}", Util::NameOf(armor), armor->GetFormID(), done);
			}
			removing.clear();
			if (clipRunning) {
				step = Step::kStopClip;
				stepAt = Clock::now() + kClipTail;
			} else {
				step = Step::kIdle;
			}
			return;
		}
		// Step::kStopClip
		Clip(player, 0, kClipStop);
		clipRunning = false;
		step = Step::kIdle;
	}

	void Helmet::Tick()
	{
		Util::WarnIfSIModuleOn("HelmetToggle.enabled", "/MCP/modules/HelmetToggle/enabled");
		auto* player = Util::Player();
		if (!player) {
			return;
		}

		const auto* loc = player->GetCurrentLocation();
		const RE::FormID locID = loc ? loc->GetFormID() : 0;
		if (locID != location) {
			location = locID;
			if (offDismissed) {
				offDismissed = false;
				Log("location changed: 투구 벗기 offered again");
			}
		}
		const bool combat = player->IsInCombat();
		if (wasInCombat && !combat && onDismissed) {
			onDismissed = false;
		}
		wasInCombat = combat;

		const auto worn = WornHeadgear(player);
		if (!worn.empty() && !stowed.empty() && step == Step::kIdle) {
			// Headgear is on again (from the inventory or by 투구 쓰기): nothing to remember.
			Log("headgear worn again ({}): stowed list cleared", Util::NameOf(worn.front()));
			stowed.clear();
		}
		std::erase_if(stowed, [this, player](RE::FormID a_id) { return !Carried(player, a_id); });

		std::string why;
		const bool dungeon = InDungeon(player, why);
		const auto* controls = RE::ControlMap::GetSingleton();
		const bool movable = controls && controls->IsMovementControlsEnabled();
		const bool busy = step != Step::kIdle;

		LogGate(std::format("worn={} stowed={} dungeon={} ({}) combat={} movable={} busy={} dismissed={}/{}",
			worn.empty() ? "-"s : Util::NameOf(worn.front()), stowed.size(), dungeon, why, combat, movable, busy,
			offDismissed, onDismissed));

		off.Update(!worn.empty() && !dungeon && !combat && movable && !busy && !offDismissed,
			[] { return "투구 벗기 (길게)"s; });
		on.Update(worn.empty() && !stowed.empty() && combat && !busy && !onDismissed, [] { return "투구 쓰기"s; });
	}

	void Helmet::OnAccepted(std::uint16_t a_eventID)
	{
		auto* player = Util::Player();
		if (!player) {
			return;
		}
		if (a_eventID == kHelmetOff) {
			TakeOff(player);
		} else if (a_eventID == kHelmetOn) {
			PutOn(player);
		}
	}

	void Helmet::OnDeclined(std::uint16_t a_eventID)
	{
		if (a_eventID == kHelmetOff) {
			offDismissed = true;
			Log("투구 벗기 declined: hidden until the location changes");
		} else if (a_eventID == kHelmetOn) {
			onDismissed = true;
			Log("투구 쓰기 declined: hidden until combat ends");
		}
	}

	void Helmet::OnDisabled()
	{
		if (clipRunning) {
			if (auto* player = Util::Player()) {
				Clip(player, 0, kClipStop);
			}
		}
		step = Step::kIdle;
		removing.clear();
		clipRunning = false;
	}

	void Helmet::Save(SKSE::SerializationInterface* a_intfc) const
	{
		if (!a_intfc->OpenRecord(kRecordHelmet, kRecordVersion)) {
			logs::error("could not open co-save record HELM");
			return;
		}
		const auto count = static_cast<std::uint32_t>(stowed.size());
		a_intfc->WriteRecordData(count);
		for (const auto id : stowed) {
			a_intfc->WriteRecordData(id);
		}
	}

	void Helmet::Load(SKSE::SerializationInterface* a_intfc, std::uint32_t)
	{
		std::uint32_t count = 0;
		if (a_intfc->ReadRecordData(count) != sizeof(count) || count > kMaxStowed) {
			logs::warn("co-save record HELM is damaged; ignoring it");
			return;
		}
		stowed.clear();
		for (std::uint32_t i = 0; i < count; ++i) {
			RE::FormID saved = 0;
			RE::FormID current = 0;
			if (a_intfc->ReadRecordData(saved) != sizeof(saved)) {
				logs::warn("co-save record HELM is truncated");
				return;
			}
			if (a_intfc->ResolveFormID(saved, current)) {
				stowed.push_back(current);
			}
		}
		Log("loaded stowed headgear: {}", stowed.size());
	}

	void Helmet::Revert()
	{
		stowed.clear();
		step = Step::kIdle;
		removing.clear();
		clipRunning = false;
	}
}
