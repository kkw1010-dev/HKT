# 021 · Make light (plan)

Status (2026-09-24): researched, not built. Recommended as the next SI absorption.

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
  archetype), or a TCL lantern worn. The lantern test needs TCL's lantern forms or keyword, to be
  read from `TorchesCandlelightLanterns.esp` before building.
- Prompt-only: CIGAR's standing rule is that its prompts replace other mods' hotkeys
  (`Settings::PromptOnly`, keys moved to F13/F14 so the real key is free). TCL is on mouse 4 now;
  ask the user whether TCL joins prompt-only mode or keeps its key as well.

## Open before building

- The two SI numbers (5 s, level 14) are SI's; confirm the light level reads the same scale.
- The lantern forms or keyword in TCL.
- The user's answer on TCL's hotkey.
