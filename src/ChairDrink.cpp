#include "ChairDrink.h"

#include "Rest.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		// Alcohol on this order: Gourmet tags vanilla ale, mead and wine MAG_FoodTypeAle / Wine; OCF,
		// Hunterborn-style and vendor keywords cover the rest. Resolved by editor ID, so a missing one
		// is simply skipped.
		constexpr std::array kAlcoholKeywords{ "MAG_FoodTypeAle"sv, "MAG_FoodTypeWine"sv, "OCF_AlchDrinkAlcohol"sv,
			"VendorItemDrinkAlcoholModerate"sv, "VendorItemDrinkAlcoholStrong"sv, "_SH_AlcoholDrinkKeyword"sv };
		// The places the user named: inns and houses (the player's own included).
		constexpr std::array kPlaceKeywords{ "LocTypeInn"sv, "LocTypeHouse"sv, "LocTypePlayerHouse"sv };
		constexpr auto kDrinkEvent = "ChairDrinkingStart"sv;
		constexpr auto kCheckDelay = 1s;
		// Movement input held this long while still seated means the drinking idle kept the player in
		// the chair.
		constexpr auto kStuckAfter = 3s;
	}

	ChairDrink::ChairDrink()
	{
		prompt.SetPromptType(SkyPromptAPI::kHold);
		prompt.SetRepeat(true);
	}

	ChairDrink* ChairDrink::GetSingleton()
	{
		static ChairDrink singleton;
		return &singleton;
	}

	void ChairDrink::OnGameLoaded()
	{
		prompt.Reset();
		lastGate.clear();
		checkPending = false;
		drinking = false;
		moving = false;
		dismissed = false;
		alcoholKeywords.clear();
		placeKeywords.clear();
		std::string found;
		for (const auto name : kAlcoholKeywords) {
			if (auto* keyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>(name)) {
				alcoholKeywords.push_back(keyword);
				found += std::format(" {}", name);
			}
		}
		for (const auto name : kPlaceKeywords) {
			if (auto* keyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>(name)) {
				placeKeywords.push_back(keyword);
			}
		}
		Log("ready: alcohol keywords [{} ] places {}/{}", found, placeKeywords.size(), kPlaceKeywords.size());
		if (alcoholKeywords.empty() || placeKeywords.empty()) {
			Log("WARN a keyword list is empty: the prompt never shows");
			Util::Notify("CIGAR: 의자 음주 키워드를 찾지 못함. 로그 확인");
		}
	}

	bool ChairDrink::AtInnOrHome(RE::PlayerCharacter* a_player, std::string& a_where) const
	{
		auto* location = a_player->GetCurrentLocation();
		a_where = location ? Util::NameOf(location) : "no location"s;
		for (auto* loc = location; loc; loc = loc->parentLoc) {
			for (auto* keyword : placeKeywords) {
				if (loc->HasKeyword(keyword)) {
					a_where += std::format(" [{}]", keyword->GetFormEditorID());
					return true;
				}
			}
		}
		return false;
	}

	bool ChairDrink::IsAlcohol(const RE::AlchemyItem* a_item) const
	{
		if (!a_item || a_item->IsPoison()) {
			return false;
		}
		return std::ranges::any_of(alcoholKeywords, [a_item](RE::BGSKeyword* a_keyword) { return a_item->HasKeyword(a_keyword); });
	}

	RE::AlchemyItem* ChairDrink::PickDrink(RE::PlayerCharacter* a_player) const
	{
		RE::AlchemyItem* best = nullptr;
		std::int32_t bestValue = 0;
		const auto inventory = a_player->GetInventory([](RE::TESBoundObject& a_obj) { return a_obj.Is(RE::FormType::AlchemyItem); });
		for (const auto& [object, data] : inventory) {
			auto* item = object ? object->As<RE::AlchemyItem>() : nullptr;
			if (!item || data.first <= 0 || !IsAlcohol(item) || (data.second && data.second->IsQuestObject())) {
				continue;
			}
			const auto value = item->GetGoldValue();
			if (!best || value < bestValue || (value == bestValue && item->GetFormID() < best->GetFormID())) {
				best = item;
				bestValue = value;
			}
		}
		return best;
	}

	void ChairDrink::Tick()
	{
		auto* player = Util::Player();
		const auto now = Clock::now();
		const bool chair = Rest::InChair(player);

		if (checkPending && now >= checkAt) {
			checkPending = false;
			const auto count = drank ? Util::ItemCount(player, drank) : 0;
			Log("after drinking {}: {} -> {} carried, still seated={}", drank ? Util::NameOf(drank) : "-"s, countBefore, count, chair);
			if (count >= countBefore) {
				Log("WARN the drink was not consumed");
				Util::Notify("CIGAR: 술을 마시지 못함. 로그 확인");
			}
		}

		// Getting up is the game's own chair exit; watch that the drinking idle does not block it.
		if (drinking) {
			const auto* controls = RE::PlayerControls::GetSingleton();
			const bool input = controls && controls->data.moveInputVec.Length() > 0.1f;
			if (!chair) {
				Log("got up after drinking");
				drinking = false;
				moving = false;
			} else if (!input) {
				moving = false;
			} else if (!moving) {
				moving = true;
				moveSince = now;
			} else if (now - moveSince >= kStuckAfter && !warnedStuck) {
				warnedStuck = true;
				Log("WARN still seated {}s into movement input after the drinking idle",
					std::chrono::duration_cast<std::chrono::seconds>(kStuckAfter).count());
				Util::Notify("CIGAR: 음주 후 일어나지 못함. 로그 확인");
			}
		}

		if (dismissed && !chair) {
			dismissed = false;
			Log("stood up: 마시기 may show again");
		}

		std::string where;
		const bool place = chair && AtInnOrHome(player, where);
		const bool combat = player->IsInCombat();
		auto* drink = chair && place && !combat ? PickDrink(player) : nullptr;

		LogGate(std::format("chair={} place={} ({}) combat={} drink={} dismissed={}", chair, place, chair ? where : "-"s, combat,
			drink ? Util::NameOf(drink) : "-"s, dismissed));

		prompt.Update(drink != nullptr && !dismissed, [drink] { return std::format("마시기 (길게): {}", Util::NameOf(drink)); });
	}

	void ChairDrink::OnDeclined(std::uint16_t a_eventID)
	{
		if (a_eventID != kDrink) {
			return;
		}
		dismissed = true;
		prompt.Withdraw();
		Log("마시기 declined: hidden until the player stands up");
	}

	void ChairDrink::OnAccepted(std::uint16_t a_eventID)
	{
		if (a_eventID != kDrink) {
			return;
		}
		auto* player = Util::Player();
		std::string where;
		if (!Rest::InChair(player) || !AtInnOrHome(player, where) || player->IsInCombat()) {
			Log("accept ignored: no longer seated at an inn or home");
			return;
		}
		auto* drink = PickDrink(player);
		if (!drink) {
			Log("accept ignored: no alcohol carried");
			return;
		}
		// One drinking idle at a time; a second bottle drinks on without restarting it.
		const bool animated = drinking || player->NotifyAnimationGraph(RE::BSFixedString{ kDrinkEvent });
		countBefore = Util::ItemCount(player, drink);
		RE::ActorEquipManager::GetSingleton()->EquipObject(player, drink);
		drank = drink;
		checkPending = true;
		checkAt = Clock::now() + kCheckDelay;
		if (animated) {
			drinking = true;
			warnedStuck = false;
		}
		Log("drank {} ({:08X}) at {}; {}={}", Util::NameOf(drink), drink->GetFormID(), where, kDrinkEvent, animated);
		if (!animated) {
			Log("WARN the graph refused {}: the drink was taken without the animation", kDrinkEvent);
		}
	}
}
