# 018 · Acquired gear equip

Status: built on 2026-09-21. Weapon half confirmed by the log on 2026-09-25 (긴 활 offered, held,
accepted, `equipped 긴 활`); the whole module passed in game on 2026-09-25 (reported by the user; no log sent).

## Contract

When the player acquires a playable weapon or piece of armor, CIGAR offers it for immediate equip
for 15 seconds. This absorbs Streamlined Interactions' `ItemUse.enabled_equip_weapon` and
`ItemUse.enabled_equip_armor` switches. The rest of SI's `ItemUse` module remains enabled.

This is a non-combat action, so accepting it requires a hold. The prompt is suppressed during
combat and while movement controls are unavailable. A newer acquired piece of gear replaces the
current offer.

## Implementation

- `TESContainerChangedEvent` is the acquisition gate: the destination must be the player, the
  source must not already be the player, and the transferred count must be positive.
- The event sink copies only the base-object FormID and marshals work to the game thread. Each
  transfer is preserved so unrelated ammo or ingredient events cannot overwrite a gear pickup;
  among valid gear items, the newest item wins.
- Only playable `WEAP` and `ARMO` records are offered. The item must still be carried and must not
  already be equipped when shown and when accepted.
- The action uses `ActorEquipManager::EquipObject`, the same native equipment path used elsewhere
  in CIGAR.
- The offer is transient, so no co-save state is needed.

## Test

1. Deploy the author build and confirm SI's `enabled_equip_weapon` and `enabled_equip_armor` are
   false while recharge, spellbook and make-light keep their existing values.
2. Out of combat, pick up a playable weapon. Expect `장착하기 (길게): <무기 이름>` for 15 seconds.
3. Hold the prompt key. The weapon should equip and the prompt should disappear.
4. Repeat with armor. The armor should equip.
5. Acquire gear in combat. No prompt should be visible until combat ends; it may appear if the
   15-second offer window has not expired.
6. Drop the offered item or equip it manually. The prompt should disappear without acting.

## Lanterns excluded (2026-09-24)

Items from TorchesCandlelightLanterns plugins are not offered: 불 밝히기 (Light) lights and puts
lanterns away through TCL, and TCL swaps lit and unlit lantern armors in and out of the inventory,
which would otherwise raise an equip prompt each time. The user's request.

## No fishing rods (2026-09-24)

At the user's request, a weapon with `ccBGSSSE001_FishingPoleKW` (every Creation Club fishing rod)
is not offered: the fishing itself equips the rod (Streamlined Fishing, when the supplies are
activated).

> 2026-09-25: on this modlist that is not true. Fish Anywhere With Water's loose
> `ccbgssse001_fishingactscript.pex` overrides Streamlined Fishing's, so the supplies equip no rod;
> see `027-fishing.md`.

## No woodcutter's axes (2026-09-25)

At the user's request, anything in `woodChoppingAxes` (`10ACCC:Skyrim.esm`: the woodcutter's axe,
the Poacher's Axe) is not offered: it is carried for Woodcutting Tweaks' tree harvest and the
chopping block, not wielded. The TCL lantern filter is gone with TCL.

## TCL filter back (2026-09-25)

TCL was restored after CIGAR's `Light` was abandoned, so the TCL lantern filter is back (the
2026-09-24 test of it passed). The woodcutter's axe filter stays.

## SexLab and scripted armor (2026-09-25)

In game a SexLab scene offered "장착하기 (길게): FEEFB813": `SLOVE_Tongue1Armor` (SLOVE.esp), a
nameless slot-44 armor its scripts give the player (not softbody). Fill Her Up's `Inflater - leakOral`
and `leakA` arrived the same way and were skipped only because they were already worn. Now not
offered: a nameless armor or weapon, anything received while the player is in SexLab's animating
faction, and anything defined in `sr_FillHerUp.esp`. Each logs `acquired ... not offered`.
