#include "Eat.h"

#include "Settings.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		// Survival Mode (Creation Club) and its Update.esm forms. Survival Mode Improved (SKSE)
		// lowers Survival_HungerNeedValue on the equip event of any food carrying one of the four
		// Survival_FoodRestoreHunger effects, by the matching Survival_HungerRestore*Amount global
		// (colinswrath/Survival-Mode-Improved-SKSE, Events.h ProcessHungerOnEquipEvent).
		constexpr auto kSurvivalPlugin = "ccqdrsse001-survivalmode.esl"sv;
		constexpr RE::FormID kModeEnabledID = 0x826;  // Survival_ModeEnabled
		constexpr RE::FormID kHungerValueID = 0x81A;  // Survival_HungerNeedValue
		// Survival_HungerStage1Value .. Stage5Value
		constexpr std::array<RE::FormID, 5> kStageValueIDs{ 0x806, 0x802, 0x803, 0x804, 0x805 };
		constexpr RE::FormID kRawMeatListID = 0x8B0;  // Survival_FoodRawMeat

		constexpr auto kUpdatePlugin = "Update.esm"sv;
		// Survival_FoodRestoreHungerVerySmall, Small, Medium, Large
		constexpr std::array<RE::FormID, 4> kHungerEffectIDs{ 0x2EE1, 0x2EE2, 0x2EE3, 0x2EE4 };

		constexpr auto kSMIPlugin = "SurvivalModeImproved.esp"sv;
		constexpr RE::FormID kSMIHungerEnabledID = 0xF27;  // SMI_HungerShouldBeEnabled

		constexpr auto kSkyrimPlugin = "Skyrim.esm"sv;
		constexpr RE::FormID kVendorItemFoodRawID = 0xA0E56;  // SMI's second raw-food test

		// Gourmet (Simonrim) food types that are not a meal: raw, poisoned, drugs, ale, wine.
		constexpr auto kGourmetPlugin = "Gourmet.esp"sv;
		constexpr std::array<RE::FormID, 6> kGourmetExcludedIDs{ 0x808, 0xA6A, 0x969, 0xA4D, 0xA4B, 0xA4C };

		constexpr auto kSexLabPlugin = "SexLab.esm"sv;
		constexpr RE::FormID kSexLabAnimatingID = 0xE50F;

		// SMI lowers hunger on the equip event; give it and the eating animation time before the
		// gate is read again, so the same prompt is not offered for a second bite.
		constexpr auto kQuietAfterEat = 3s;
	}

	Eat::Eat()
	{
		eat.SetRepeat(true);
	}

	Eat* Eat::GetSingleton()
	{
		static Eat singleton;
		return &singleton;
	}

	void Eat::OnGameLoaded()
	{
		eat.Reset();
		lastGate.clear();
		active = false;
		offeredFood = nullptr;
		quietUntil = {};
		checkAfterEat = false;
		excludedKeywords.clear();

		auto* handler = RE::TESDataHandler::GetSingleton();
		if (!handler || !handler->LookupModByName(kSurvivalPlugin)) {
			Log("Survival Mode not found; the eat prompt is off");
			return;
		}
		modeEnabled = handler->LookupForm<RE::TESGlobal>(kModeEnabledID, kSurvivalPlugin);
		hungerValue = handler->LookupForm<RE::TESGlobal>(kHungerValueID, kSurvivalPlugin);
		bool stagesOk = true;
		for (std::size_t i = 0; i < kStageValueIDs.size(); ++i) {
			stageValues[i] = handler->LookupForm<RE::TESGlobal>(kStageValueIDs[i], kSurvivalPlugin);
			stagesOk = stagesOk && stageValues[i];
		}
		rawMeat = handler->LookupForm<RE::BGSListForm>(kRawMeatListID, kSurvivalPlugin);
		bool effectsOk = true;
		for (std::size_t i = 0; i < kHungerEffectIDs.size(); ++i) {
			hungerEffects[i] = handler->LookupForm<RE::EffectSetting>(kHungerEffectIDs[i], kUpdatePlugin);
			effectsOk = effectsOk && hungerEffects[i];
		}
		smiHungerEnabled = handler->LookupModByName(kSMIPlugin) ? handler->LookupForm<RE::TESGlobal>(kSMIHungerEnabledID, kSMIPlugin) : nullptr;
		sexlabAnimating = handler->LookupModByName(kSexLabPlugin) ? handler->LookupForm<RE::TESFaction>(kSexLabAnimatingID, kSexLabPlugin) : nullptr;

		if (auto* kw = handler->LookupForm<RE::BGSKeyword>(kVendorItemFoodRawID, kSkyrimPlugin)) {
			excludedKeywords.push_back(kw);
		}
		const bool gourmet = handler->LookupModByName(kGourmetPlugin) != nullptr;
		if (gourmet) {
			for (const auto id : kGourmetExcludedIDs) {
				if (auto* kw = handler->LookupForm<RE::BGSKeyword>(id, kGourmetPlugin)) {
					excludedKeywords.push_back(kw);
				}
			}
		}

		Log("Survival mode={} hunger={} stages={} effects={} rawList={} smi={} gourmet={} excludedKeywords={} sexlab={}",
			modeEnabled != nullptr, hungerValue != nullptr, stagesOk, effectsOk, rawMeat != nullptr,
			smiHungerEnabled != nullptr, gourmet, excludedKeywords.size(), sexlabAnimating != nullptr);
		if (!modeEnabled || !hungerValue || !stagesOk || !effectsOk) {
			Log("WARN Survival Mode found but its hunger forms did not resolve; the eat prompt is off");
			if (!warnedOff) {
				warnedOff = true;
				Util::Notify(Text::L("CIGAR: 서바이벌 허기 연동 실패. 먹기 프롬프트 비활성", "CIGAR: Survival hunger link failed. Eat prompt off"));
			}
			return;
		}
		active = true;
		Log("ready");
	}

	int Eat::HungerStage(float a_value) const
	{
		int stage = 0;
		for (std::size_t i = 0; i < stageValues.size(); ++i) {
			if (a_value >= stageValues[i]->value) {
				stage = static_cast<int>(i) + 1;
			}
		}
		return stage;
	}

	bool Eat::Edible(RE::AlchemyItem* a_food) const
	{
		if (!a_food || !a_food->IsFood()) {
			return false;
		}
		bool restores = false;
		for (const auto* effect : a_food->effects) {
			const auto* base = effect ? effect->baseEffect : nullptr;
			if (!base) {
				continue;
			}
			// A harmful effect (food poisoning, damage) rules the food out.
			if (base->IsHostile() || base->IsDetrimental()) {
				return false;
			}
			if (std::ranges::find(hungerEffects, base) != hungerEffects.end()) {
				restores = true;
			}
		}
		if (!restores) {
			return false;
		}
		if (rawMeat && rawMeat->HasForm(a_food)) {
			return false;
		}
		return std::ranges::none_of(excludedKeywords, [a_food](RE::BGSKeyword* a_kw) { return a_food->HasKeyword(a_kw); });
	}

	RE::AlchemyItem* Eat::PickFood(RE::PlayerCharacter* a_player, std::size_t& a_candidates) const
	{
		a_candidates = 0;
		RE::AlchemyItem* best = nullptr;
		std::int32_t bestValue = 0;
		float bestWeight = 0.0f;
		const auto inventory = a_player->GetInventory([](RE::TESBoundObject& a_obj) { return a_obj.Is(RE::FormType::AlchemyItem); });
		for (const auto& [object, data] : inventory) {
			auto* food = object ? object->As<RE::AlchemyItem>() : nullptr;
			if (data.first <= 0 || !Edible(food)) {
				continue;
			}
			if (data.second && data.second->IsQuestObject()) {
				continue;
			}
			++a_candidates;
			// Every hunger tier fully sates under Starfrost, so spend the cheapest food first.
			const auto value = food->GetGoldValue();
			const auto weight = food->GetWeight();
			const bool better = !best || value < bestValue ||
			                    (value == bestValue && (weight < bestWeight || (weight == bestWeight && food->GetFormID() < best->GetFormID())));
			if (better) {
				best = food;
				bestValue = value;
				bestWeight = weight;
			}
		}
		return best;
	}

	bool Eat::Live(RE::PlayerCharacter* a_player, std::string& a_gate, RE::AlchemyItem*& a_food) const
	{
		a_food = nullptr;
		const bool enabled = modeEnabled->value >= 1.0f && (!smiHungerEnabled || smiHungerEnabled->value >= 1.0f);
		const float hunger = hungerValue->value;
		const int stage = HungerStage(hunger);
		const int minStage = Settings::EatMinStage();
		const bool combat = a_player->IsInCombat();
		const auto* controls = RE::ControlMap::GetSingleton();
		const bool movable = controls && controls->IsMovementControlsEnabled();
		const bool sexlab = sexlabAnimating && a_player->IsInFaction(sexlabAnimating);
		const bool quiet = Clock::now() < quietUntil;

		const bool hungry = enabled && stage >= minStage;
		std::size_t candidates = 0;
		// The inventory is only scanned while the prompt could be shown.
		const bool ready = hungry && !combat && movable && !sexlab && !quiet;
		if (ready) {
			a_food = PickFood(a_player, candidates);
		}
		// The hunger value itself changes every few seconds; the gate logs the stage only.
		a_gate = std::format("survival={} stage={} min={} combat={} movable={} sexlab={} quiet={} foods={} pick={}",
			enabled, stage, minStage, combat, movable, sexlab, quiet, ready ? std::to_string(candidates) : "-"s,
			a_food ? Util::NameOf(a_food) : "-"s);
		return ready && a_food;
	}

	void Eat::Tick()
	{
		if (!active) {
			return;
		}
		auto* player = Util::Player();
		if (checkAfterEat && Clock::now() >= quietUntil) {
			checkAfterEat = false;
			const float after = hungerValue->value;
			Log("hunger after eating: {:.0f} (was {:.0f})", after, hungerBeforeEat);
			// Survival Mode Improved lowers hunger on the equip event; no drop means it did not see it.
			if (after >= hungerBeforeEat && hungerBeforeEat > 0.0f && !warnedNoDrop) {
				warnedNoDrop = true;
				Log("WARN hunger did not drop after eating; Survival Mode Improved may not have handled the equip");
				Util::Notify(Text::L("CIGAR: 먹기 후 허기 변화 없음. 로그 확인", "CIGAR: Hunger unchanged after eating. See the log"));
			}
		}
		std::string gate;
		RE::AlchemyItem* food = nullptr;
		const bool live = Live(player, gate, food);
		LogGate(std::move(gate));
		// The prompt names the food; when the pick changes, offer it again with the new name.
		if (eat.Offered() && food != offeredFood) {
			eat.Withdraw();
			eat.Reset();
		}
		offeredFood = live ? food : nullptr;
		eat.Update(live, [food] { return Text::F("먹기: {}", "Eat: {}", Util::NameOf(food)); });
	}

	void Eat::OnAccepted(std::uint16_t a_eventID)
	{
		if (!active || a_eventID != kEat) {
			return;
		}
		auto* player = Util::Player();
		std::string gate;
		RE::AlchemyItem* food = nullptr;
		// Re-check: the inventory or the situation may have changed while the prompt was up.
		if (!Live(player, gate, food)) {
			Log("accept ignored, no longer live: {}", gate);
			return;
		}
		const float before = hungerValue->value;
		auto* manager = RE::ActorEquipManager::GetSingleton();
		manager->EquipObject(player, food);
		quietUntil = Clock::now() + kQuietAfterEat;
		hungerBeforeEat = before;
		checkAfterEat = true;
		Log("ate {} ({:08X}, value {}) hunger before {:.0f}", Util::NameOf(food), food->GetFormID(), food->GetGoldValue(), before);
	}
}
