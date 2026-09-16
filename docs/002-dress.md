# 002 · Dress (undress / dress)

## Why

SI's `DressActions` module (Water, Bed and Wardrobe Undress) treats
`HDTSMPObjectBase` (`000805:HDT SMP Object - Simple.esp`) as clothing. The item
is:

- in slot 60, with `ArmorType = Clothing`;
- **NonPlayable**, with the `SexLabNoStrip` and `OStimNoStrip` keywords;
- kept equipped by GT Softbody's `DynamicSmpCollision` quest and its
  `SMPCloakFFSelf` effect.

SI removes it, Softbody re-equips it, and SI's undress prompt never goes away.
Test 3 in `001` covers this. Bed and Wardrobe Undress are assumed to share
SI's strip code; that was not observed.

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

Undress is offered in any context when something strippable is worn. The items
it removes are remembered in the SKSE co-save (record `CIGR`/`DRES`), with
FormIDs re-resolved on load.

Dress is offered once no context applies, CIGAR's remembered items exist, and
nothing strippable is worn. If the player puts clothes on by hand after leaving,
the remembered set is dropped.

## Unverified until played

- Bed detection by `kCanSleep`, including bedrolls and modded beds. Every
  aimed furniture logs `furniture <name> flags=<hex> sleep=<bool>`.
- Wardrobe detection on modded containers such as the Snazzy wardrobes.
- SI's Bed and Wardrobe Undress staying off under the Power User preset.
