#include "PartyOutfit.h"

#include "Util.h"

namespace CIGAR
{
	namespace
	{
		// Skyrim.esm: MQ201 Diplomatic Immunity, MQ201PartyOutfit, MQ201PartyBoots.
		constexpr RE::FormID kQuestID = 0x035D5F;
		constexpr RE::FormID kOutfitID = 0x0E40DF;
		constexpr RE::FormID kBootsID = 0x0E40DE;
		// MQ201 objectives shown from the party clothes on: 40 talk to Malborn (at the party),
		// 50 create a distraction and slip away.
		constexpr std::array kPartyObjectives{ std::uint16_t{ 40 }, std::uint16_t{ 50 } };
		constexpr std::uint32_t kRecord = 'QOUT';
		constexpr std::uint32_t kRecordVersion = 1;
		constexpr std::uint32_t kMaxStored = 32;
	}

	PartyOutfit::PartyOutfit()
	{
		wear.SetPromptType(SkyPromptAPI::kHold);
		back.SetPromptType(SkyPromptAPI::kHold);
	}

	PartyOutfit* PartyOutfit::GetSingleton()
	{
		static PartyOutfit singleton;
		return &singleton;
	}

	void PartyOutfit::OnGameLoaded()
	{
		wear.Reset();
		back.Reset();
		lastGate.clear();
		quest = RE::TESForm::LookupByID<RE::TESQuest>(kQuestID);
		outfit = RE::TESForm::LookupByID<RE::TESObjectARMO>(kOutfitID);
		boots = RE::TESForm::LookupByID<RE::TESObjectARMO>(kBootsID);
		Log("ready: MQ201={} outfit={} boots={} stored={}", quest != nullptr, outfit ? Util::NameOf(outfit) : "-"s,
			boots ? Util::NameOf(boots) : "-"s, stored.size());
	}

	bool PartyOutfit::PartyShown() const
	{
		if (!quest || !quest->IsRunning()) {
			return false;
		}
		for (const auto* objective : quest->objectives) {
			if (objective && objective->state.get() == RE::QUEST_OBJECTIVE_STATE::kDisplayed &&
				std::ranges::find(kPartyObjectives, objective->index) != kPartyObjectives.end()) {
				return true;
			}
		}
		return false;
	}

	bool PartyOutfit::Wearing(RE::PlayerCharacter* a_player) const
	{
		return outfit && a_player->GetWornArmor(outfit->GetFormID()) != nullptr;
	}

	std::size_t PartyOutfit::CarriedStored(RE::PlayerCharacter* a_player) const
	{
		return static_cast<std::size_t>(std::ranges::count_if(stored, [a_player](RE::FormID a_id) {
			auto* item = RE::TESForm::LookupByID<RE::TESBoundObject>(a_id);
			return item && Util::ItemCount(a_player, item) > 0;
		}));
	}

	void PartyOutfit::Tick()
	{
		auto* player = Util::Player();
		if (!player || !outfit) {
			return;
		}
		const bool party = PartyShown();
		const bool wearing = Wearing(player);
		const bool carried = Util::ItemCount(player, outfit) > 0;
		const std::size_t storedCarried = CarriedStored(player);
		const bool combat = player->IsInCombat();
		const auto* controls = RE::ControlMap::GetSingleton();
		const bool movable = controls && controls->IsMovementControlsEnabled();
		LogGate(std::format("mq201={} party={} carried={} wearing={} stored={} ({} carried) combat={} movable={}",
			quest && quest->IsRunning() ? std::to_string(quest->GetCurrentStageID()) : "-"s, party, carried, wearing,
			stored.size(), storedCarried, combat, movable));
		wear.Update(party && carried && !wearing && !combat && movable, [this] { return std::format("파티 의상 입기 (길게): {}", Util::NameOf(outfit)); });
		back.Update(!party && wearing && storedCarried > 0 && !combat && movable, [] { return "원래 장비로 (길게)"s; });
	}

	void PartyOutfit::OnAccepted(std::uint16_t a_eventID)
	{
		auto* player = Util::Player();
		auto* manager = RE::ActorEquipManager::GetSingleton();
		if (!player || !manager || !outfit) {
			return;
		}
		if (a_eventID == kWear) {
			stored.clear();
			for (auto* armor : Util::GetStrippable(player)) {
				if (armor != outfit && armor != boots && stored.size() < kMaxStored) {
					stored.push_back(armor->GetFormID());
					manager->UnequipObject(player, armor);
				}
			}
			manager->EquipObject(player, outfit);
			if (boots && Util::ItemCount(player, boots) > 0) {
				manager->EquipObject(player, boots);
			}
			Log("party clothes on; stored {} worn pieces", stored.size());
		} else if (a_eventID == kBack) {
			manager->UnequipObject(player, outfit);
			if (boots && player->GetWornArmor(boots->GetFormID())) {
				manager->UnequipObject(player, boots);
			}
			std::size_t equipped = 0;
			for (const auto id : stored) {
				auto* armor = RE::TESForm::LookupByID<RE::TESObjectARMO>(id);
				if (armor && Util::ItemCount(player, armor) > 0) {
					manager->EquipObject(player, armor);
					++equipped;
				}
			}
			Log("previous gear back: {} of {} pieces", equipped, stored.size());
			stored.clear();
		}
	}

	void PartyOutfit::Save(SKSE::SerializationInterface* a_intfc) const
	{
		if (!a_intfc->OpenRecord(kRecord, kRecordVersion)) {
			logs::error("could not open co-save record QOUT");
			return;
		}
		const auto count = static_cast<std::uint32_t>(stored.size());
		a_intfc->WriteRecordData(count);
		for (const auto id : stored) {
			a_intfc->WriteRecordData(id);
		}
	}

	void PartyOutfit::Load(SKSE::SerializationInterface* a_intfc, std::uint32_t)
	{
		std::uint32_t count = 0;
		if (a_intfc->ReadRecordData(count) != sizeof(count) || count > kMaxStored) {
			logs::warn("co-save record QOUT is damaged; ignoring it");
			return;
		}
		stored.clear();
		for (std::uint32_t i = 0; i < count; ++i) {
			RE::FormID saved = 0;
			RE::FormID current = 0;
			if (a_intfc->ReadRecordData(saved) != sizeof(saved)) {
				return;
			}
			if (a_intfc->ResolveFormID(saved, current)) {
				stored.push_back(current);
			}
		}
		Log("loaded stored gear: {}", stored.size());
	}

	void PartyOutfit::Revert()
	{
		stored.clear();
	}
}
