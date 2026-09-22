# 020 · Warm hands (plan)

Status (2026-09-22): researched, not built. The user picked it as the next SI absorption.

## What SI does (DLL strings)

- `IdleActions::Act: Warming hands.`; idles `IdleWarmHandsStanding` and `IdleWarmHandsCrouched`.
- `IdleActions::LoadFireSources`, with `Fire sources not found` and `Campfire (CC) not found`
  and a reference to `ccqdrsse002-firewood.esl`: it builds its own list of fire objects,
  including the Creation Club campfire.
- It was part of `IdleActions`, which is off since 2026-09-21, so the action is gone from the
  game until CIGAR has it.

## Facts checked on this modlist

- **Idles.** `Skyrim.esm` IDLE `IdleWarmHandsStanding` (0E8642) and `IdleWarmHandsCrouched`
  (0E8643) send the events of the same names. The events live in `mt_behavior.hkx`, not
  `0_master.hkx` (both read from `MUNG - Pandora Output NEW`), so `verify_deploy.py`'s Rest check
  must look there for them.
- **What counts as a fire.** Survival Mode finds heat sources through
  `Survival_WarmUpObjectsList` (`0008AA:ccqdrsse001-survivalmode.esl`): its
  `Survival_HeatSourceLocatorQuest` aliases take the closest loaded, enabled reference whose base
  is in that list. The winning copy (`Embers XD - Patch - Survival Mode Improved.esp`) holds 90
  base objects: vanilla fires, USSEP's additions, Survival Mode Improved's and Embers XD's. That
  is a maintained list on this modlist, so CIGAR reads it instead of keeping its own.
  OCF's `OCF_FL_*_HeatSource*` lists are empty in the plugin (filled at run time, if at all).

## Plan

- A new pose in `Rest`, so it inherits what already passed in game: a hold entry prompt,
  movement input exits, combat exits, pass time while held, the animation-event log.
- Gate: the standing-ready conditions `Rest` already checks, plus a reference whose base is in
  `Survival_WarmUpObjectsList`, enabled, within reach and roughly in front of the player.
- Crouched or standing by the fire's height: the top of its bounds below about knee height
  (a campfire) → `IdleWarmHandsCrouched`; higher (a brazier, a forge, a hearth) →
  `IdleWarmHandsStanding`.
- Soft integration: without the Survival Mode plugin or the list, the prompt never appears and
  the log says why once.
- Prompt text: 손 녹이기 (길게).

## Decisions with a default (numbers are mine; the user may change them)

- Reach: 200 units from the player to the fire reference.
- In front: within 60° of the player's facing, so walking past a fire with the back to it does
  not offer it.
- Exit: the same as sit and lean; no exit prompt.

## To find out in game

- Which exit event the warm-hands state takes (`IdleChairExitStart`, then `IdleStop`, as for the
  other poses; the log records which one the graph accepted).
- Whether it sends `idleChairSitting` like the leans. If not, `WARN not confirmed` appears after
  6 s and the rest counts as settled then; harmless, but worth one look.
