# 013 · Needs (Private Needs - Orgasm)

**Status:** confirmed in game 2026-09-19 (test 1, all four steps passed); start threshold 50% since 2026-09-20.

## Target

[SL+] Private Needs - Orgasm KOR 1.10.2 (`Private Needs - Orgasm.esp`, ESL-flagged). The install
is recorded in `TKL-Agent/Installation and Modification/cases/008-private-needs-orgasm-installation.md`.
The shipped `.psc` sources match the `.pex` for every name below (checked by `verify_deploy.py`).

| Form | What CIGAR uses |
|---|---|
| `PNO_Config` `00087C` | `PNO_ConfigScript`: `bladdertoggleVal`, `boweltoggleVal`, `bladdercontent`, `bowelcontent`, the six key variables, `mapKey()`. `PNO_Utilityscript` (same quest): `UrinateAndDefecate(int, Actor, ObjectReference)`, `IsInSexScene` |
| `PNO_MainQuest` `00087B` | running state; `PNO_QF_MainQuest`: `bladder_lastlevel`, `bowel_lastlevel` |
| `PN_ExcreteAnimHandleMgef` `000853` | active on the player while PNO strips, plays and excretes |

Levels are `content / size * 5`, capped at 5, with `bladderSize` 600 and `bowelSize` 1440
(autoreadonly, so the sizes are constants in `Needs.cpp`).

## What PNO's hotkeys do (`PNO_ConfigScript.OnKeyDown`)

Nothing happens unless the main quest runs and `bladdertoggleVal` is on.

| Key (default) | Action |
|---|---|
| Universal (Y, 21) | `PNMenu()`: a message box, 소변 → `UrinateAndDefecate(1)`, 대변 → `UrinateAndDefecate(3)` |
| CheckNeeds (U, 22) | `CheckNeedsMessage()`: level and percent notifications |
| Urinate / Excrete (unset) | `UrinateAndDefecate(1)` / `(3)`; during a sex scene, `Orgasm(player)` instead |
| Wetself (unset) | `WetSelf(player)` |
| Toilet (unset) | activates the crosshair's sit furniture, then `UrinateAndDefecate(3, none, furniture)` |

The Y/U defaults are set by PNO's MCM **Initialize** button (`loadSetting(true)`), not at game
start; before that every key is -1. The handler (`pno_ExcreteAnimHandleMgef.PrepereCheck`) refuses in
combat, while swimming, at level 0, when `DisableUrinateKeywords` / `DisableDefecateKeywords` items
are worn, and (with the privacy option) when watched; it shows its own notification for each.
Type 3 urinates as well when the bladder level is above 0. Sitting on furniture in
`pn_ExcreteDootyList` triggers type 3 by itself (`PNO_PlayerAliasScript.OnSit`).

## The module

- **Prompts.** 소변 보기 (N%) while the bladder is on and at or above the panel's fill (default 50%, the user's choice 2026-09-20; was stage 1,
  the lowest level PNO acts on); 대변 보기 (N%) likewise for the bowel. The bowel is **off by default
  in PNO's MCM** (`boweltoggleVal = false`), so 대변 보기 needs it switched on there. A prompt is
  offered again when its level changes, so the percent stays roughly current.
- **Hidden** while PNO's main quest is stopped (notified once per session), while excreting, in
  combat, swimming, seated or on furniture, mounted, without movement controls, in a sex scene
  (PNO's `IsInSexScene` or SexLab's animating faction) and for 3 s after an accept.
- **Accept** calls `UrinateAndDefecate(1 | 3, None, None)`, the same call as PNO's keys and menu.
  CIGAR then watches for PNO's excrete effect: none within 3 s is logged as a WARN and notified
  (PNO refused; its own notification says why); when the effect ends, the log shows the fill
  before and after (PNO resets it in its pee/poop effect).
- **Prompt-only** (`privateneeds`, default on) sets all six key variables to -1 and calls `mapKey()`,
  remembering each bound key in `CIGAR.json` (`promptOnly.privateneeds.manualKeys`). Switching it off
  gives each remembered key back once. Keys are checked at load, from the panel's key check button,
  and 1.5 s after the journal menu closes, because PNO binds Y/U again whenever its MCM initialises
  or loads a profile. Nothing polls.

## Not covered

- The toilet key (any sit furniture) and the wet-self key: not needs-driven, left unbound.
- Orgasm during a scene (PNO's excrete keys call `Orgasm(player)` then): no prompt; the prompts are
  hidden in scenes.

## Test 1 (2026-09-19): passed, all four steps

1. New game, PNO MCM: switch the mod on and press Initialize, close the menu. Expect a log line
   `PNO keys changed (prompt-only=true): menu 21->- check 22->-` and Y/U doing nothing.
2. Wait for bladder level 1 (or raise `bladderrate`). Expect 소변 보기 (N%). Accept: PNO strips,
   plays, pees; the log shows `excrete handler ended: bladder N% -> 0%`.
3. Turn the bowel on in PNO's MCM; at level 1 expect 대변 보기 (N%).
4. Enter combat, swim, sit: the prompts go away.
