# 028 · Helmet (off by default, so the face shows)

Status (2026-09-25, second design): rebuilt without Helmet Toggle 2 and deployed. **투구 벗기 with the
take-off clip passed in game (the user, 2026-09-25).** 투구 쓰기 in combat passed too (test run 2).
**The hip display (IED) is dropped** (the user, 2026-09-25, after test run 3): taking the helmet off and
putting it on is enough. The IED entry is removed from the configs (its history is in git, before 2.0.0). The first design (pressing Helmet Toggle's hotkey) passed its five tests but was replaced the
same day; its record is kept below under "First design".

## The user's decision (2026-09-25, second)

- Helmet Toggle 2 has no reason to stay. Take only its motions and IED conditions.
- The helmet is off by default: 투구 벗기 keeps offering itself while headgear is worn, except inside
  dungeons. In combat, with an enemy, 투구 쓰기 comes up.
- Why: players keep looking at their character's face; most have sculpted a handsome or beautiful
  Dragonborn, and anything that hides or disturbs the face and hair is bad UX. So the helmet comes
  off outdoors and in the field as well, and only dungeons are the exception.

## As built (second design)

- `src/Helmet.cpp`, once a second:
  - **투구 벗기 (길게)** while ArmorHelmet or ClothingHead headgear is worn on the head or hair slot
    (circlets excluded), out of combat, movement controls on, and not in a dungeon. A dungeon is an
    interior cell whose location has one of 19 location types (LocTypeDungeon, LocTypeClearable,
    LocTypeDraugrCrypt, Dwemer, Falmer, vampire/warlock/dragon priest lairs, animal dens, bandit and
    Forsworn camps, hagraven nests, werewolf/werebear lairs, spriggan groves, giant camps, dragon
    lairs, Riekling camps, ash spawn; looked up by editor ID at load). Exteriors never count.
  - **투구 쓰기** (one press, the combat exception of the input policy) in combat while no headgear is
    worn and a piece CIGAR took off is still carried.
- Taking off: Helmet Toggle 2's take-off clip (graph variable `iGPMAAnimationType` = 2 for a helmet,
  6 for a ClothingHead hood, then the event `OffsetGPMA`), the headgear really unequipped 0.7 s in
  (`ActorEquipManager::UnequipObject`), `OffsetGPMAStop` 1.85 s later. Seated or with a weapon drawn,
  the clip is skipped and the headgear comes off at once.
- Putting on: every stowed piece still carried is equipped at once, without a clip (it is combat).
- The stowed pieces are kept in the co-save (record `HELM` v1). Wearing headgear again by any route
  clears the list; a stowed piece no longer carried drops out.
- Declining 투구 벗기 hides it until the location changes; declining 투구 쓰기, until combat ends.
- Gate line: `worn= stowed= dungeon= (reason) combat= movable= busy= dismissed=off/on`; each clip logs
  whether the variable was set and the event accepted.

## Assets and modlist (2026-09-25)

- New asset mod `mods\CIGAR - Helmet Motions`: `meshes\OpenAnimationReplacer\CIGAR Helmet\` with
  Helmet Toggle 2's `Helmet Unequip` and `Hood Unequip` OAR submods, byte-identical copies. They are
  keyed only on `iGPMAAnimationType`; the `OffsetGPMA` event and the variable are in the Pandora
  output's `0_Master.hkx` already. The equip clips are not copied (putting on is instant).
- Profile `TKL - MUNG ADDON` (MO2 closed and reopened; backups `*.bak_20260925_cigar-helmet`):
  `Helmet Toggle 2` and `Helmet Toggle 2 - KR` disabled, `Helmet Toggle 2.esp` unticked (no plugin
  has it as a master), `CIGAR - Helmet Motions` enabled above the CIGAR release entry. `Helmet
  Toggle 2 - SMP Hair Fix` stays: it is the winning FSMP `configs.xml` for the whole game.
- `verify_deploy.py` checks that Helmet Toggle 2 is off, the motions mod is on with both clips, and
  `0_Master.hkx` still has the event and the variable.
- The first design's physics-reset key press is gone: the synthetic F12 for Auto Physics Reset also
  opened photo mode (test 2 of the first design, the user). A real unequip should not need a reset.

## Not done: the helmet on the belt (IED)

Helmet Toggle's IED profile does not carry over. It only repositions the node
`ExtraPelvisArmorHelmet1`, which belongs to the *hidden variant mesh* of a helmet that is still
equipped (Dynamic Armor Variants), plus a hand node during its clip. Once the helmet is really
unequipped there is nothing for those entries to move. Showing a carried, unequipped helmet on the
pelvis needs a new IED entry (a last-equipped or inventory display for the head slot) in the user's
IED config, which is stored per save; that is the next step if the user wants the belt display.

## First design (2026-09-25, replaced)

### The user's decision (2026-09-25, first)

A helmet prompt (off when the player enters a safe location, on in an unsafe one) is built on
Helmet Toggle 2 (Nexus 100617, v3.6), which is already
installed: CIGAR presses Helmet Toggle's hotkey instead of the player, and the safe list is Helmet
Toggle's own ("헬멧토글에 기준값이 있음").

## Helmet Toggle 2, as read from its sources (`source/scripts`, shipped with the mod)

- The hotkey is an MCM Helper keybind, `HT_ManageHelmet`, whose action is
  `CallFunction HT_MCM.PressHotkey` on `Helmet Toggle 2.esp|800` (HT_AnimationQuest). The user's
  settings (`mods\TAKEALOOK - MCM and INI\MCM\Settings\Helmet Toggle 2.ini`) have `iEnableHotkey = 1`,
  the simple type: each press flips `HT_HotkeyState` and calls `ManageHelmet(player, state)`.
- With the simple type, `CheckConditions` always returns after `CheckHotkey`, so Helmet Toggle's
  own location automation never runs on this setup; the helmet only changes on the key (and in
  dialogue, if that option is on).
- `HT_HelmetState` (`0x804`): 0 while headgear is on, above 0 while Helmet Toggle has it hidden.
- Safe location (`HT_PlayerAlias.FindLocationKeyword`): any location keyword in `HT_UnsafeLocations`
  makes it unsafe; otherwise any keyword in `HT_SafeLocations` makes it safe; otherwise, with
  `iEnableParentLocation = 1` (the user's), the parent location's keywords against the safe list.
  No location means unsafe. `iEnableInteriors` (interiors only) is 0 here.
- The lists are filled at runtime by its FLM ini (`Helmet Toggle_FLM.ini`): safe = LocTypeInn,
  LocTypeCity, LocTypeDwelling, LocTypeStore, LocTypeTown (plus the plugin's LocTypePlayerHouse and
  `HT_UnequipHeadgearHere`); unsafe = animal dens, bandit/Forsworn/giant camps, clearable, dragon
  and dragon priest lairs, draugr crypts, dungeons, Dwemer ruins, Falmer hives, hagraven nests,
  spriggan groves, vampire/warlock/werewolf/werebear lairs, Riekling camps, ash spawn, and
  `HT_EquipHeadgearHere` (KID puts it on the Ratway).
- Headgear it manages: worn armor in slots 30, 31, 42, 44, 55 with ArmorHelmet, ClothingHead,
  HT_ArmorHood, HT_ArmorMask or HT_ArmorHelmetAlt, not HT_IgnoreHeadgear (wigs, gags, lowered hoods,
  capes; its KID ini) and not ClothingCirclet.

## As built

- `src/Helmet.cpp`, once a second, out of combat with movement controls on:
  - **투구 벗기 (길게)** in a safe location while headgear is on (state 0 and a managed piece worn);
  - **투구 쓰기 (길게)** in an unsafe location while Helmet Toggle has it hidden (state above 0).
- Accepting dispatches `HT_MCM.PressHotkey()` on the quest, exactly what the key did. Three seconds
  later the module checks `HT_HelmetState`. If it did not move the wanted way (the simple hotkey's
  remembered state can disagree with what is worn), it presses once more; if that fails too it logs
  a WARN, notifies once and hides the prompt until the location changes.
- Declining (double tap) hides both prompts until the location changes.
- Only hotkey type 1 is handled; another type logs a WARN and shows nothing. Weather and season
  options are not mirrored (off here; a note is logged when on).
- Gate line: `type= state= worn= loc= safe= (reason) combat= movable= dismissed= pending=`.

## The hotkey taken over

B (48) removed from MCM Helper's `keybinds.json` (`mods\TAKEALOOK - MCM and INI\MCM\Settings`,
backup `keybinds.json.bak_20260925_helmet-key`, CRLF kept, one entry removed). MCM Memory's profile
has no Helmet Toggle row, so nothing restores it. `verify_deploy.py` checks the scripts' names and
that the keybind stays unbound.

Known limit (read from the source, not seen in game): with the key gone, the helmet cannot be put on inside a safe place or taken off in an
unsafe one (equipping a helmet from the inventory is undone by Helmet Toggle's own state).

## Test 1 (2026-09-25): passed

All five steps passed (the user; log 14:17-14:25): 투구 쓰기 in 황량한 폭포 무덤 → `helmet on
(state 0)`, 투구 벗기 in 화이트런 → `helmet off (state 1)`, B does nothing, decline until the next
location, no prompt in combat.

## SMP hair after the toggle (reported 2026-09-25)

The user: after Helmet Toggle takes the helmet off (a Dynamic Armor Variants swap to a hidden
variant, not an unequip; `ApplyHiddenVariant` in `HT_PlayerAlias`), SMP hair has no physics. Taking
the helmet off and on for real makes the hair physics work, even in Helmet Toggle's hidden state.

First fix, built the same day and untested: once the state has changed, CIGAR waits 1.5 s more
(Helmet Toggle's own sequence runs about 2.6 s after the press) and presses Auto Physics Reset's
manual reset key (`uManualResetKey`, 88 = F12, read from its INI at load), i.e. a full SMP reset.
Log: `physics reset: pressed Auto Physics Reset key 88 (sent)`; Auto Physics Reset's own log should
show the reset.

If that is not enough, the alternative the user asked about is a real unequip by CIGAR with Helmet
Toggle's clips (its OAR folders `Helmet Equip`/`Helmet Unequip` etc., driven by the graph variable
`iGPMAAnimationType` and the event `OffsetGPMA`) and its IED placement on the pelvis node
`ExtraPelvisArmorHelmet1`. That replaces Helmet Toggle's logic instead of pressing its key.
