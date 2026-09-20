#include "Potion.h"

#include "Settings.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		constexpr auto kSexLabPlugin = "SexLab.esm"sv;
		constexpr RE::FormID kSexLabAnimatingID = 0xE50F;

		// The equip and the effect need a moment before the gate is read again, so the same prompt
		// is not offered for a second bottle. This is the settle time `Eat` uses, for the same
		// reason; SI's own `cooldown` value was not carried over, because what it covers in SI is
		// not documented and its DLL was not disassembled.
		constexpr auto kQuietAfterDrink = 3s;
		// The inventory is only re-read this often, because the gate itself runs ten times a second.
		constexpr auto kScanInterval = 1s;
		// How deep the player must be before a water-breathing potion is offered. 1.0 is fully
		// submerged; the head is under well before that.
		constexpr float kSubmergedEnough = 0.85f;

		using Archetype = RE::EffectArchetypes::ArchetypeID;

		RE::ActorValue ValueOf(Potion::Need a_need)
		{
			switch (a_need) {
			case Potion::Need::kHealth:
				return RE::ActorValue::kHealth;
			case Potion::Need::kStamina:
				return RE::ActorValue::kStamina;
			case Potion::Need::kMagicka:
				return RE::ActorValue::kMagicka;
			case Potion::Need::kWaterBreathing:
				return RE::ActorValue::kWaterBreathing;
			default:
				return RE::ActorValue::kNone;
			}
		}

		// A restore or fortify effect on a_value: the archetypes that raise an actor value.
		bool RaisesValue(const RE::EffectSetting* a_base, RE::ActorValue a_value)
		{
			if (a_base->data.primaryAV != a_value) {
				return false;
			}
			switch (a_base->GetArchetype()) {
			case Archetype::kValueModifier:
			case Archetype::kDualValueModifier:
			case Archetype::kPeakValueModifier:
				return true;
			default:
				return false;
			}
		}
	}

	Potion::Potion()
	{
		drink.SetRepeat(true);
	}

	Potion* Potion::GetSingleton()
	{
		static Potion singleton;
		return &singleton;
	}

	const char* Potion::NeedTag(Need a_need)
	{
		switch (a_need) {
		case Need::kHealth:
			return "health";
		case Need::kWaterBreathing:
			return "waterbreathing";
		case Need::kStamina:
			return "stamina";
		case Need::kMagicka:
			return "magicka";
		case Need::kCurePoison:
			return "curepoison";
		case Need::kCureDisease:
			return "curedisease";
		default:
			return "-";
		}
	}

	void Potion::OnGameLoaded()
	{
		drink.Reset();
		lastGate.clear();
		offeredNeed = Need::kNone;
		offeredPotion = nullptr;
		scannedAt = {};
		quietUntil = {};
		checkAfterDrink = false;

		auto* handler = RE::TESDataHandler::GetSingleton();
		sexlabAnimating = handler && handler->LookupModByName(kSexLabPlugin)
		                      ? handler->LookupForm<RE::TESFaction>(kSexLabAnimatingID, kSexLabPlugin)
		                      : nullptr;
		const auto tune = Settings::PotionTune();
		Log("ready: health={}({:.0f}%/{:.0f}%) stamina={}({:.0f}%) magicka={}({:.0f}%) curePoison={} cureDisease={} waterBreathing={} sexlab={}",
			tune.health, tune.healthThreshold * 100.0f, tune.urgentHealthThreshold * 100.0f,
			tune.stamina, tune.staminaThreshold * 100.0f, tune.magicka, tune.magickaThreshold * 100.0f,
			tune.curePoison, tune.cureDisease, tune.waterBreathing, sexlabAnimating != nullptr);
	}

	bool Potion::HasActiveValue(RE::Actor* a_actor, RE::ActorValue a_value)
	{
		auto* target = a_actor->AsMagicTarget();
		auto* effects = target ? target->GetActiveEffectList() : nullptr;
		if (!effects) {
			return false;
		}
		for (auto* active : *effects) {
			if (!active || active->flags.all(RE::ActiveEffect::Flag::kDispelled)) {
				continue;
			}
			const auto* base = active->GetBaseObject();
			if (base && !base->IsDetrimental() && RaisesValue(base, a_value)) {
				return true;
			}
		}
		return false;
	}

	bool Potion::Diseased(RE::Actor* a_actor)
	{
		auto* target = a_actor->AsMagicTarget();
		auto* effects = target ? target->GetActiveEffectList() : nullptr;
		if (!effects) {
			return false;
		}
		for (auto* active : *effects) {
			if (!active || !active->spell || active->flags.all(RE::ActiveEffect::Flag::kDispelled)) {
				continue;
			}
			if (active->spell->GetSpellType() == RE::MagicSystem::SpellType::kDisease) {
				return true;
			}
		}
		return false;
	}

	bool Potion::Poisoned(RE::Actor* a_actor)
	{
		auto* target = a_actor->AsMagicTarget();
		auto* effects = target ? target->GetActiveEffectList() : nullptr;
		if (!effects) {
			return false;
		}
		for (auto* active : *effects) {
			if (!active || !active->spell || active->flags.all(RE::ActiveEffect::Flag::kDispelled)) {
				continue;
			}
			// A poison applied to the player is either a poison-type magic item or a poison the
			// engine applied from an AlchemyItem.
			if (active->spell->GetSpellType() == RE::MagicSystem::SpellType::kPoison) {
				return true;
			}
			const auto* alchemy = active->spell->As<RE::AlchemyItem>();
			if (alchemy && alchemy->IsPoison()) {
				return true;
			}
		}
		return false;
	}

	float Potion::Strength(const RE::AlchemyItem* a_potion, Need a_need)
	{
		float total = 0.0f;
		for (const auto* effect : a_potion->effects) {
			const auto* base = effect ? effect->baseEffect : nullptr;
			if (!base) {
				continue;
			}
			// One harmful effect rules the whole bottle out, however good the rest is.
			if (base->IsHostile() || base->IsDetrimental()) {
				return 0.0f;
			}
			const auto archetype = base->GetArchetype();
			if (a_need == Need::kCureDisease) {
				total += archetype == Archetype::kCureDisease ? 1.0f : 0.0f;
				continue;
			}
			if (a_need == Need::kCurePoison) {
				total += archetype == Archetype::kCurePoison ? 1.0f : 0.0f;
				continue;
			}
			const auto value = ValueOf(a_need);
			if (!RaisesValue(base, value)) {
				continue;
			}
			const float magnitude = std::max(effect->effectItem.magnitude, 0.0f);
			const float duration = static_cast<float>(effect->effectItem.duration);
			// Water breathing is worth its duration; a restore over time is worth magnitude x time.
			if (a_need == Need::kWaterBreathing) {
				total += duration > 0.0f ? duration : 1.0f;
			} else if (archetype == Archetype::kValueModifier && duration <= 0.0f) {
				total += magnitude;
			} else {
				total += magnitude * std::max(duration, 1.0f);
			}
		}
		return total;
	}

	Potion::Pick Potion::Scan(RE::PlayerCharacter* a_player, Need a_need, float a_missing, bool a_strongest) const
	{
		Pick result;
		RE::AlchemyItem* strongest = nullptr;
		float strongestScore = 0.0f;
		RE::AlchemyItem* enough = nullptr;
		float enoughScore = 0.0f;

		const auto inventory = a_player->GetInventory([](RE::TESBoundObject& a_obj) { return a_obj.Is(RE::FormType::AlchemyItem); });
		for (const auto& [object, data] : inventory) {
			auto* potion = object ? object->As<RE::AlchemyItem>() : nullptr;
			if (!potion || data.first <= 0 || !potion->IsMedicine() || potion->IsPoison() || potion->IsFood()) {
				continue;
			}
			if (data.second && data.second->IsQuestObject()) {
				continue;
			}
			const float score = Strength(potion, a_need);
			if (score <= 0.0f) {
				continue;
			}
			++result.candidates;
			if (!strongest || score > strongestScore ||
				(score == strongestScore && potion->GetFormID() < strongest->GetFormID())) {
				strongest = potion;
				strongestScore = score;
			}
			// The weakest bottle that still covers what is missing, so a grand potion is not spent
			// on a scratch.
			if (score >= a_missing && (!enough || score < enoughScore ||
				(score == enoughScore && potion->GetFormID() < enough->GetFormID()))) {
				enough = potion;
				enoughScore = score;
			}
		}
		result.potion = a_strongest ? strongest : (enough ? enough : strongest);
		return result;
	}

	Potion::Need Potion::Current(RE::PlayerCharacter* a_player, float& a_ratio, float& a_missing, bool& a_urgent) const
	{
		a_ratio = 1.0f;
		a_missing = 0.0f;
		a_urgent = false;
		const auto tune = Settings::PotionTune();
		auto* owner = a_player->AsActorValueOwner();

		const auto bar = [a_player, owner](RE::ActorValue a_value, float& a_out) {
			const float max = a_player->GetActorValueMax(a_value);
			const float current = owner->GetActorValue(a_value);
			a_out = max > 0.0f ? std::clamp(current / max, 0.0f, 1.0f) : 1.0f;
			return std::max(max - current, 0.0f);
		};

		if (tune.health) {
			float ratio = 1.0f;
			const float missing = bar(RE::ActorValue::kHealth, ratio);
			if (ratio <= tune.healthThreshold) {
				a_ratio = ratio;
				a_missing = missing;
				a_urgent = ratio <= tune.urgentHealthThreshold;
				return Need::kHealth;
			}
		}
		if (tune.waterBreathing) {
			const auto* state = a_player->AsActorState();
			const bool swimming = state && state->IsSwimming();
			if (swimming && !HasActiveValue(a_player, RE::ActorValue::kWaterBreathing)) {
				const float submerged = a_player->GetSubmergeLevel(a_player->GetPositionZ(), a_player->GetParentCell());
				if (submerged >= kSubmergedEnough) {
					a_ratio = submerged;
					return Need::kWaterBreathing;
				}
			}
		}
		if (tune.stamina) {
			float ratio = 1.0f;
			const float missing = bar(RE::ActorValue::kStamina, ratio);
			if (ratio <= tune.staminaThreshold) {
				a_ratio = ratio;
				a_missing = missing;
				return Need::kStamina;
			}
		}
		if (tune.magicka) {
			float ratio = 1.0f;
			const float missing = bar(RE::ActorValue::kMagicka, ratio);
			if (ratio <= tune.magickaThreshold) {
				a_ratio = ratio;
				a_missing = missing;
				return Need::kMagicka;
			}
		}
		if (tune.curePoison && Poisoned(a_player)) {
			return Need::kCurePoison;
		}
		if (tune.cureDisease && Diseased(a_player)) {
			return Need::kCureDisease;
		}
		return Need::kNone;
	}

	void Potion::Tick()
	{
		if (!checkAfterDrink || Clock::now() < quietUntil) {
			return;
		}
		checkAfterDrink = false;
		auto* player = Util::Player();
		float ratio = 1.0f;
		float missing = 0.0f;
		bool urgent = false;
		const Need now = Current(player, ratio, missing, urgent);
		Log("after drinking for {}: need is now {}", NeedTag(drankFor), NeedTag(now));
		// Drinking that changed nothing means the bottle did not do what its effects said.
		if (now == drankFor && ratio <= ratioBeforeDrink && !warnedNoChange) {
			warnedNoChange = true;
			Log("WARN the {} need did not ease after drinking (was {:.2f}, now {:.2f})", NeedTag(drankFor), ratioBeforeDrink, ratio);
			Util::Notify("CIGAR: 물약을 마신 뒤 변화 없음. 로그 확인");
		}
	}

	void Potion::FastTick()
	{
		auto* player = Util::Player();
		const auto* controls = RE::ControlMap::GetSingleton();
		const bool movable = controls && controls->IsMovementControlsEnabled();
		const bool sexlab = sexlabAnimating && player->IsInFaction(sexlabAnimating);
		const auto now = Clock::now();
		const bool quiet = now < quietUntil;

		float ratio = 1.0f;
		float missing = 0.0f;
		bool urgent = false;
		const Need need = Current(player, ratio, missing, urgent);
		const bool ready = need != Need::kNone && movable && !sexlab && !quiet;

		// The inventory is read when the need changes, when the bottle in hand is gone, and once a
		// second otherwise; the rest of this gate is cheap enough to run every tick.
		std::size_t candidates = 0;
		if (!ready) {
			offeredPotion = nullptr;
		} else if (need != offeredNeed || !offeredPotion || now - scannedAt >= kScanInterval) {
			const auto pick = Scan(player, need, missing, urgent);
			offeredPotion = pick.potion;
			candidates = pick.candidates;
			scannedAt = now;
		}
		const Need was = offeredNeed;
		offeredNeed = ready ? need : Need::kNone;

		LogGate(std::format("need={} ratio={:.2f} urgent={} movable={} sexlab={} quiet={} potions={} pick={}",
			NeedTag(need), ratio, urgent, movable, sexlab, quiet,
			ready ? std::to_string(candidates) : "-"s, offeredPotion ? Util::NameOf(offeredPotion) : "-"s));

		// The prompt names the potion, so a different need or bottle is offered again with its name.
		if (drink.Offered() && (need != was || !offeredPotion)) {
			drink.Withdraw();
			drink.Reset();
		}
		auto* potion = offeredPotion;
		const bool showRatio = need == Need::kHealth || need == Need::kStamina || need == Need::kMagicka;
		const int percent = static_cast<int>(ratio * 100.0f);
		drink.Update(ready && potion != nullptr, [potion, showRatio, percent] {
			return showRatio ? std::format("마시기: {} ({}%)", Util::NameOf(potion), percent)
			                 : std::format("마시기: {}", Util::NameOf(potion));
		});
	}

	void Potion::OnAccepted(std::uint16_t a_eventID)
	{
		if (a_eventID != kDrink) {
			return;
		}
		auto* player = Util::Player();
		float ratio = 1.0f;
		float missing = 0.0f;
		bool urgent = false;
		// Re-check: the inventory or the situation may have changed while the prompt was up.
		const Need need = Current(player, ratio, missing, urgent);
		if (need == Need::kNone) {
			Log("accept ignored: no need holds any more");
			return;
		}
		const auto pick = Scan(player, need, missing, urgent);
		if (!pick.potion) {
			Log("accept ignored: no potion for {} in the inventory", NeedTag(need));
			return;
		}
		RE::ActorEquipManager::GetSingleton()->EquipObject(player, pick.potion);
		quietUntil = Clock::now() + kQuietAfterDrink;
		drankFor = need;
		ratioBeforeDrink = ratio;
		checkAfterDrink = true;
		Log("drank {} ({:08X}, strength {:.0f}) for {} at {:.2f}{}", Util::NameOf(pick.potion),
			pick.potion->GetFormID(), Strength(pick.potion, need), NeedTag(need), ratio, urgent ? " (urgent)" : "");
	}
}
