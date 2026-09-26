#include "ChairDrink.h"

#include "Rest.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		// The base game tags no drink as alcohol, so its 29 are known by FormID (read from Skyrim.esm,
		// HearthFires.esm and Dragonborn.esm with houseCARL, 2026-09-26). The keywords below add the
		// drinks other mods tag.
		struct DrinkRef
		{
			RE::FormID id;
			std::string_view plugin;
		};
		constexpr std::array kBaseGameDrinks{
			DrinkRef{ 0x034C5E, "Skyrim.esm"sv },     // Ale
			DrinkRef{ 0x09380D, "Skyrim.esm"sv },     // Argonian Ale
			DrinkRef{ 0x034C5D, "Skyrim.esm"sv },     // Nord Mead
			DrinkRef{ 0x02C35A, "Skyrim.esm"sv },     // Black-Briar Mead
			DrinkRef{ 0x0F693F, "Skyrim.esm"sv },     // Black-Briar Reserve
			DrinkRef{ 0x0508CA, "Skyrim.esm"sv },     // Honningbrew Mead
			DrinkRef{ 0x0555E8, "Skyrim.esm"sv },     // Dragon's Breath Mead
			DrinkRef{ 0x03133C, "Skyrim.esm"sv },     // Wine
			DrinkRef{ 0x0C5348, "Skyrim.esm"sv },     // Wine
			DrinkRef{ 0x03133B, "Skyrim.esm"sv },     // Alto Wine
			DrinkRef{ 0x0C5349, "Skyrim.esm"sv },     // Alto Wine
			DrinkRef{ 0x0F257E, "Skyrim.esm"sv },     // Jessica's Wine
			DrinkRef{ 0x085368, "Skyrim.esm"sv },     // Spiced Wine
			DrinkRef{ 0x01895F, "Skyrim.esm"sv },     // Firebrand Wine
			DrinkRef{ 0x0B91D7, "Skyrim.esm"sv },     // Cyrodilic Brandy
			DrinkRef{ 0x036D53, "Skyrim.esm"sv },     // Colovian Brandy
			DrinkRef{ 0x0D055E, "Skyrim.esm"sv },     // Stros M'Kai Rum
			DrinkRef{ 0x065C37, "Skyrim.esm"sv },     // Velvet LeChance
			DrinkRef{ 0x065C38, "Skyrim.esm"sv },     // White-Gold Tower
			DrinkRef{ 0x065C39, "Skyrim.esm"sv },     // Cliff Racer
			DrinkRef{ 0x003536, "HearthFires.esm"sv },  // Surilie Brothers Wine
			DrinkRef{ 0x003535, "HearthFires.esm"sv },  // Argonian Bloodwine
			DrinkRef{ 0x03572F, "Dragonborn.esm"sv },   // Ashfire Mead
			DrinkRef{ 0x0320DF, "Dragonborn.esm"sv },   // Emberbrand Wine
			DrinkRef{ 0x024E0B, "Dragonborn.esm"sv },   // Sadri's Sujamma
			DrinkRef{ 0x0207E6, "Dragonborn.esm"sv },   // Sujamma
			DrinkRef{ 0x0248CE, "Dragonborn.esm"sv },   // Matze
			DrinkRef{ 0x0248CC, "Dragonborn.esm"sv },   // Shein
			DrinkRef{ 0x0207E5, "Dragonborn.esm"sv },   // Flin
		};
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
		alcoholForms.clear();
		placeKeywords.clear();
		std::string found;
		if (auto* handler = RE::TESDataHandler::GetSingleton()) {
			for (const auto& ref : kBaseGameDrinks) {
				if (const auto id = handler->LookupFormID(ref.id, ref.plugin); id != 0) {
					alcoholForms.push_back(id);
				}
			}
		}
		found = std::format(" {} of {} base-game drinks;", alcoholForms.size(), kBaseGameDrinks.size());
		for (const auto name : kAlcoholKeywords) {
			if (auto* keyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>(name)) {
				alcoholKeywords.push_back(keyword);
				found += std::format(" {}", name);
			}
		}
		const bool noAlcohol = alcoholForms.empty() && alcoholKeywords.empty();
		for (const auto name : kPlaceKeywords) {
			if (auto* keyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>(name)) {
				placeKeywords.push_back(keyword);
			}
		}
		Log("ready: alcohol [{} ] places {}/{}", found, placeKeywords.size(), kPlaceKeywords.size());
		if (noAlcohol || placeKeywords.empty()) {
			Log("WARN no alcohol or no place keyword resolved: the prompt never shows");
			Util::Notify(Text::L("CIGAR: 의자 음주 대상을 찾지 못함. 로그 확인", "CIGAR: No drinks or places found for chair drinking. See the log"));
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
		return std::ranges::find(alcoholForms, a_item->GetFormID()) != alcoholForms.end() ||
		       std::ranges::any_of(alcoholKeywords, [a_item](RE::BGSKeyword* a_keyword) { return a_item->HasKeyword(a_keyword); });
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
				Util::Notify(Text::L("CIGAR: 술을 마시지 못함. 로그 확인", "CIGAR: Could not drink. See the log"));
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
				Util::Notify(Text::L("CIGAR: 음주 후 일어나지 못함. 로그 확인", "CIGAR: Could not stand up after drinking. See the log"));
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

		prompt.Update(drink != nullptr && !dismissed, [drink] { return Text::F("마시기 (길게): {}", "Drink (hold): {}", Util::NameOf(drink)); });
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
