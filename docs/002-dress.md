# 002 · Dress (undress / dress)

## Why

An undress that treats every worn armor as clothing never finishes on this modlist:
`HDTSMPObjectBase` (`000805:HDT SMP Object - Simple.esp`) is

- in slot 60, with `ArmorType = Clothing`;
- **NonPlayable**, with the `SexLabNoStrip` and `OStimNoStrip` keywords;
- kept equipped by GT Softbody's `DynamicSmpCollision` quest and its
  `SMPCloakFFSelf` effect.

A strip removes it, Softbody re-equips it, and the undress prompt never goes away.
Test 3 in `001` covers this. So `Dress` strips by the rules below.

## Rules

`Util::IsStrippable(armor, slot)` leaves an item on when any of these hold:

- the slot is 31, 40, 41, 43, 50 or 51 (hair, tail, long hair, ears,
  decapitation);
- the item is non-playable;
- the item has the keyword `SexLabNoStrip`, `OStimNoStrip`, `zad_Lockable` or
  `zad_QuestItem`.

A helmet that also covers slots 31 and 43 is still removed through slot 30.

## Places

The module recognises three contexts:

- **water**: `TESObjectREFR::IsInWater()` (the water surface is above the
  player's position). The Papyrus builds used `PO3_SKSEFunctions.IsActorInWater`.
- **bed**: the crosshair lands on furniture whose active-marker flags include
  `kCanSleep` (`TESFurniture::furnFlags`).
- **wardrobe**: the crosshair lands on a container whose EditorID or world model
  path contains `wardrobe` or `dresser` (case-insensitive). The EditorID is
  available because po3 Tweaks has `Load EditorIDs = true`; the model path works
  without it.
  Vanilla examples are `NobleWardrobe01`, `UpperWardrobe01` and
  `CommonWardrobe01` (`Clutter\Common\Wardrobe01.nif`).

A bed or wardrobe context lasts while the player stays in the same cell and
within 250 units of the last one aimed at.

The prompt follows the player's state, not a list of items CIGAR removed:

| Context | Dressed (something strippable worn) | Naked |
|---|---|---|
| any bed or wardrobe/dresser | 탈의하기 | 착용하기, if the remembered outfit is in the inventory |
| water | 탈의하기 | (bathing, from the `Bathe` module) |
| none | — | 착용하기, only when CIGAR undressed the player and they have not dressed since |

- **Remembered outfit:** the strippable set worn at the last undress, or
  whenever the player stands dressed at a bed, wardrobe or water. Dressing
  re-equips the pieces still in the inventory, so undressing by hand from the
  inventory still gets 착용하기 at a bed or wardrobe.
- **Co-save:** record `CIGR`/`DRES`. Version 2 holds the outfit FormIDs,
  re-resolved on load, plus the undressed-by-CIGAR flag. Version 1 saves (the
  removed-item list) still load.
- **Settle window:** unequips are queued, so the worn state is ignored for 3
  ticks after an undress or dress, and prompts wait out that window too.
- **Re-arming:** every change of context re-arms both prompts.

History:

- **DLL test 1** (2026-09-17): dress appeared only after a later water exit,
  because the first rule offered it only after leaving the place.
- **DLL test 2:** dress appeared on the spot, but not after moving elsewhere.
  The log showed "player dressed without the prompt; forgetting 3 item(s)" in
  the same second as the undress: the queued unequips had not applied yet, so
  the remembered list was wiped. The user then asked for the state-based rule
  above, which also removes that race.
- The water flow and bed detection (`flags=88000003 sleep=true`; chairs and
  benches `sleep=false`) worked throughout.
- **DLL test 3** (state-based rule): confirmed. Dress appeared at another
  wardrobe or bed after an undress elsewhere, and after an undress done by
  hand in the inventory.

## Unverified until played

- Bed detection on bedrolls and modded beds (vanilla bed confirmed). Every
  aimed furniture logs `furniture <name> flags=<hex> sleep=<bool>`.
- Wardrobe detection on modded containers such as the Snazzy wardrobes.
