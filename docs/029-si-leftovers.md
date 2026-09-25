# 029 · The rest of SI, plus 독 바르기, in one pass

Status (2026-09-25): built and deployed. The user asked to absorb every remaining Streamlined
Interactions feature at once, with poison application added, and to test it in one run.

## Test run 1 (2026-09-25, the user's report; crash log `crash-2026-09-25-16-10-16.log`)

| # | Test | Result |
|---|---|---|
| 1 | 읽기 on a spell tome (염력) | passed |
| 2 | 투구 벗기 + helmet on the hip | the take-off passed; **the hip display did not appear** (IED, below) |
| 3 | 몸 장비 교체 on a loose cuirass | passed; the user then set the enchantment rule (GearSwap, below) |
| 4 | 살펴보기 | worked; the user asked for a smoother zoom and a new name, and it showed on furniture (Observe, below) |
| 5 | 독 바르기 | **crashed to desktop on accept** (Poison, below) |
| 6-9 | helmet in combat, 유술, PartyOutfit, quest letter | not reached (the crash ended the run) |

Fixes built and deployed the same day (untested in game):

- **Poison CTD.** `HandleEntryPoint(kModPoisonDoseCount, player, &doses)` passed no filter forms. That
  entry point has three condition tabs (every PERK carrying it says `PerkConditionTabCount = 3`: perk
  owner, weapon, poison), so the engine read `&doses` as the weapon and the next stack word as the
  poison and the result pointer; the crash is inside PerkEntryPointExtender's condition hook, one frame
  above `Poison::OnAccepted`. Now called with the weapon and the poison. `Recharge` already passed its
  filter forms.
- **IED hip display.** Not a fault of the belt entry: IED rejected the whole of `DefaultConfigUser.json`
  (`LoadConfigStore ... parse failed`), and the user's exports `KKW1.json` / `KKW2.json` the same way,
  because Helmet Toggle's four custom entries (`Helmet Toggle - DAV Config / Helmets / Masks / Masks
  Belt`) name forms in `Helmet Toggle 2.esp`, which is disabled. Their IED log lines were the only
  sign. The four entries were removed from all three files (backups `*.bak_20260925_drop-helmettoggle`)
  and `CIGAR - Helmet on Belt` was added to both exports. `verify_deploy.py` now fails when an IED
  config names a plugin that is not loaded or the belt entry is missing. The note below that called
  the Helmet Toggle entries "inert" was wrong.
- **GearSwap.** The user's rule: a worn enchanted piece is never offered a replacement; otherwise only
  the armor rating decides. The rating is now the one the inventory shows (`GetArmorValue`, tempering
  and perks included) instead of the base record's.
- **Observe → 주시하기.** Renamed (살펴보기 read like turning a 3D model around). The zoom is eased on a
  smootherstep curve every frame (2 s in, 0.7 s out) instead of stepping on the 100 ms tick, the way
  back is eased too, and furniture is never a target.

## Test run 2 (2026-09-25 16:34-17:05, the user's report and `CIGAR.log`)

| # | Test | Result |
|---|---|---|
| 5 | 독 바르기 (after the CTD fix) | passed |
| 6 | 투구 쓰기 in combat | passed |
| 7 | 유술 with Cinematic Clash off | 20 of 33 (`docs/012`, test 21) |
| 8 | PartyOutfit (on a test save) | passed |
| 9 | Faendal's letter | the 읽기 prompt showed, but an SI prompt overlapped it and the press went to SI |

After run 2 (the user): **SI is disabled** in the profile (`-[NoDelete] 0008 StreamlinedInteractions`;
MO2 closed and relaunched for the edit, backup `modlist.txt.bak_20260925_disable-si`), since CIGAR
absorbs all of it; `verify_deploy.py` now fails while it is enabled. **Books are their own module,
`BookRead`** (책 읽기, PromptID 39), split out of `ItemEquip`, which now handles weapons and armor only.
Both untested in game.

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

## Observe (주시하기, first built as 살펴보기)

- SI's defaults: idle 5 s, distance 5000, FOV offset 40.
- Gate (100 ms): weapon sheathed, no movement input and out of combat for 5 s, looking and movement
  controls on, no zoom still easing out, and something to watch. Since 2026-09-25 (the user: it is for
  observation and scouting) that is a named actor under the crosshair within 5000 units, or **scenery**:
  nothing under the crosshair while not looking at the floor (pitch below Rest's 0.6 rad, so 앉기 and
  주시하기 never share a moment). Objects and furniture under the crosshair are not watched. Scenery
  reads `주시하기 (누르고 있기)` with no name.
- Hold-mode prompt (like 시간 보내기): while held, `PlayerCamera` `worldFOV` (or `firstPersonFOV` in
  first person) glides to base - 40 (at least 15) over 2 s on a smootherstep curve. Release, movement or
  combat glides it back over 0.7 s. A small thread posts one task a frame while an ease runs.

## GearSwap (부위별 장비 교체)

- Crosshair: a loose armor piece, or an unlocked container or a corpse. Anything whose taking is a
  crime (`IsCrimeToActivate`) is skipped; quest items in containers are skipped.
- Parts: head (30), body (32), hands (33), feet (37). A piece replaces a worn one of the same weight
  class (heavy / light / clothing) with a higher armor rating. Empty parts are left alone (the helmet
  is off by the user's choice, and a player undressed at home should not be offered the chest).
- A worn enchanted piece (base or player enchantment, `InventoryEntryData::IsEnchanted`) blocks its
  part: nothing is offered for it (the user, 2026-09-25). Ratings are `PlayerCharacter::GetArmorValue`
  on the inventory entry, so tempering and perks count; a loose piece in the world is rated untempered.
- The best gain wins; the prompt reads `<부위> 장비 교체 (길게): <이름>`. Accepting picks the loose item
  up or moves it out of the container, then equips it; the old piece stays in the inventory.

## BookRead (책 읽기; books in ItemEquip until 2026-09-25)

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

IED's default config applies to games without IED data (a new game). The claim first written here, that
Helmet Toggle's own entries were inert with Helmet Toggle off, was wrong: they made IED reject the whole
file. They are removed; see test run 1 above.
