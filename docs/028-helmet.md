# 028 · Helmet (Helmet Toggle 2's hotkey as a prompt)

Status (2026-09-25): built as module `Helmet` and deployed; not yet tested in game.

## The user's decision (2026-09-25)

SI's HelmetToggle ("prompt to toggle the helmet off/on when the player enters a safe/unsafe
location", SI's own help text) is rebuilt on Helmet Toggle 2 (Nexus 100617, v3.6), which is already
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
that the keybind stays unbound. SI's `HelmetToggle.enabled` is in the replaced list (it was already
off).

Known limit (read from the source, not seen in game): with the key gone, the helmet cannot be put on inside a safe place or taken off in an
unsafe one (equipping a helmet from the inventory is undone by Helmet Toggle's own state).
