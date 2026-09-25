#include "GearSwap.h"

#include "Util.h"

namespace CIGAR
{
	namespace
	{
		using Slot = RE::BGSBipedObjectForm::BipedObjectSlot;

		// The four body parts SI names (SwapOutfitHead/Chest/Arms/Legs) and the slot that stands for each.
		struct Part
		{
			Slot        slot;
			const char* name;
		};
		constexpr std::array kParts{ Part{ Slot::kHead, "머리" }, Part{ Slot::kBody, "몸" }, Part{ Slot::kHands, "손" },
			Part{ Slot::kFeet, "발" } };
	}

	GearSwap::GearSwap()
	{
		swap.SetPromptType(SkyPromptAPI::kHold);
	}

	GearSwap* GearSwap::GetSingleton()
	{
		static GearSwap singleton;
		return &singleton;
	}

	void GearSwap::OnGameLoaded()
	{
		swap.Reset();
		lastGate.clear();
		offered = {};
		Log("ready");
	}

	float GearSwap::Rating(const RE::TESObjectARMO* a_armor)
	{
		return a_armor ? static_cast<float>(a_armor->armorRating) / 100.0f : 0.0f;
	}

	bool GearSwap::SameClass(const RE::TESObjectARMO* a_a, const RE::TESObjectARMO* a_b)
	{
		return a_a->IsHeavyArmor() == a_b->IsHeavyArmor() && a_a->IsLightArmor() == a_b->IsLightArmor();
	}

	void GearSwap::Consider(RE::PlayerCharacter* a_player, RE::TESObjectARMO* a_armor, Offer& a_best) const
	{
		if (!a_armor || !a_armor->GetPlayable()) {
			return;
		}
		const char* name = a_armor->GetName();
		if (!name || !*name) {
			return;
		}
		for (const auto& part : kParts) {
			if (!a_armor->HasPartOf(part.slot)) {
				continue;
			}
			// A swap replaces a worn piece. An empty part is left alone: the helmet is off by the
			// user's choice (docs/028), and a player undressed at home is not offered the chest.
			auto* worn = a_player->GetWornArmor(part.slot);
			if (!worn || worn == a_armor) {
				return;
			}
			float gain = 0.0f;
			if (SameClass(worn, a_armor) && Rating(a_armor) > Rating(worn)) {
				gain = Rating(a_armor) - Rating(worn);
			}
			if (gain > a_best.gain) {
				a_best.armor = a_armor;
				a_best.part = part.name;
				a_best.gain = gain;
			}
			return;  // an armor is judged on the first part it covers
		}
	}

	GearSwap::Offer GearSwap::Evaluate(RE::PlayerCharacter* a_player) const
	{
		Offer best;
		RE::ObjectRefHandle aimed;
		if (auto* pick = RE::CrosshairPickData::GetSingleton()) {
			aimed = pick->GetActiveTarget();
		}
		const auto ref = aimed.get();
		if (!ref || ref.get() == a_player) {
			best.why = "nothing aimed";
			return best;
		}
		auto* base = ref->GetBaseObject();
		auto* actor = ref->As<RE::Actor>();
		if (actor && !actor->IsDead()) {
			best.why = "living actor";
			return best;
		}
		if (ref->IsCrimeToActivate()) {
			best.why = "owned (taking it is a crime)";
			return best;
		}
		best.source = aimed;
		if (auto* armor = base ? base->As<RE::TESObjectARMO>() : nullptr) {
			Consider(a_player, armor, best);
		} else if (actor || (base && base->Is(RE::FormType::Container))) {
			if (ref->IsLocked()) {
				best.why = "locked";
				return best;
			}
			for (const auto& [object, data] : ref->GetInventory()) {
				if (data.first > 0 && object && data.second && !data.second->IsQuestObject()) {
					Consider(a_player, object->As<RE::TESObjectARMO>(), best);
				}
			}
		} else {
			best.why = "not armor or a container";
			return best;
		}
		best.why = best.armor ? "better piece" : "nothing better";
		return best;
	}

	void GearSwap::Tick()
	{
		auto* player = Util::Player();
		if (!player) {
			return;
		}
		const auto offer = Evaluate(player);
		const bool combat = player->IsInCombat();
		const auto* controls = RE::ControlMap::GetSingleton();
		const bool movable = controls && controls->IsMovementControlsEnabled();
		LogGate(std::format("aim={} piece={} part={} gain={:.1f} state={} combat={} movable={}",
			offer.source.get() ? Util::NameOf(offer.source.get().get()) : "-"s, offer.armor ? Util::NameOf(offer.armor) : "-"s,
			offer.part, offer.gain, offer.why, combat, movable));
		const bool can = offer.armor && !combat && movable;
		if (can && offered.armor && offered.armor != offer.armor) {
			swap.Withdraw();
			swap.Reset();
		}
		offered = can ? offer : Offer{};
		auto* armor = offer.armor;
		const std::string part = offer.part;
		swap.Update(can, [armor, part] { return std::format("{} 장비 교체 (길게): {}", part, Util::NameOf(armor)); });
	}

	void GearSwap::OnAccepted(std::uint16_t a_eventID)
	{
		if (a_eventID != kSwap) {
			return;
		}
		auto* player = Util::Player();
		const auto offer = player ? Evaluate(player) : Offer{};
		const auto source = offer.source.get();
		if (!offer.armor || !source) {
			Log("accept ignored: {}", offer.why);
			return;
		}
		if (source->GetBaseObject() == offer.armor) {
			player->PickUpObject(source.get(), 1);
		} else {
			source->RemoveItem(offer.armor, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, player);
		}
		if (Util::ItemCount(player, offer.armor) <= 0) {
			Log("WARN {} did not reach the inventory", Util::NameOf(offer.armor));
			Util::Notify("CIGAR: 장비 가져오기 실패. 로그 확인");
			return;
		}
		RE::ActorEquipManager::GetSingleton()->EquipObject(player, offer.armor);
		Log("swapped {} for the {} part (+{:.1f}) from {}", Util::NameOf(offer.armor), offer.part, offer.gain,
			Util::NameOf(source.get()));
	}
}
