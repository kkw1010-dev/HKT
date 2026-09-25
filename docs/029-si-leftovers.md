# 029 · The rest of SI, plus 독 바르기, in one pass

Status (2026-09-25): built and deployed; not yet tested in game. The user asked to absorb every
remaining Streamlined Interactions feature at once, with poison application added, and to test it
in one run.

What SI still had (all already off in its settings; evidence in `SI/_ABSORPTION/_MAP.md`) and where it
went:

| SI feature | CIGAR | Notes |
|---|---|---|
| Observer (`Observe` scene) | module `Observe` | below |
| Outfit Swap (Piecewise) | module `GearSwap` | below |
| ItemUse spellbook equip (`Equipper::ReadBook`) | `ItemEquip`, books | below |
| Quest note/book equip (4b) | `ItemEquip`, books | quest items only |
| Quest outfit swap (4c, "Party Clothes") | module `PartyOutfit` | MQ201 only |
| — (new, the user's) | module `Poison` | 독 바르기 |
| TidyUp | not built | needs `sweepingOrganizesStuff.esp`, not installed |
| KillMove | not built | excluded earlier by the user; `Execute` covers it |

SI's replaced list (`tools/sync_si_settings.py`, `verify_deploy.py`) now also holds `Observer.enabled`,
`DressActions.enabled_piecewiseoutfitswap` and `ItemUse.enabled_equip_spellbook`. With these, no SI
module does anything on this modlist; `ItemUse.enabled` is the only switch left on and all its
actions are off.

## Poison (독 바르기)

- Gate (1 s): weapon drawn; the right-hand entry is a weapon (not a staff, not fists) without poison;
  a poison is carried; movement controls on; not in a SexLab scene.
- Pick: the most valuable poison carried (`GetGoldValue`).
- Accept (hold): doses = 1 through the `Mod Poison Dose Count` perk entry point (Concentrated Poison),
  `InventoryEntryData::PoisonObject` on the equipped entry, one poison removed. Logs
  `applied ... poisoned after=<bool>` and notifies if the weapon is not poisoned afterwards.

## Observe (살펴보기)

- SI's defaults: idle 5 s, distance 5000, FOV offset 40, 20 degrees a second.
- Gate (100 ms): weapon sheathed, no movement input and out of combat for 5 s, a named crosshair target
  within 5000 units, looking and movement controls on.
- Hold-mode prompt (like 시간 보내기): while held, `PlayerCamera` `worldFOV` (or `firstPersonFOV` in
  first person) drops toward base - 40 (at least 15) at 20/s. Release, movement or combat puts the
  base value back at once.

## GearSwap (부위별 장비 교체)

- Crosshair: a loose armor piece, or an unlocked container or a corpse. Anything whose taking is a
  crime (`IsCrimeToActivate`) is skipped; quest items in containers are skipped.
- Parts: head (30), body (32), hands (33), feet (37). A piece replaces a worn one of the same weight
  class (heavy / light / clothing) with a higher armor rating. Empty parts are left alone (the helmet
  is off by the user's choice, and a player undressed at home should not be offered the chest).
- The best gain wins; the prompt reads `<부위> 장비 교체 (길게): <이름>`. Accepting picks the loose item
  up or moves it out of the container, then equips it; the old piece stays in the inventory.

## ItemEquip books

- A newly acquired spell tome whose spell is not known, or a book that is a quest item, gets the same
  15 s hold prompt as gear, reading `읽기 (길게): <이름>`.
- Spell tome: `TESObjectBOOK::Read`, `AddSpell` if still unknown, the tome removed, "<주문> 습득"
  notification. Quest book: `Read`, then `BookMenu::OpenMenuFromBaseForm` shows the page.
- Other books (loot) are not offered.

## PartyOutfit (파티 의상)

- MQ201 Diplomatic Immunity (`035D5F`), MQ201PartyOutfit (`0E40DF`), MQ201PartyBoots (`0E40DE`).
- 파티 의상 입기 (hold) while objective 40 or 50 is displayed (the party), the clothes are carried and not
  worn, out of combat. It stores every strippable worn piece (co-save record `QOUT`), unequips them,
  and puts on the clothes and boots.
- 원래 장비로 (hold) once neither objective is displayed, while the clothes are worn and a stored piece is
  carried again.

## Helmet on the belt (IED)

`mods\TAKEALOOK - MCM and INI\SKSE\Plugins\IED\DefaultConfigUser.json` gained one custom entry for the
player, `CIGAR - Helmet on Belt` (backup `DefaultConfigUser.json.bak_20260925_cigar-helmet-belt`;
compared after writing: identical apart from the new entry). Decoded against the IED source
(SlavicPotato/ied-dev, `ConfigCustom.h`, `ConfigLastEquipped.h`, `ConfigEquipmentOverrideCondition.h`):

- `cflags` 4112 = kAlwaysUnload | kLastEquippedMode;
- last equipped: biped slots 30 and 31 (`sl` [0, 1]), flags 3 = kPrioritizeRecentBipedSlots |
  kDisableIfSlotOccupied (nothing shows while a helmet is worn), filter = the armor-type condition
  with keyword ArmorHelmet copied from the user's own "Guards Pose - Helmet on Hand" entry;
- node `NPC Pelvis [Pelv]`, position (0, 17, -6), rotation (0, 0, 90 degrees), scale 0.85 - a first
  guess on the right hip, to be tuned in IED's own editor; base flags 4560 (hide lying down, equip
  sound, reference mode, synced transform, drop on death).

IED's default config applies to games without IED data (a new game). Helmet Toggle's own IED entries
in the same file point at its hidden-variant node and are inert with Helmet Toggle off.
