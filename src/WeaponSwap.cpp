#include "WeaponSwap.h"

#include "Settings.h"
#include "TDM/TrueDirectionalMovementAPI.h"
#include "Util.h"

namespace CIGAR
{
	namespace
	{
		constexpr auto kSexLabPlugin = "SexLab.esm"sv;
		constexpr RE::FormID kSexLabAnimatingID = 0xE50F;

		// Hostiles farther than this are not considered the enemy (the panel's range tops out below it).
		constexpr float kSearchRadius = 4096.0f;
		// Inside this band below the switch distance the previous zone holds, so an enemy standing at
		// the line does not swap the prompts back and forth.
		constexpr float kHysteresis = 100.0f;
		// Equipping is queued; give it time before the gate is read again and the result is checked.
		constexpr auto kQuietAfterEquip = 1500ms;
		constexpr auto kScanInterval = 1s;

		const char* ZoneName(int a_zone)
		{
			constexpr std::array names{ "-", "near", "far", "flee" };
			return names[static_cast<std::size_t>(a_zone)];
		}

		const char* HandsName(int a_hands)
		{
			constexpr std::array names{ "empty", "melee", "ranged", "other" };
			return names[static_cast<std::size_t>(a_hands)];
		}

		// The instance the equip should take: the favourited one, else the best tempered one.
		RE::ExtraDataList* ExtraFor(const RE::InventoryEntryData* a_entry, bool a_favorite)
		{
			if (!a_entry || !a_entry->extraLists) {
				return nullptr;
			}
			RE::ExtraDataList* best = nullptr;
			float bestHealth = 0.0f;
			for (auto* list : *a_entry->extraLists) {
				if (!list) {
					continue;
				}
				if (a_favorite) {
					if (list->HasType<RE::ExtraHotkey>()) {
						return list;
					}
					continue;
				}
				const auto* health = list->GetByType<RE::ExtraHealth>();
				if (health && health->health > bestHealth) {
					best = list;
					bestHealth = health->health;
				}
			}
			return best;
		}
	}

	WeaponSwap* WeaponSwap::GetSingleton()
	{
		static WeaponSwap singleton;
		return &singleton;
	}

	void WeaponSwap::OnGameLoaded()
	{
		ranged.Reset();
		melee.Reset();
		lastGate.clear();
		lastZone = Zone::kNone;
		rangedPick = {};
		meleePick = {};
		offeredRanged = nullptr;
		offeredMelee = nullptr;
		nextScan = {};
		quietUntil = {};
		savedMelee = nullptr;
		savedLeft = nullptr;
		savedBow = nullptr;
		savedAmmo = nullptr;
		expected = nullptr;
		checkPending = false;

		if (!tdm) {
			tdm = TDM_API::RequestPluginAPI();
		}
		auto* handler = RE::TESDataHandler::GetSingleton();
		sexlabAnimating = handler && handler->LookupModByName(kSexLabPlugin) ?
		                      handler->LookupForm<RE::TESFaction>(kSexLabAnimatingID, kSexLabPlugin) :
		                      nullptr;
		auto* defaults = RE::BGSDefaultObjectManager::GetSingleton();
		rightSlot = defaults ? defaults->GetObject<RE::BGSEquipSlot>(RE::DEFAULT_OBJECT::kRightHandEquip) : nullptr;
		leftSlot = defaults ? defaults->GetObject<RE::BGSEquipSlot>(RE::DEFAULT_OBJECT::kLeftHandEquip) : nullptr;
		Log("tdm={} sexlab={} rightSlot={} leftSlot={} range={:.0f}", tdm != nullptr, sexlabAnimating != nullptr,
			rightSlot != nullptr, leftSlot != nullptr, Settings::WeaponSwapRange());
		if (!rightSlot || !leftSlot) {
			Log("WARN hand equip slots did not resolve; one-handed weapons use their default hand");
		}
		Log("ready");
	}

	RE::NiPointer<RE::Actor> WeaponSwap::FindTarget(RE::PlayerCharacter* a_player, bool& a_locked) const
	{
		a_locked = false;
		if (tdm && tdm->GetTargetLockState()) {
			auto target = tdm->GetCurrentTarget().get();
			if (target && !target->IsDead()) {
				a_locked = true;
				return target;
			}
		}
		for (auto* actor : Util::NearbyHostiles(a_player, kSearchRadius)) {
			if (actor->IsInCombat()) {
				return RE::NiPointer<RE::Actor>(actor);
			}
		}
		return {};
	}

	bool WeaponSwap::Fleeing(RE::Actor* a_actor)
	{
		const auto* controller = a_actor ? a_actor->GetActorRuntimeData().combatController : nullptr;
		return controller && controller->state && controller->state->isFleeing;
	}

	bool WeaponSwap::IsRanged(const RE::TESObjectWEAP* a_weapon)
	{
		return a_weapon && (a_weapon->IsBow() || a_weapon->IsCrossbow());
	}

	bool WeaponSwap::IsTwoHanded(const RE::TESObjectWEAP* a_weapon)
	{
		return a_weapon && (a_weapon->IsTwoHandedSword() || a_weapon->IsTwoHandedAxe());
	}

	WeaponSwap::Hands WeaponSwap::Wielded(RE::PlayerCharacter* a_player)
	{
		auto* right = a_player->GetEquippedObject(false);
		if (!right) {
			return Hands::kEmpty;
		}
		const auto* weapon = right->As<RE::TESObjectWEAP>();
		if (!weapon) {
			return Hands::kOther;  // a spell
		}
		if (IsRanged(weapon)) {
			return Hands::kRanged;
		}
		if (weapon->IsHandToHandMelee()) {
			return Hands::kEmpty;
		}
		return weapon->IsMelee() ? Hands::kMelee : Hands::kOther;  // other: a staff
	}

	RE::TESAmmo* WeaponSwap::PickAmmo(RE::PlayerCharacter* a_player, bool a_bolt)
	{
		// Keep the ammunition the player chose when it fits; otherwise the strongest that fits.
		if (auto* current = a_player->GetCurrentAmmo(); current && current->IsBolt() == a_bolt && Util::ItemCount(a_player, current) > 0) {
			return current;
		}
		RE::TESAmmo* best = nullptr;
		float bestDamage = -1.0f;
		const auto inventory = a_player->GetInventory([](RE::TESBoundObject& a_obj) { return a_obj.Is(RE::FormType::Ammo); });
		for (const auto& [object, data] : inventory) {
			auto* ammo = object ? object->As<RE::TESAmmo>() : nullptr;
			if (!ammo || data.first <= 0 || !ammo->GetPlayable() || ammo->IsBolt() != a_bolt) {
				continue;
			}
			const float damage = ammo->GetRuntimeData().data.damage;
			if (!best || damage > bestDamage || (damage == bestDamage && ammo->GetFormID() < best->GetFormID())) {
				best = ammo;
				bestDamage = damage;
			}
		}
		return best;
	}

	WeaponSwap::Pick WeaponSwap::PickPrevious(RE::PlayerCharacter* a_player, bool a_ranged) const
	{
		auto* weapon = a_ranged ? savedBow : savedMelee;
		if (!weapon) {
			return {};
		}
		auto inventory = a_player->GetInventory([weapon](RE::TESBoundObject& a_obj) { return &a_obj == weapon; });
		const auto it = inventory.find(weapon);
		auto* entry = it != inventory.end() ? it->second.second.get() : nullptr;
		if (!entry || it->second.first <= 0) {
			return {};
		}
		RE::TESAmmo* ammo = nullptr;
		if (a_ranged) {
			const bool bolt = weapon->IsCrossbow();
			ammo = savedAmmo && savedAmmo->IsBolt() == bolt && Util::ItemCount(a_player, savedAmmo) > 0 ? savedAmmo : PickAmmo(a_player, bolt);
			if (!ammo) {
				return {};
			}
		}
		const bool favorite = entry->IsFavorited();
		return { weapon, ExtraFor(entry, favorite), ammo, favorite, a_player->GetDamage(entry), true };
	}

	WeaponSwap::Pick WeaponSwap::PickWeapon(RE::PlayerCharacter* a_player, bool a_ranged) const
	{
		// The user's rule: switching back returns to the loadout held before, not to the strongest weapon.
		if (auto previous = PickPrevious(a_player, a_ranged); previous.weapon) {
			return previous;
		}
		RE::TESAmmo* arrows = nullptr;
		RE::TESAmmo* bolts = nullptr;
		if (a_ranged) {
			arrows = PickAmmo(a_player, false);
			bolts = PickAmmo(a_player, true);
		}
		Pick best;
		auto inventory = a_player->GetInventory([](RE::TESBoundObject& a_obj) { return a_obj.Is(RE::FormType::Weapon); });
		for (auto& [object, data] : inventory) {
			auto* weapon = object ? object->As<RE::TESObjectWEAP>() : nullptr;
			auto* entry = data.second.get();
			if (!weapon || !entry || data.first <= 0 || !weapon->GetPlayable() || weapon->IsBound()) {
				continue;
			}
			RE::TESAmmo* ammo = nullptr;
			if (a_ranged) {
				if (!IsRanged(weapon)) {
					continue;
				}
				// A bow without arrows or a crossbow without bolts cannot shoot.
				ammo = weapon->IsCrossbow() ? bolts : arrows;
				if (!ammo) {
					continue;
				}
			} else if (!weapon->IsMelee() || weapon->IsHandToHandMelee()) {
				continue;
			}
			// The user's order: favourites first, then the strongest by the inventory's damage figure.
			const bool favorite = entry->IsFavorited();
			const float damage = a_player->GetDamage(entry);
			const bool better = !best.weapon || (favorite && !best.favorite) ||
			                    (favorite == best.favorite &&
			                        (damage > best.damage || (damage == best.damage && weapon->GetFormID() < best.weapon->GetFormID())));
			if (better) {
				best = { weapon, ExtraFor(entry, favorite), ammo, favorite, damage };
			}
		}
		return best;
	}

	WeaponSwap::State WeaponSwap::Read(RE::PlayerCharacter* a_player, RE::NiPointer<RE::Actor>& a_hold)
	{
		State s;
		s.combat = a_player->IsInCombat();
		const auto* controls = RE::ControlMap::GetSingleton();
		s.movable = controls && controls->IsMovementControlsEnabled();
		s.sexlab = sexlabAnimating && a_player->IsInFaction(sexlabAnimating);
		s.quiet = Clock::now() < quietUntil;
		s.hands = Wielded(a_player);
		if (s.combat) {
			a_hold = FindTarget(a_player, s.locked);
			s.target = a_hold.get();
		}
		if (!s.target) {
			return s;
		}
		s.distance = a_player->GetPosition().GetDistance(s.target->GetPosition());
		const float range = Settings::WeaponSwapRange();
		if (Fleeing(s.target)) {
			s.zone = Zone::kFleeing;
		} else if (s.distance >= range) {
			s.zone = Zone::kFar;
		} else if (s.distance < range - kHysteresis) {
			s.zone = Zone::kNear;
		} else {
			// Inside the band: keep the side the enemy came from.
			s.zone = lastZone == Zone::kNear ? Zone::kNear : Zone::kFar;
		}
		return s;
	}

	void WeaponSwap::RefreshPicks(RE::PlayerCharacter* a_player, bool a_force)
	{
		const auto now = Clock::now();
		if (!a_force && now < nextScan) {
			return;
		}
		nextScan = now + kScanInterval;
		rangedPick = PickWeapon(a_player, true);
		meleePick = PickWeapon(a_player, false);
	}

	void WeaponSwap::FastTick()
	{
		auto* player = Util::Player();
		if (checkPending && Clock::now() >= quietUntil) {
			CheckResult(player);
		}
		RE::NiPointer<RE::Actor> hold;
		const auto s = Read(player, hold);
		if (s.zone != lastZone) {
			if (s.zone != Zone::kNone) {
				Log("zone {} at distance {:.0f} (switch at {:.0f}, target {}, locked={})", ZoneName(static_cast<int>(s.zone)),
					s.distance, Settings::WeaponSwapRange(), Util::NameOf(s.target), s.locked);
			}
			lastZone = s.zone;
		}

		const bool ready = s.combat && s.movable && !s.sexlab && !s.quiet && s.target;
		const bool distant = s.zone == Zone::kFar || s.zone == Zone::kFleeing;
		const bool wantRanged = ready && distant && (s.hands == Hands::kMelee || s.hands == Hands::kEmpty);
		const bool wantMelee = ready && s.zone == Zone::kNear && s.hands == Hands::kRanged;
		if (wantRanged || wantMelee) {
			RefreshPicks(player, false);
		}
		const auto pickName = [](bool a_want, const Pick& a_pick) {
			if (!a_want) {
				return "-"s;
			}
			return a_pick.weapon ? Util::NameOf(a_pick.weapon) + (a_pick.previous ? "(prev)" : a_pick.favorite ? "(fav)" : "") : "none"s;
		};
		// Distance changes every tick, so the gate carries the zone; the zone line above has the distance.
		LogGate(std::format("combat={} target={} locked={} zone={} hands={} movable={} sexlab={} quiet={} ranged={} melee={}",
			s.combat, s.target ? Util::NameOf(s.target) : "-"s, s.locked, ZoneName(static_cast<int>(s.zone)),
			HandsName(static_cast<int>(s.hands)), s.movable, s.sexlab, s.quiet,
			pickName(wantRanged, rangedPick), pickName(wantMelee, meleePick)));

		const auto update = [](PromptSlot& a_slot, bool a_want, const Pick& a_pick, RE::TESObjectWEAP*& a_offered, std::string_view a_label) {
			// The prompt names the weapon; when the pick changes, offer it again with the new name.
			if (a_slot.Offered() && a_pick.weapon != a_offered) {
				a_slot.Withdraw();
				a_slot.Reset();
			}
			const bool live = a_want && a_pick.weapon;
			a_offered = live ? a_pick.weapon : nullptr;
			a_slot.Update(live, [&] { return std::format("{}: {}", a_label, Util::NameOf(a_pick.weapon)); });
		};
		update(ranged, wantRanged, rangedPick, offeredRanged, "원거리 무기");
		update(melee, wantMelee, meleePick, offeredMelee, "근접 무기");
	}

	void WeaponSwap::RestoreLeft(RE::PlayerCharacter* a_player)
	{
		auto* left = std::exchange(savedLeft, nullptr);
		if (!left) {
			return;
		}
		auto* manager = RE::ActorEquipManager::GetSingleton();
		if (auto* spell = left->As<RE::SpellItem>()) {
			if (a_player->HasSpell(spell)) {
				manager->EquipSpell(a_player, spell, leftSlot);
				Log("left hand back: {}", Util::NameOf(spell));
			}
			return;
		}
		auto* item = left->As<RE::TESBoundObject>();
		if (!item) {
			return;
		}
		const auto count = Util::ItemCount(a_player, item);
		auto* weapon = item->As<RE::TESObjectWEAP>();
		// The same one-handed weapon in both hands needs two of it.
		const auto needed = weapon && weapon == meleePick.weapon ? 2 : 1;
		if (count < needed) {
			Log("left hand not restored: {} carried {} (needs {})", Util::NameOf(item), count, needed);
			return;
		}
		// A shield uses its own slot; a weapon is sent to the left hand.
		manager->EquipObject(a_player, item, nullptr, 1, weapon ? leftSlot : nullptr);
		Log("left hand back: {}", Util::NameOf(item));
	}

	void WeaponSwap::OnAccepted(std::uint16_t a_eventID)
	{
		if (a_eventID != kRanged && a_eventID != kMelee) {
			return;
		}
		auto* player = Util::Player();
		RE::NiPointer<RE::Actor> hold;
		const auto s = Read(player, hold);
		const bool isRanged = a_eventID == kRanged;
		// Re-check: the fight or the hands may have changed while the prompt was up.
		const bool ready = s.combat && s.movable && !s.sexlab && s.target;
		const bool live = isRanged ?
		                      ready && (s.zone == Zone::kFar || s.zone == Zone::kFleeing) && (s.hands == Hands::kMelee || s.hands == Hands::kEmpty) :
		                      ready && s.zone == Zone::kNear && s.hands == Hands::kRanged;
		RefreshPicks(player, true);
		const auto& pick = isRanged ? rangedPick : meleePick;
		if (!live || !pick.weapon) {
			Log("accept ignored, no longer live: zone={} hands={} pick={}", ZoneName(static_cast<int>(s.zone)),
				HandsName(static_cast<int>(s.hands)), pick.weapon ? Util::NameOf(pick.weapon) : "none"s);
			return;
		}

		auto* manager = RE::ActorEquipManager::GetSingleton();
		if (isRanged) {
			auto* right = player->GetEquippedObject(false);
			auto* left = player->GetEquippedObject(true);
			savedMelee = s.hands == Hands::kMelee && right ? right->As<RE::TESObjectWEAP>() : nullptr;
			savedLeft = left && left != right ? left : nullptr;
			manager->EquipObject(player, pick.weapon, pick.extra);
			const bool ammoChanged = pick.ammo && player->GetCurrentAmmo() != pick.ammo;
			if (ammoChanged) {
				const auto count = std::max(1, Util::ItemCount(player, pick.ammo));
				manager->EquipObject(player, pick.ammo, nullptr, static_cast<std::uint32_t>(count));
			}
			Log("equip ranged {} ({:08X}, damage {:.0f}, favorite={}, previous={}) ammo {}{}; saved melee {} left {}", Util::NameOf(pick.weapon),
				pick.weapon->GetFormID(), pick.damage, pick.favorite, pick.previous, pick.ammo ? Util::NameOf(pick.ammo) : "-"s,
				ammoChanged ? " (equipped)" : " (already)", savedMelee ? Util::NameOf(savedMelee) : "-"s, savedLeft ? Util::NameOf(savedLeft) : "-"s);
		} else {
			auto* right = player->GetEquippedObject(false);
			savedBow = right ? right->As<RE::TESObjectWEAP>() : nullptr;
			savedAmmo = player->GetCurrentAmmo();
			const bool twoHanded = IsTwoHanded(pick.weapon);
			manager->EquipObject(player, pick.weapon, pick.extra, 1, twoHanded ? nullptr : rightSlot);
			Log("equip melee {} ({:08X}, damage {:.0f}, favorite={}, previous={}, twoHanded={}); saved bow {} ammo {}", Util::NameOf(pick.weapon),
				pick.weapon->GetFormID(), pick.damage, pick.favorite, pick.previous, twoHanded,
				savedBow ? Util::NameOf(savedBow) : "-"s, savedAmmo ? Util::NameOf(savedAmmo) : "-"s);
			if (twoHanded) {
				savedLeft = nullptr;
			} else {
				RestoreLeft(player);
			}
		}
		expected = pick.weapon;
		expectedAmmo = isRanged ? pick.ammo : nullptr;
		checkPending = true;
		quietUntil = Clock::now() + kQuietAfterEquip;
	}

	void WeaponSwap::CheckResult(RE::PlayerCharacter* a_player)
	{
		checkPending = false;
		auto* right = a_player->GetEquippedObject(false);
		auto* left = a_player->GetEquippedObject(true);
		auto* ammo = a_player->GetCurrentAmmo();
		if (right == expected) {
			Log("after equip: right {} left {} ammo {}", Util::NameOf(right), left ? Util::NameOf(left) : "-"s,
				ammo ? Util::NameOf(ammo) : "-"s);
			// Seen in game (2026-09-19): the ammo queued right after the bow was not equipped. Equip it
			// again now that the bow is in hand, and say so.
			if (expectedAmmo && ammo != expectedAmmo && Util::ItemCount(a_player, expectedAmmo) > 0) {
				const auto count = std::max(1, Util::ItemCount(a_player, expectedAmmo));
				RE::ActorEquipManager::GetSingleton()->EquipObject(a_player, expectedAmmo, nullptr, static_cast<std::uint32_t>(count));
				Log("ammo was not equipped with the bow; equipping {} x{} again", Util::NameOf(expectedAmmo), count);
			}
			return;
		}
		Log("WARN after equip: right hand is {}, expected {}", right ? Util::NameOf(right) : "-"s, Util::NameOf(expected));
		if (!warnedEquip) {
			warnedEquip = true;
			Util::Notify("CIGAR: 무기 전환 실패. 로그 확인");
		}
	}
}
