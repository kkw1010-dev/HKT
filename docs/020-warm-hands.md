# 020 · Warm hands

Status (2026-09-22): first in-game test: prompt, braziers and hearths, "not with the back to
the fire" and pass time passed; crouching never happened, forges never offered, and movement
did not end it (only a jump did). Second test: crouching, braziers, movement exit and jumping out
passed; forges still did not offer (fixed below, not yet retested). The user picked it
as the next SI absorption and accepted the defaults below.

## As built

- `Rest::ScanFire`, run with the lean scan every 250 ms while the player stands ready: loaded
  references within 200 units whose base is in `Survival_WarmUpObjectsList` (a flat list of 90:
  70 movable statics, 10 furniture, 6 activators, 3 lights, 1 static), not disabled or deleted,
  within 60° of the player's facing; the closest wins. It is scanned again at the press.
- Crouched when the fire's top (reference Z + bound max Z x scale) is under 50 units above the
  player's feet, otherwise standing.
- Prompt 손 녹이기 (길게), hold. The pose is a `Rest` pose, so movement exits, combat exits, and
  시간 보내기 appears while warming.
- Logged: the gate's `fire=` field, and on accept `fire scan: fire <name> (<refid>) at N units,
  N deg off, top N above the feet -> warming hands (crouched|standing)`. Without the Survival
  Mode plugin the load log says 손 녹이기 is off.
- `verify_deploy.py` checks both idle events in the winning `mt_behavior.hkx`.
- The player is not turned to face the fire; the idle plays along the current facing, up to 60°
  off.

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

## Fixes after the first in-game test (2026-09-22)

- **Always standing.** The crouch test used the top of the fire's bounds, which include flames
  and smoke (85-317 units in the log), so every fire came out standing. It now uses where the
  fire stands: its origin under 25 units above the player's feet crouches (campfires, fire pits,
  cooking pots, floor hearths); a forge or smelter (workbench keyword `CraftingSmithingForge`,
  `CraftingSmithingSkyforge` or `CraftingSmelter`) and anything raised (a brazier) stands.
- **Forges never offered.** Forges are in the list (five of them) and Base Object Swapper does not
  touch them; a forge's origin sits in its middle, so the player at its front was more than 200
  units away. A fire now counts within 200 units of its origin or 100 of its horizontal bounds,
  searched to 400. When the closest fire is still turned down, the log says so once with its
  distance, edge distance and angle.
- **Movement did not end it.** Warm hands is a plain idle: no `idleChairSitting`, and
  `IdleChairExitStart` was refused. Movement input waited for the 6 s confirmation, and the
  player had meanwhile jumped out, so CIGAR sent `IdleForceDefaultState` to a sprinting player.
  Now warm hands counts as settled after 1 s, gets up with `IdleStop`, and ends without sending
  anything when the game's own `IdleStop`, `tailMTIdle` or `tailMTLocomotion` arrives (warm
  hands only; the sit, lie and lean exits passed and are left as they are).

## Forges, second attempt (2026-09-22)

The log explained it: `closest fire not offered: 대장간의 화로 (000C430A) at 63 units (edge 0),
113 deg off`. Reach passed; the angle was taken to the forge's origin, which from its front sits
off to the side. Distance and angle are now taken to the closest point of the reference's
rotated bounds seen from above (the bounds turned by the reference's Z angle, with the same
clockwise convention as the player's facing); standing within the bounds counts as touching it.
The transform was checked against a brute-force search over 20 000 random boxes (no mismatch);
the rotation convention itself is assumed, not yet confirmed in game.

## Result (2026-09-22)

Forges now offer from any face and not from behind: every warm-hands check has passed in game
(crouching at floor fires, standing at braziers, hearths and forges, the facing cone, pass time,
the movement exit, jumping out). The warm-hands clips come from Dynamic Female Hand Warming,
which picked one pair by body armor; they are now random through the lean-variety OAR patch
(install case 016), which brings its other crouched clips, a kneel among them, into play.

Random hand-warming clips (a kneel among them) passed in game on 2026-09-24.
