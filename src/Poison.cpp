#include "Poison.h"

#include "Util.h"

namespace CIGAR
{
	namespace
	{
		constexpr auto kSexLabPlugin = "SexLab.esm"sv;
		constexpr RE::FormID kSexLabAnimatingID = 0xE50F;
	}

	Poison::Poison()
	{
		apply.SetPromptType(SkyPromptAPI::kHold);
	}

	Poison* Poison::GetSingleton()
	{
		static Poison singleton;
		return &singleton;
	}

	void Poison::OnGameLoaded()
	{
		apply.Reset();
		lastGate.clear();
		auto* handler = RE::TESDataHandler::GetSingleton();
		sexlabAnimating = handler && handler->LookupModByName(kSexLabPlugin)
		                      ? handler->LookupForm<RE::TESFaction>(kSexLabAnimatingID, kSexLabPlugin)
		                      : nullptr;
		Log("ready: sexlab={}", sexlabAnimating != nullptr);
	}

	Poison::Pick Poison::Evaluate(RE::PlayerCharacter* a_player) const
	{
		Pick pick;
		const auto* state = a_player->AsActorState();
		if (!state || !state->IsWeaponDrawn()) {
			pick.why = "weapon sheathed";
			return pick;
		}
		pick.entry = a_player->GetEquippedEntryData(false);
		pick.weapon = pick.entry && pick.entry->object ? pick.entry->object->As<RE::TESObjectWEAP>() : nullptr;
		if (!pick.weapon || pick.weapon->IsStaff() || pick.weapon->IsHandToHandMelee()) {
			pick.why = "no weapon in the right hand";
			return pick;
		}
		if (pick.entry->IsPoisoned()) {
			pick.why = "already poisoned";
			return pick;
		}
		std::int32_t best = -1;
		for (const auto& [object, data] : a_player->GetInventory()) {
			auto* alchemy = object ? object->As<RE::AlchemyItem>() : nullptr;
			if (!alchemy || !alchemy->IsPoison() || data.first <= 0) {
				continue;
			}
			const auto value = alchemy->GetGoldValue();
			if (value > best) {
				best = value;
				pick.poison = alchemy;
			}
		}
		pick.why = pick.poison ? "ready" : "no poison carried";
		return pick;
	}

	void Poison::Tick()
	{
		auto* player = Util::Player();
		if (!player) {
			return;
		}
		const auto pick = Evaluate(player);
		const auto* controls = RE::ControlMap::GetSingleton();
		const bool movable = controls && controls->IsMovementControlsEnabled();
		const bool sexlab = sexlabAnimating && player->IsInFaction(sexlabAnimating);
		LogGate(std::format("weapon={} poison={} state={} movable={} sexlab={}",
			pick.weapon ? Util::NameOf(pick.weapon) : "-"s, pick.poison ? Util::NameOf(pick.poison) : "-"s, pick.why,
			movable, sexlab));
		auto* poison = pick.poison;
		apply.Update(poison && movable && !sexlab, [poison] { return std::format("독 바르기 (길게): {}", Util::NameOf(poison)); });
	}

	void Poison::OnAccepted(std::uint16_t a_eventID)
	{
		if (a_eventID != kApply) {
			return;
		}
		auto* player = Util::Player();
		if (!player) {
			return;
		}
		const auto pick = Evaluate(player);
		if (!pick.poison) {
			Log("accept ignored: {}", pick.why);
			return;
		}
		// Concentrated Poison and similar perks raise the doses through this entry point.
		float doses = 1.0f;
		RE::BGSEntryPoint::HandleEntryPoint(RE::BGSEntryPoint::ENTRY_POINTS::kModPoisonDoseCount, player, &doses);
		const auto count = static_cast<std::uint32_t>(std::max(1.0f, doses));
		pick.entry->PoisonObject(pick.poison, count);
		player->RemoveItem(pick.poison, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
		auto* after = player->GetEquippedEntryData(false);
		const bool poisoned = after && after->IsPoisoned();
		Log("applied {} ({:08X}) to {}: doses={} poisoned after={}", Util::NameOf(pick.poison), pick.poison->GetFormID(),
			Util::NameOf(pick.weapon), count, poisoned);
		if (!poisoned) {
			Util::Notify("CIGAR: 독 바르기 확인 실패. 로그 확인");
		}
	}
}
