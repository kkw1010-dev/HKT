#include "Recharge.h"

#include "Util.h"

namespace CIGAR
{
	namespace
	{
		// SI offers the prompt when "the player has a weapon with low charge" (its MCM text); its
		// number is not in the settings or the strings. A quarter is CIGAR's choice.
		constexpr float kLowCharge = 0.25f;
		// SI's recharge_weapon_oooc ("only prompt ... when the player is out of combat") is on.
		// The engine applies the restore at once; the check waits a moment all the same.
		constexpr auto kCheckAfter = 1s;
		constexpr RE::FormID kReusableKeywordID = 0x0ED2F1;  // ReusableSoulGem, Skyrim.esm

		const char* SoulSetting(RE::SOUL_LEVEL a_soul)
		{
			switch (a_soul) {
			case RE::SOUL_LEVEL::kPetty:
				return "iSoulLevelValuePetty";
			case RE::SOUL_LEVEL::kLesser:
				return "iSoulLevelValueLesser";
			case RE::SOUL_LEVEL::kCommon:
				return "iSoulLevelValueCommon";
			case RE::SOUL_LEVEL::kGreater:
				return "iSoulLevelValueGreater";
			case RE::SOUL_LEVEL::kGrand:
				return "iSoulLevelValueGrand";
			default:
				return nullptr;
			}
		}

		RE::ActorValue ChargeValue(bool a_left)
		{
			return a_left ? RE::ActorValue::kLeftItemCharge : RE::ActorValue::kRightItemCharge;
		}

		const char* HandTag(bool a_left)
		{
			return a_left ? "left" : "right";
		}
	}

	Recharge::Recharge()
	{
		prompt.SetPromptType(SkyPromptAPI::kHold);
		prompt.SetRepeat(true);
	}

	Recharge* Recharge::GetSingleton()
	{
		static Recharge singleton;
		return &singleton;
	}

	void Recharge::OnGameLoaded()
	{
		prompt.Reset();
		lastGate.clear();
		checkAfterAccept = false;
		explainedNoGem = false;
		reusableKeyword = RE::TESForm::LookupByID<RE::BGSKeyword>(kReusableKeywordID);

		// Mod Soul Gem Recharge's filter tabs, from the engine's own table: the arguments after the
		// perk owner must match them, so the call is only made when their shape is understood.
		perkArgument = -1;
		std::string names;
		if (const auto* entry = RE::BGSEntryPoint::GetEntryPoint(RE::BGSEntryPoint::ENTRY_POINTS::kModSoulGemRecharge)) {
			std::vector<std::string> others;
			for (std::uint32_t i = 0; i < entry->parameters.count && entry->parameters.data; ++i) {
				const char* name = entry->parameters.data[i].name;
				const std::string text = name ? name : "?";
				names += std::format("{}{}", names.empty() ? "" : ", ", text);
				if (!Util::ContainsNoCase(text, "Owner")) {
					others.push_back(text);
				}
			}
			if (others.empty()) {
				perkArgument = 0;
			} else if (others.size() == 1) {
				perkArgument = Util::ContainsNoCase(others[0], "Soul") ? 1 : 2;
			}
		}

		std::string values;
		auto* settings = RE::GameSettingCollection::GetSingleton();
		for (auto soul : { RE::SOUL_LEVEL::kPetty, RE::SOUL_LEVEL::kLesser, RE::SOUL_LEVEL::kCommon, RE::SOUL_LEVEL::kGreater, RE::SOUL_LEVEL::kGrand }) {
			auto* setting = settings ? settings->GetSetting(SoulSetting(soul)) : nullptr;
			values += std::format("{}{}", values.empty() ? "" : "/", setting ? std::to_string(setting->GetInteger()) : "?"s);
		}
		Util::WarnIfSIModuleOn("ItemUse.enabled_recharge_weapon", "/MCP/modules/ItemUse/enabled_recharge_weapon");
		Log("ready: soul values {} reusableKeyword={} perk entry point tabs [{}] -> {}", values, reusableKeyword != nullptr, names,
			perkArgument == 0 ? "owner only" : perkArgument == 1 ? "soul gem" : perkArgument == 2 ? "weapon" : "not understood, perks skipped");
	}

	std::optional<Recharge::Hand> Recharge::ReadHand(RE::PlayerCharacter* a_player, bool a_left)
	{
		auto* entry = a_player->GetEquippedEntryData(a_left);
		auto* weapon = entry && entry->object ? entry->object->As<RE::TESObjectWEAP>() : nullptr;
		if (!weapon || !entry->GetEnchantment()) {
			return std::nullopt;
		}
		// A two-handed weapon is the same entry in both hands; it is read once, as the right.
		if (a_left && a_player->GetEquippedEntryData(false) == entry) {
			return std::nullopt;
		}

		Hand hand;
		hand.left = a_left;
		hand.weapon = weapon;
		if (entry->extraLists) {
			for (auto* xList : *entry->extraLists) {
				if (!xList) {
					continue;
				}
				if (!hand.xList) {
					hand.xList = xList;
				}
				if (a_left ? xList->HasType<RE::ExtraWornLeft>() : xList->HasType<RE::ExtraWorn>()) {
					hand.xList = xList;
					break;
				}
			}
		}

		const auto* xEnchant = hand.xList ? hand.xList->GetByType<RE::ExtraEnchantment>() : nullptr;
		hand.itemMax = xEnchant && xEnchant->enchantment ? static_cast<float>(xEnchant->charge) : static_cast<float>(weapon->amountofEnchantment);
		const auto* xCharge = hand.xList ? hand.xList->GetByType<RE::ExtraCharge>() : nullptr;
		hand.itemCurrent = xCharge ? xCharge->charge : hand.itemMax;

		auto* owner = a_player->AsActorValueOwner();
		hand.avMax = owner->GetPermanentActorValue(ChargeValue(a_left));
		hand.avCurrent = owner->GetActorValue(ChargeValue(a_left));

		hand.fromAV = hand.avMax > 0.0f;
		hand.current = hand.fromAV ? hand.avCurrent : hand.itemCurrent;
		hand.max = hand.fromAV ? hand.avMax : hand.itemMax;
		if (hand.max <= 0.0f) {
			return std::nullopt;
		}
		return hand;
	}

	std::optional<Recharge::Hand> Recharge::NeedyHand(RE::PlayerCharacter* a_player)
	{
		std::optional<Hand> best;
		for (bool left : { false, true }) {
			auto hand = ReadHand(a_player, left);
			if (hand && hand->Ratio() <= kLowCharge && (!best || hand->Ratio() < best->Ratio())) {
				best = hand;
			}
		}
		return best;
	}

	float Recharge::Worth(RE::PlayerCharacter* a_player, RE::TESSoulGem* a_gem, RE::SOUL_LEVEL a_soul, RE::TESObjectWEAP* a_weapon) const
	{
		auto* settings = RE::GameSettingCollection::GetSingleton();
		const char* name = SoulSetting(a_soul);
		auto* setting = settings && name ? settings->GetSetting(name) : nullptr;
		float value = setting ? static_cast<float>(setting->GetInteger()) : 0.0f;
		if (value <= 0.0f) {
			return 0.0f;
		}
		switch (perkArgument) {
		case 0:
			RE::BGSEntryPoint::HandleEntryPoint(RE::BGSEntryPoint::ENTRY_POINTS::kModSoulGemRecharge, a_player, &value);
			break;
		case 1:
			RE::BGSEntryPoint::HandleEntryPoint(RE::BGSEntryPoint::ENTRY_POINTS::kModSoulGemRecharge, a_player, static_cast<RE::TESForm*>(a_gem), &value);
			break;
		case 2:
			RE::BGSEntryPoint::HandleEntryPoint(RE::BGSEntryPoint::ENTRY_POINTS::kModSoulGemRecharge, a_player, static_cast<RE::TESForm*>(a_weapon), &value);
			break;
		default:
			break;
		}
		return std::max(value, 0.0f);
	}

	Recharge::Pick Recharge::Scan(RE::PlayerCharacter* a_player, const Hand& a_hand) const
	{
		Pick result;
		std::vector<Gem> gems;
		const auto skip = [&result](const RE::TESSoulGem* a_gem, const char* a_why) {
			++result.skipped;
			if (result.skippedWhy.size() < 200) {
				result.skippedWhy += std::format("{}{}({})", result.skippedWhy.empty() ? "" : ", ", Util::NameOf(a_gem), a_why);
			}
		};

		const auto inventory = a_player->GetInventory([](RE::TESBoundObject& a_obj) { return a_obj.Is(RE::FormType::SoulGem); });
		for (const auto& [object, data] : inventory) {
			auto* gem = object ? object->As<RE::TESSoulGem>() : nullptr;
			if (!gem || data.first <= 0) {
				continue;
			}
			const bool reusable = reusableKeyword && gem->HasKeyword(reusableKeyword);
			// Instances that carry their own soul (a gem the player filled) sit in their own extra
			// lists; the rest of the stack is the base form's contained soul.
			std::int32_t inLists = 0;
			if (data.second && data.second->extraLists) {
				for (auto* xList : *data.second->extraLists) {
					if (!xList) {
						continue;
					}
					const auto count = xList->GetCount();
					inLists += count;
					const auto soul = xList->GetSoulLevel();
					if (soul == RE::SOUL_LEVEL::kNone || count <= 0) {
						continue;
					}
					++result.filled;
					if (xList->HasQuestObjectAlias()) {
						skip(gem, "quest");
						continue;
					}
					gems.push_back({ gem, xList, soul, reusable, 0.0f });
				}
			}
			const auto plain = data.first - inLists;
			const auto soul = gem->GetContainedSoul();
			if (plain <= 0 || soul == RE::SOUL_LEVEL::kNone) {
				continue;
			}
			++result.filled;
			// A reusable gem that comes filled as its own form has no empty form to give back.
			if (reusable) {
				skip(gem, "reusable, pre-filled");
				continue;
			}
			gems.push_back({ gem, nullptr, soul, false, 0.0f });
		}

		// The smallest gem that fills what is missing, so a grand soul is not spent on a sliver;
		// failing that, the largest.
		const float missing = a_hand.Missing();
		std::optional<Gem> enough;
		std::optional<Gem> largest;
		for (auto& gem : gems) {
			gem.worth = Worth(a_player, gem.base, gem.soul, a_hand.weapon);
			if (gem.worth <= 0.0f) {
				skip(gem.base, "no worth");
				continue;
			}
			if (!largest || gem.worth > largest->worth) {
				largest = gem;
			}
			if (gem.worth >= missing && (!enough || gem.worth < enough->worth)) {
				enough = gem;
			}
		}
		result.gem = enough ? enough : largest;
		return result;
	}

	std::int32_t Recharge::FilledGemCount(RE::PlayerCharacter* a_player) const
	{
		std::int32_t total = 0;
		const auto inventory = a_player->GetInventory([](RE::TESBoundObject& a_obj) { return a_obj.Is(RE::FormType::SoulGem); });
		for (const auto& [object, data] : inventory) {
			auto* gem = object ? object->As<RE::TESSoulGem>() : nullptr;
			if (!gem || data.first <= 0) {
				continue;
			}
			std::int32_t inLists = 0;
			if (data.second && data.second->extraLists) {
				for (auto* xList : *data.second->extraLists) {
					if (xList) {
						inLists += xList->GetCount();
						total += xList->GetSoulLevel() != RE::SOUL_LEVEL::kNone ? xList->GetCount() : 0;
					}
				}
			}
			if (gem->GetContainedSoul() != RE::SOUL_LEVEL::kNone) {
				total += std::max(data.first - inLists, 0);
			}
		}
		return total;
	}

	void Recharge::Tick()
	{
		Util::WarnIfSIModuleOn("ItemUse.enabled_recharge_weapon", "/MCP/modules/ItemUse/enabled_recharge_weapon");
		auto* player = Util::Player();

		if (checkAfterAccept && Clock::now() - acceptedAt >= kCheckAfter) {
			checkAfterAccept = false;
			const auto hand = ReadHand(player, acceptedLeft);
			const float now = hand ? hand->current : 0.0f;
			const auto gems = FilledGemCount(player);
			Log("after recharge ({}): charge {:.0f} -> {:.0f} (expected {:.0f}; av {:.0f}/{:.0f} item {:.0f}/{:.0f}), filled gems {} -> {}",
				HandTag(acceptedLeft), chargeBefore, now, expected, hand ? hand->avCurrent : 0.0f, hand ? hand->avMax : 0.0f,
				hand ? hand->itemCurrent : 0.0f, hand ? hand->itemMax : 0.0f, gemsBefore, gems);
			if ((now <= chargeBefore + 0.5f || gems >= gemsBefore) && !warnedNoChange) {
				warnedNoChange = true;
				Log("WARN recharge did not take: the charge did not rise or no gem was spent");
				Util::Notify("CIGAR: 무기 충전 뒤 변화 없음. 로그 확인");
			}
		}

		const bool combat = player->IsInCombat();
		const auto* controls = RE::ControlMap::GetSingleton();
		const bool movable = controls && controls->IsMovementControlsEnabled();
		const auto right = ReadHand(player, false);
		const auto left = ReadHand(player, true);
		const auto hand = NeedyHand(player);

		std::optional<Gem> gem;
		std::size_t filled = 0;
		if (hand && !combat && movable) {
			auto pick = Scan(player, *hand);
			gem = pick.gem;
			filled = pick.filled;
			if (!gem && !explainedNoGem) {
				explainedNoGem = true;
				Log("low charge on {} ({:.0f}%) and no usable soul gem: {} filled{}", Util::NameOf(hand->weapon), hand->Ratio() * 100.0f,
					pick.filled, pick.skippedWhy.empty() ? ""s : std::format("; skipped: {}", pick.skippedWhy));
			}
			if (gem) {
				explainedNoGem = false;
			}
		}

		const auto describe = [](const std::optional<Hand>& a_hand) {
			return a_hand ? std::format("{:.0f}/{:.0f}{}", a_hand->current, a_hand->max, a_hand->fromAV ? "av" : "item") : "-"s;
		};
		LogGate(std::format("right={} left={} low={} combat={} movable={} gems={} pick={}",
			describe(right), describe(left), hand ? HandTag(hand->left) : "-", combat, movable,
			hand && !combat && movable ? std::to_string(filled) : "-"s,
			gem ? std::format("{}({:.0f})", Util::NameOf(gem->base), gem->worth) : "-"s));

		const bool available = hand && gem && !combat && movable;
		auto* weapon = hand ? hand->weapon : nullptr;
		const int percent = hand ? static_cast<int>(hand->Ratio() * 100.0f) : 0;
		prompt.Update(available, [weapon, percent] { return std::format("충전하기 (길게): {} ({}%)", Util::NameOf(weapon), percent); });
	}

	void Recharge::OnAccepted(std::uint16_t a_eventID)
	{
		if (a_eventID != kRecharge) {
			return;
		}
		auto* player = Util::Player();
		// Re-read: the hand, the charge or the gems may have changed while the prompt was up.
		const auto hand = NeedyHand(player);
		if (!hand) {
			Log("accept ignored: no equipped weapon is low any more");
			return;
		}
		if (player->IsInCombat()) {
			Log("accept ignored: in combat");
			return;
		}
		const auto pick = Scan(player, *hand);
		if (!pick.gem) {
			Log("accept ignored: no usable soul gem");
			return;
		}
		const auto& gem = *pick.gem;
		const float restore = std::min(gem.worth, hand->Missing());

		gemsBefore = FilledGemCount(player);
		chargeBefore = hand->current;
		expected = hand->current + restore;
		acceptedLeft = hand->left;

		// The gem first, so a failure never leaves a free recharge. A reusable one comes back empty.
		player->RemoveItem(gem.base, 1, RE::ITEM_REMOVE_REASON::kRemove, gem.xList, nullptr);
		if (gem.reusable) {
			player->AddObjectToContainer(gem.base, nullptr, 1, nullptr);
		}

		auto* owner = player->AsActorValueOwner();
		if (hand->fromAV) {
			owner->RestoreActorValue(ChargeValue(hand->left), restore);
		}
		if (auto* xCharge = hand->xList ? hand->xList->GetByType<RE::ExtraCharge>() : nullptr) {
			const float target = hand->fromAV ? expected : hand->itemCurrent + restore;
			xCharge->charge = std::min(target, hand->itemMax > 0.0f ? hand->itemMax : target);
		}

		acceptedAt = Clock::now();
		checkAfterAccept = true;
		Log("recharged {} ({}) with {} ({}, {}, worth {:.0f}): {:.0f} + {:.0f} of {:.0f}{}", Util::NameOf(hand->weapon), HandTag(hand->left),
			Util::NameOf(gem.base), static_cast<int>(gem.soul), gem.xList ? "player-filled" : "pre-filled", gem.worth,
			hand->current, restore, hand->max, gem.reusable ? " (reusable, returned empty)" : "");
	}
}
