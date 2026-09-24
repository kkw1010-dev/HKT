# 018 · Acquired gear equip

Status: built on 2026-09-21; awaiting runtime confirmation.

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
