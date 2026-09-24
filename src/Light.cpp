#include "Light.h"

#include "Util.h"

namespace CIGAR
{
	namespace
	{
		// SI's ItemUse values: darkness_threshold 14, dont_show_in_combat_makelight on. SI waits
		// time_till_makelight_prompt (5 s); the user wants the prompt as soon as it is dark
		// (2026-09-24), so only a short debounce is left against flicker at the edge of a light.
		constexpr float kDarkLevel = 14.0f;
		constexpr auto kDarkDelay = 300ms;
		// 불 끄기 waits for the light to hold this long above the dark line, so walking past a
		// brazier does not offer it.
		constexpr float kBrightLevel = 30.0f;
		constexpr auto kBrightDelay = 3s;
		constexpr auto kLitCheck = 1500ms;
		// Submerge level (0 dry, 1 fully under) from which the player is in water, not at its edge.
		constexpr float kInWater = 0.5f;

		// The engine's own GetLightLevel condition on the player, so the number means what the
		// game's conditions mean. CommonLib has no direct accessor.
		bool LightBelow(RE::PlayerCharacter* a_player, float a_level)
		{
			RE::TESConditionItem item;
			item.data.object = RE::CONDITIONITEMOBJECT::kSelf;
			item.data.functionData.function = RE::FUNCTION_DATA::FunctionID::kGetLightLevel;
			item.data.flags.opCode = RE::CONDITION_ITEM_DATA::OpCode::kLessThan;
			item.data.comparisonValue.f = a_level;
			RE::ConditionCheckParams params(a_player, nullptr);
			return item.IsTrue(params);
		}
	}

	Light::Light()
	{
		prompt.SetPromptType(SkyPromptAPI::kHold);
		putOut.SetPromptType(SkyPromptAPI::kHold);
	}

	Light* Light::GetSingleton()
	{
		static Light singleton;
		return &singleton;
	}

	void Light::OnGameLoaded()
	{
		prompt.Reset();
		putOut.Reset();
		lastGate.clear();
		darkSince = Clock::now();
		bright = false;
		checkAfterAccept = false;
		heldTorch = nullptr;
		savedLeft = nullptr;
		torchOut = false;
		lastNoSource.clear();
		auto* defaults = RE::BGSDefaultObjectManager::GetSingleton();
		leftSlot = defaults ? defaults->GetObject<RE::BGSEquipSlot>(RE::DEFAULT_OBJECT::kLeftHandEquip) : nullptr;
		Util::WarnIfSIModuleOn("ItemUse.enabled_makelight", "/MCP/modules/ItemUse/enabled_makelight");
		Log("ready: torches from the pack, then known light spells");
	}

	void Light::Tick()
	{
		Util::WarnIfSIModuleOn("ItemUse.enabled_makelight", "/MCP/modules/ItemUse/enabled_makelight");
	}

	bool Light::Dark(RE::PlayerCharacter* a_player) const
	{
		return LightBelow(a_player, kDarkLevel);
	}

	std::string Light::LightBand(RE::PlayerCharacter* a_player) const
	{
		constexpr std::array kBands{ 5.0f, 14.0f, 30.0f, 60.0f, 100.0f };
		for (const float band : kBands) {
			if (LightBelow(a_player, band)) {
				return std::format("<{:.0f}", band);
			}
		}
		return ">=100";
	}

	bool Light::Lit(RE::PlayerCharacter* a_player, std::string& a_how)
	{
		for (const bool left : { true, false }) {
			if (auto* object = a_player->GetEquippedObject(left); object && object->Is(RE::FormType::Light)) {
				a_how = "light in hand";
				return true;
			}
		}
		if (auto* effects = a_player->AsMagicTarget()->GetActiveEffectList()) {
			for (auto* effect : *effects) {
				const auto* base = effect ? effect->GetBaseObject() : nullptr;
				if (!base || effect->flags.any(RE::ActiveEffect::Flag::kInactive, RE::ActiveEffect::Flag::kDispelled)) {
					continue;
				}
				if (base->GetArchetype() == RE::EffectArchetypes::ArchetypeID::kLight) {
					a_how = "light effect";
					return true;
				}
			}
		}
		return false;
	}

	bool Light::InWater(RE::PlayerCharacter* a_player)
	{
		// Water reads dark to GetLightLevel, and a torch cannot burn in it (the user, 2026-09-24).
		const auto* state = a_player->AsActorState();
		return (state && state->IsSwimming()) ||
		       a_player->GetSubmergeLevel(a_player->GetPositionZ(), a_player->GetParentCell()) >= kInWater;
	}

	bool Light::IsLightSpell(const RE::SpellItem* a_spell, bool& a_self)
	{
		a_self = false;
		if (!a_spell || a_spell->GetSpellType() != RE::MagicSystem::SpellType::kSpell) {
			return false;
		}
		for (const auto* effect : a_spell->effects) {
			const auto* base = effect ? effect->baseEffect : nullptr;
			if (base && base->GetArchetype() == RE::EffectArchetypes::ArchetypeID::kLight) {
				a_self = a_spell->GetDelivery() == RE::MagicSystem::Delivery::kSelf;
				return true;
			}
		}
		return false;
	}

	Light::Source Light::FindSource(RE::PlayerCharacter* a_player)
	{
		Source source;
		std::uint32_t bestRadius = 0;
		const auto inventory = a_player->GetInventory([](RE::TESBoundObject& a_obj) { return a_obj.Is(RE::FormType::Light); });
		for (const auto& [object, data] : inventory) {
			auto* torch = object ? object->As<RE::TESObjectLIGH>() : nullptr;
			if (!torch || data.first <= 0 || !torch->CanBeCarried()) {
				continue;
			}
			if (!source.torch || torch->data.radius > bestRadius) {
				source.torch = torch;
				bestRadius = torch->data.radius;
			}
		}
		if (source.torch) {
			return source;
		}

		// Spells the player learned, and the ones the character starts with.
		std::vector<RE::SpellItem*> known;
		for (auto* spell : a_player->GetActorRuntimeData().addedSpells) {
			known.push_back(spell);
		}
		if (auto* base = a_player->GetActorBase(); base && base->actorEffects) {
			for (std::uint32_t i = 0; i < base->actorEffects->numSpells; ++i) {
				known.push_back(base->actorEffects->spells[i]);
			}
		}
		const float magicka = a_player->AsActorValueOwner()->GetActorValue(RE::ActorValue::kMagicka);
		bool bestSelf = false;
		for (auto* spell : known) {
			bool self = false;
			if (!IsLightSpell(spell, self)) {
				continue;
			}
			const float cost = spell->CalculateMagickaCost(a_player);
			if (cost > magicka) {
				++source.unaffordable;
				continue;
			}
			// A light that follows the player (Candlelight) before one that is thrown (Magelight).
			if (!source.spell || (self && !bestSelf) || (self == bestSelf && cost < source.cost)) {
				source.spell = spell;
				source.cost = cost;
				bestSelf = self;
			}
		}
		return source;
	}

	void Light::FastTick()
	{
		auto* player = Util::Player();
		const auto now = Clock::now();
		auto* ui = RE::UI::GetSingleton();
		std::string how;
		const bool dark = Dark(player);
		const bool lit = Lit(player, how);
		const bool combat = player->IsInCombat();
		const bool menu = !ui || ui->IsApplicationMenuOpen() || ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME);
		const bool water = InWater(player);

		// The torch CIGAR handed over is gone from the hand (put away, dropped, swapped): nothing to give back.
		if (torchOut && player->GetEquippedObject(true) != heldTorch && !checkAfterAccept) {
			Log("left hand no longer holds {}; forgetting {}", heldTorch ? Util::NameOf(heldTorch) : "-"s,
				savedLeft ? Util::NameOf(savedLeft) : "empty hand"s);
			torchOut = false;
			heldTorch = nullptr;
			savedLeft = nullptr;
		}

		const bool want = dark && !lit && !combat && !menu && !water && !player->IsDead();
		const auto source = want ? FindSource(player) : Source{};
		const bool has = source.torch || source.spell;
		if (!want || !has) {
			darkSince = now;
		}
		const bool available = want && has && now - darkSince >= kDarkDelay;

		// Bright enough, for a while, with CIGAR's torch still in hand: offer to put it out.
		const bool isBright = !LightBelow(player, kBrightLevel);
		if (!isBright) {
			bright = false;
		} else if (!bright) {
			bright = true;
			brightSince = now;
		}
		const bool canPutOut = torchOut && bright && now - brightSince >= kBrightDelay && !combat && !menu;

		LogGate(std::format("dark={} light{} lit={}{} combat={} menu={} water={} torch={} spell={}{} ready={} torchOut={} putOut={}",
			dark, LightBand(player), lit, lit ? " (" + how + ")" : "", combat, menu, water,
			source.torch ? Util::NameOf(source.torch) : "-"s, source.spell ? Util::NameOf(source.spell) : "-"s,
			source.unaffordable ? std::format(" ({} light spells short of magicka)", source.unaffordable) : ""s,
			available, torchOut, canPutOut));

		// Dark and unlit with nothing to light: said once per situation, so a missing prompt is explained.
		const auto noSource = want && !has ? "dark, but no torch carried and no light spell known or affordable"s : ""s;
		if (noSource != lastNoSource) {
			lastNoSource = noSource;
			if (!noSource.empty()) {
				Log("{}", noSource);
			}
		}

		if (checkAfterAccept && now - acceptedAt >= kLitCheck) {
			checkAfterAccept = false;
			if (lit) {
				Log("lit after the accept ({})", how);
			} else {
				Log("WARN still not lit {}ms after the accept", std::chrono::duration_cast<std::chrono::milliseconds>(kLitCheck).count());
				Util::Notify("CIGAR: 불 밝히기 실패. 로그 확인");
			}
		}

		auto* torch = source.torch;
		auto* spell = source.spell;
		if (prompt.Offered() && !available) {
			prompt.Withdraw();
			prompt.Reset();
		}
		prompt.Update(available, [torch, spell] {
			return std::format("불 밝히기 (길게): {}", Util::NameOf(torch ? static_cast<RE::TESForm*>(torch) : spell));
		});
		putOut.Update(canPutOut, [] { return "불 끄기 (길게)"s; });
	}

	void Light::OnAccepted(std::uint16_t a_eventID)
	{
		auto* player = Util::Player();
		if (a_eventID == kPutOut) {
			PutOut(player);
			return;
		}
		if (a_eventID != kLight) {
			return;
		}
		const auto source = FindSource(player);
		if (source.torch) {
			if (!torchOut) {
				auto* left = player->GetEquippedObject(true);
				auto* right = player->GetEquippedObject(false);
				// A two-handed weapon or a bow fills both hands; the right hand keeps it only if it is
				// one-handed, so what comes back is the left hand as it was.
				savedLeft = left && left != right ? left : nullptr;
			}
			RE::ActorEquipManager::GetSingleton()->EquipObject(player, source.torch, nullptr, 1, leftSlot);
			heldTorch = source.torch;
			torchOut = true;
			Log("took {} ({:08X}) into the left hand; saved {}", Util::NameOf(source.torch), source.torch->GetFormID(),
				savedLeft ? Util::NameOf(savedLeft) : "empty hand"s);
		} else if (source.spell) {
			bool self = false;
			IsLightSpell(source.spell, self);
			auto* caster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
			if (!caster) {
				Log("WARN no instant caster for {}", Util::NameOf(source.spell));
				return;
			}
			caster->CastSpellImmediate(source.spell, false, self ? player : nullptr, 1.0f, false, 0.0f, player);
			player->AsActorValueOwner()->DamageActorValue(RE::ActorValue::kMagicka, source.cost);
			Log("cast {} ({:08X}, {}, cost {:.0f})", Util::NameOf(source.spell), source.spell->GetFormID(), self ? "self" : "aimed", source.cost);
		} else {
			Log("accept ignored: no torch or light spell any more");
			return;
		}
		acceptedAt = Clock::now();
		checkAfterAccept = true;
	}

	void Light::PutOut(RE::PlayerCharacter* a_player)
	{
		if (!torchOut) {
			Log("put out ignored: no torch of CIGAR's in hand");
			return;
		}
		auto* manager = RE::ActorEquipManager::GetSingleton();
		auto* saved = std::exchange(savedLeft, nullptr);
		bool restored = false;
		if (auto* spell = saved ? saved->As<RE::SpellItem>() : nullptr) {
			if (a_player->HasSpell(spell)) {
				manager->EquipSpell(a_player, spell, leftSlot);
				restored = true;
			}
		} else if (auto* item = saved ? saved->As<RE::TESBoundObject>() : nullptr) {
			if (Util::ItemCount(a_player, item) > 0) {
				// A shield uses its own slot; a weapon is sent to the left hand.
				manager->EquipObject(a_player, item, nullptr, 1, item->Is(RE::FormType::Weapon) ? leftSlot : nullptr);
				restored = true;
			}
		}
		if (!restored && heldTorch) {
			manager->UnequipObject(a_player, heldTorch, nullptr, 1, leftSlot);
		}
		Log("put out {}: left hand back to {}", heldTorch ? Util::NameOf(heldTorch) : "-"s,
			restored ? Util::NameOf(saved) : (saved ? Util::NameOf(saved) + " (gone; hand emptied)" : "empty hand"s));
		torchOut = false;
		heldTorch = nullptr;
		putOut.Withdraw();
		putOut.Reset();
	}

	void Light::OnDisabled()
	{
		torchOut = false;
		heldTorch = nullptr;
		savedLeft = nullptr;
		checkAfterAccept = false;
	}
}
