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

`CIGAR_Util.IsStrippable(item, slot)` leaves an item on when any of these hold:

- the slot is 31, 40, 41, 43, 50 or 51 (hair, tail, long hair, ears,
  decapitation);
- the item is non-playable;
- the item has the keyword `SexLabNoStrip`, `OStimNoStrip`, `zad_Lockable` or
  `zad_QuestItem`.

A helmet that also covers slots 31 and 43 is still removed through slot 30.

## Places

The module recognises three contexts:

- **water**: `PO3_SKSEFunctions.IsActorInWater(player)`.
- **bed**: the crosshair lands on furniture whose
  `PO3_SKSEFunctions.GetFurnitureType` is 3 (sleep).
- **wardrobe**: the crosshair lands on a container whose EditorID
  (`GetFormEditorID`, which works because po3 Tweaks has
  `Load EditorIDs = true`) or world model path contains `Wardrobe` or `Dresser`.
  Vanilla examples are `NobleWardrobe01`, `UpperWardrobe01` and
  `CommonWardrobe01` (`Clutter\Common\Wardrobe01.nif`).

A bed or wardrobe context lasts while the player stays in the same cell and
within 250 units of the last one aimed at.

Undress is offered in any context when something strippable is worn. The items
it removes are remembered in the save.

Dress is offered once no context applies, CIGAR's remembered items exist, and
nothing strippable is worn. If the player puts clothes on by hand after leaving,
the remembered set is dropped.

## Unverified until played

- The value `GetFurnitureType` returns for beds, including bedrolls and
  modded beds. Every aimed furniture logs `furniture <name> type=<n>`.
- Wardrobe detection on modded containers such as the Snazzy wardrobes.
- SI's Bed and Wardrobe Undress staying off under the Power User preset.
