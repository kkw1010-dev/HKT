# 021 · Make light

Status (2026-09-25): **abandoned; the module is removed.** The user ended CIGAR's lighting work
("횃불 랜턴 CIGAR 편입 프로젝트는 전면 폐기, 그냥 단축키로 관리하는게 낫겠다"). Torches Candlelight
and Lanterns is restored and runs on its own hotkey (259, mouse button 4). CIGAR keeps only the
`ItemEquip` filter that hides TCL's lantern armors. Everything below is history.

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

## As built (2026-09-24)

- `src/Light.cpp`. TCL forms resolved at load: quest `i329TCL_MCM` 089F with
  `i329TCL_MCMConfig_Script`, global `i329Hotkey` 08E7 (the key TCL registered; `OnKeyUp`
  compares against it), `i329IsLanternHandOn` 086F, the four lit-lantern lists, keyword
  `i329IsCandlelightSpell`. Any of the first three missing: notify once, module idle.
- Darkness: a `TESConditionItem` running `GetLightLevel < 14` on the player (engine evaluation).
  The gate logs a band (`<5`, `<14`, `<30`, `<60`, `<100`, `>=100`) for tuning.
- Lit: a light in either hand, an active light-archetype effect or one with TCL's candlelight
  keyword, TCL's hand-lantern flag, or a worn armor from the lit-lantern lists.
- Offered (hold) after 5 s dark, not lit, out of combat, no menu or dialogue. Accepting presses the
  key in `i329Hotkey`; 3 s later the log says whether something is lit.
- Prompt-only (panel target `tcl`, hidden key 0x67): the key is read from the MCM user INI, then
  the default INI; the player's key is remembered as the manual key. Changing it dispatches
  `SetModSettingInt("iTCLHotkey:Controls", key)` on TCL's MCM script (MCM Helper, which also
  writes its user INI), then `OnSettingChange("iTCLHotkey:Controls")`, which re-registers the key
  in-session. The log then compares `i329Hotkey` with the key sent and notifies on a mismatch.
  The panel's 모드 키 다시 확인 button and the load re-run the check.

## Found on the way

`verify_deploy.py` failed on Grapple: `FH_Grapple_Plugin.ini` read `kbKey=-1`, written by Grapple
itself at 19:26 after CIGAR had set the hidden key at 19:25 (log). With prompt-only on, CIGAR sets
the hidden key whatever the INI says, so the check now requires a valid INI key only when
Grapple's prompt-only is off.

## In game (2026-09-24) and changes

Passed: the prompt in the dark, lighting through TCL, no prompt when lit, bright or in combat,
mouse 4 no longer reaching TCL under prompt-only, and the panel switch restoring it.

- **At once.** The user wants the prompt as soon as it is dark; SI's 5 s wait is replaced by a
  300 ms debounce (the user's choice; the 300 ms is mine, against flicker at a light's edge). The
  engine's `GetLightLevel` is enough: Community Shaders changes rendering, not the value the
  engine's conditions read.
- **Duplicated lanterns.** Reported by the user. TCL's own changelog, v1.35: "hopefully fix random
  dupe on toggle off"; installed is 1.58 (2026-02-01), newest 1.60. CIGAR's press reaches the same
  `OnKeyUp` -> `PlayerLanternToggle` path as TCL's own key. CIGAR's `OnSettingChange` calls (load
  and panel only; three in the first session's log) also run `AddLanternToggleOnce`, which checks
  the item count before adding the toggle item. (Call lists read from the compiled
  `i329tcl_mcmconfig_script.pex` with a small PEX reader in the session scratchpad.) To settle it,
  each press now logs every TCL item whose count changed 3 s later.

## Not in water (2026-09-24)

The user saw 불 밝히기 while swimming: water reads dark to `GetLightLevel`. The gate now also
requires not swimming and a submerge level below 0.5, and logs `water=`.

## Without TCL (2026-09-25)

The user's rules: vanilla torches and light spells only, and **no prompt when the player has
neither** (a lantern prompt with nothing to light was the complaint).

- Gate (100 ms): `GetLightLevel < 14` for 300 ms, not lit (a light in either hand or an active
  light-archetype effect), not in combat, no menu or dialogue, not in water, and a source.
- Source: a carryable torch (LIGH with `CanBeCarried`) in the pack, the brightest one; else a
  known light spell (effect archetype `Light`, from the player's added spells and the base's spell
  list) that magicka covers, self-delivered ones (Candlelight) before aimed ones (Magelight), then
  the cheapest. The gate line lists the torch, the spell and how many light spells are short of
  magicka; "dark, but no torch ... or light spell" is logged once when that blocks the prompt.
- 불 밝히기 (길게): <횃불 또는 마법>. A torch goes into the left hand and what the left hand held
  (weapon, shield or spell; not the other half of a two-hander) is remembered. A spell is cast
  through the instant caster (self-target for Candlelight) and its magicka cost is taken.
  1.5 s later the log says whether the player is lit, with a notification if not.
- 불 끄기 (길게) — the open question from 2026-09-24, settled by CIGAR's choice: offered while
  CIGAR's torch is in hand and the light has held at 30 or brighter for 3 s, out of combat. It gives
  the left hand back (or empties it when the saved item is gone). A spell light is left to expire.
- If the player changes the left hand themselves, the saved item is forgotten.
- TCL's prompt-only target (`tcl`) and its panel line are gone.
