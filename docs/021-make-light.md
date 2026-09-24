# 021 · Make light (plan)

Status (2026-09-24): researched, not built. The user picked it as the next SI absorption and
chose prompt-only for TCL's hotkey.

## What SI does (settings and DLL strings)

- ItemUse `enabled_makelight` (on), `time_till_makelight_prompt` 5 s, `darkness_threshold` 14,
  `dont_show_in_combat_makelight` (on). MCM text: "If enabled, the mod will ask to use a torch or
  candlelight spell/scroll when the player is in a dark area." and "The darkness threshold is the
  level of darkness at which the mod will prompt".
- Absorbing it (with recharge, the other ItemUse action still on) would let SI's ItemUse module be
  switched off entirely.

## Torches Candlelight and Lanterns (TCL) on this modlist

An earlier agent's analysis, `TCL_LIGHT_HOOK_ANALYSIS.md` (untracked, 2026-09-21), recommends
pressing TCL's own hotkey rather than re-implementing light. Checked on 2026-09-24:

- `[NoDelete] 0184 Main TorchesCandlelightLanterns` and `0185 ... SMP ... ESP` are enabled;
  `TorchesCandlelightLanterns.esp` is in plugins.txt.
- Hotkey: `MCM/Settings/TorchesCandlelightLanterns.ini` `[Controls] iTCLHotkey = 259` (mouse
  button 4) overrides `MCM/Config/TorchesCandlelightLanterns/settings.ini` `iTCLHotkey=47` (V).
  `Util::PressKey` covers mouse codes 256-263, and `TDMLock` already reads a mod's INI key with
  `Util::IniInt` and presses it; the same pattern applies.
- By that analysis TCL's `PlayerLanternToggle()` falls back from lantern to torch to the
  Candlelight spell, handles oil, sneaking (drops the lantern) and restores weapons. CIGAR should
  not duplicate any of that. Its script claims are the earlier agent's, not re-read here.

## Plan

- Module `Light` (or part of an item-use module): after 5 s at or below light level 14, out of
  combat, not already lit, offer 불 밝히기 (hold, a non-combat action). Accepting presses TCL's
  hotkey. Without TCL or its key, the module stays idle and logs why.
- Darkness: CommonLib has no light-level accessor. Evaluate the engine's own condition function
  `GetLightLevel` (`FUNCTION_DATA::FunctionID::kGetLightLevel`) through a `TESConditionItem` on the
  player, so the value means what the game's conditions mean. Log the level (by stepping a few
  thresholds) when the prompt state changes.
- Already lit: a torch or other light in either hand, a Candlelight-type active effect (light
  archetype; TCL also tags its candlelight spells with keyword `i329IsCandlelightSpell`
  00931), or a lit TCL lantern worn.
- TCL lanterns (read from `TorchesCandlelightLanterns.esp`, 2026-09-24): each lantern is two
  armors in slot 55, lit (`i329DarkElfLantern` 0807 ...) and unlit (`...Off` 090C ...), with no
  keyword telling them apart (only `MagicDisallowEnchanting`, plus `i329LanternUsesOil` on some
  lit ones). The lit sets are FormLists: `i329ListHand` 0865 (SMP variants `i329ListHandSMP` 086D,
  `i329ListHandSMPRotate` 086E, winners in `TorchesCandlelightLanterns SMP.esp`) and
  `i329ListHip` 086A; `i329ListHipOff` 08EA holds the unlit hip ones. A worn slot-55 armor in a
  lit list means lit. Global `i329IsLanternHandOn` 086F is TCL's own hand-lantern flag, a
  cross-check for the log.
- TCL has no player auto-light option (its MCM: hotkey, toggle item, hand/hip, drop, fuel, SMP,
  hide unlit). Globals `i329LightLevelOn` 40 / `i329LightLevelOff` 60 look like NPC lantern
  thresholds; not used.
- Prompt-only (the user's choice, 2026-09-24): TCL joins `Settings::PromptOnly` like Grapple,
  Surrender and Valhalla. Its key (`iTCLHotkey`, now 259 = mouse 4 in
  `MCM/Settings/TorchesCandlelightLanterns.ini`) is moved to an unused key so only CIGAR's prompt
  lights, and switching prompt-only off in the panel restores it. The accept presses whichever key
  TCL listens to at that moment. TCL reads the key through MCM Helper at load
  (`OnSettingChange`/`Maintenance`), so a changed key takes effect after the INI is written and
  the game reloaded; how Grapple's and Surrender's prompt-only handled the same timing is the
  pattern to copy.

## Open before building

- The two SI numbers (5 s, level 14) are SI's; confirm the light level reads the same scale.
- How the existing prompt-only targets write another mod's key and when the mod picks it up
  (`Surrender`, `Grapple`, `Execute` for Valhalla); TCL goes through MCM Helper.
