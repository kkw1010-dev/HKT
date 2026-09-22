# 019 · Rest: sit or lie down on the ground

Status (2026-09-22): floor sit, lie down, get up and ledge sit confirmed in game by the user.
Lean in game 2026-09-22: wall (facing only), table and rail leans, hold prompts and
random clips passed. The movement exit (see "Getting up") passed in game the same day
for every pose: wall, table and rail leans, sit, lie, input queued during the enter
animation, jump out, and no prompt on screen while resting.

## Pass time (2026-09-22, not yet tested in game)

SI's pass time, as the user specified: time speeds up only while a key is held, never
by itself and never through the Wait menu (the reverted `6dd9a7d` opened it).

- Five seconds after a rest settles (SI's `passtime_delay`), any pose, 시간 보내기
  (누르고 있기) appears. It is a hold-mode prompt without a ring: key down and key
  up go to `Rest::OnHold`.
- While the key is down the game clock's timescale global climbs from x1 to x60
  over 3 s; release, getting up, combat, the rest ending or the module being
  switched off put it back at once. x60 and 3 s are my choices: SI's
  `max_timemult` is 2.0, which barely moves the clock. At the vanilla timescale 20,
  x60 is 20 game minutes per real second.
- Changing the timescale, rather than GameHour, lets the engine advance the day,
  month and days-passed globals together (survival needs follow).
- The timescale global is saved with the game, so SKSE's kSaveGame (sent before
  the file is written) restores the real value first; the next tick speeds up again
  if the key is still down. A timescale above 100 at load is logged and notified,
  in case an accelerated value was saved anyway.
- Logged: the base timescale when the clock speeds up, and on stop the reason, how
  long the key was held, how many game hours passed, and the restored value.

## Getting up (redesigned 2026-09-22)

There is no 일어나기 / 그만 기대기 prompt. The user dropped it: these poses are for
roleplay and screenshots, and an exit prompt on screen spoils them. A jump already
leaves at once, so a slow exit costs nothing in a safe spot.

- Movement input gets the player up with the slow exit (`IdleChairExitStart`, or
  `IdleRailLeanExit` for the rail). Input is read from `PlayerControls`
  (`moveInputVec` at least 0.2, or auto-move), not `IsMoving()`: the wall lean
  reports `moving=true` for its whole loop, which is why the earlier movement check
  kept ending wall leans at once (A4 failed twice).
- Input during the enter animation is queued and acted on once the pose is reached
  (`idleChairSitting` / `tailLayDown`) or after 6 s.
- The game standing the player up by itself (a jump, a drawn weapon) is read from
  its sit state: once it has been on during the rest, it going off ends the rest
  with nothing sent. Floor sit and lie are not known to set that state, so for
  them only movement input, combat and a drawn weapon end the rest.
- Combat still sends the exit at once.
- The gate logs `moveInput`, `exitQueued`, `settled` and `seated`.

Absorbs the ground actions of SI's `IdleActions` (앉기, 눕기, and getting up). SI's
`IdleActions.enabled` is off since 2026-09-21; the rest of that module (lean, warm
hands, chair eat/drink, tidy-up, pass time) is on the HANDOFF backlog.

## What SI does (from its DLL strings)

- Sends vanilla animation events to the player: `IdleSitCrossLeggedEnter`,
  `IdleLayDownEnter`; gets up with `IdleChairExitStart`, also naming `IdleStop` and
  `IdleForceDefaultState`.
- Waits for the tags `idleChairSitting` and `tailLayDown`, presumably to know the
  pose is reached (pass time counts from there).
- `t_threshold` 1.0: seconds of standing idle before its prompts.

## Ledge sit

SI sits on an edge with the legs hanging (`IdleSitLedgeEnter`) when a ray scan
(`RayCollector` in its DLL) finds a drop ahead; see `SI_IDLE_LEDGE_SIT_ANALYSIS.md`
(Antigravity's report). Its DLL has no furniture-activation strings, so "sitting on
furniture" in SI is this edge sit on a bench, wall or slope, not a chair being used.

CIGAR on 앉기: rays on the line-of-sight layer, in the player's own collision group so
the player is not hit. Ground under the player; a knee-height ray 55 units forward
(anything hit means a wall or furniture face, so no ledge); ground at 25, 40 and 55
units ahead. A drop of 40 units or more at any probe picks the ledge sit. The
numbers are my choices. Each sit logs `ledge scan: groundZ=.. drop@25=.. ... ->
ledge|floor`. If the graph refuses the ledge event, CIGAR sits cross-legged.

## Lean

SI has `LeanWall`, `LeanTable` and `LeanEdge` picked by `GetLeanLvL`, sending
`IdleWallLeanStart`, `IdleLeanTableEnter` and `IdleRailLeanEnter`; its get-up sends
`IdleRailLeanExit` for the rail.

CIGAR offers 벽에/탁자에/난간에 기대기 (hold) after the same 1 s of standing still,
without the floor-pitch condition, rescanning every 250 ms:

- In front: at 25, 35 and 45 units ahead, a ray from 140 units above the ground down
  to 20 units. The first surface 60-95 units high is a table, 95-125 a rail.
- Wall (only when no table or rail): rays 40 units ahead at waist (60) and chest
  (100) height; both hitting is a wall in front.
- `IdleWallLeanStart` turns the player around before leaning back (seen in game
  2026-09-22: with the back to a wall it leaned on air). The wall lean is therefore
  offered only facing a wall. A back-to-wall variant with `IdleWallLeanEnterInstant`
  worked but the user dropped it: each extra entry path multiplies the motion-sync
  cases to solve.
- Until the pose is reached (`idleChairSitting`, or `tailLayDown` for lying; up to 6 s),
  movement does not end the rest. In game (2026-09-22) the wall lean's turn set
  `moving=true` in the same tick as the enter event, so CIGAR dropped the rest at once
  and 그만 기대기 never appeared. `idleChairSitting` arrived about 3 s later, and the
  game's sit state (`seated`) was on while leaning. The table lean sends the same tag.
- Table, rail and wall leans play vanilla enter and exit clips around the OAR loop:
  no installed mod replaces `IdleLeanTable enter/exit.hkx`, `RailLeanEnter/Exit.hkx` or,
  for women, `Wall_IdleBackEnter.hkx` (checked across every enabled mod, 2026-09-22).
- The lean prompt was single press in the first build (its `SetPromptType(kHold)`
  was missing); fixed.
- The numbers are my choices. Accepting logs `lean scan: groundZ=.. front@25=..
  ... back waist=.. chest=.. -> leaning on ..`.
- The animation plays where the player
  stands; the player is not moved to the surface, so a lean from further away may
  float or clip.

## CIGAR's gate

- Offered (hold, 앉기 and 눕기) after 1 s of: looking down at least 0.6 rad (about
  35°; my choice, not the user's, logged as `pitch=`), not moving, out of combat,
  weapon sheathed, not in furniture, swimming, sneaking, midair, mounted or an
  animation-driven scene (`bAnimationDriven`), movement and look controls on, no
  menu or dialogue.
- Getting up: see "Getting up" above.
- First person is switched to third person first; the enter event is retried for
  1 s while the camera changes.

## Self-reporting

- Every event CIGAR sends is logged with whether the graph accepted it; a refused
  enter or get-up also notifies on screen.
- From entering until 5 s after getting up, the player's animation events are
  logged (cap 80). `WARN ... not confirmed` means no `idleChairSitting` /
  `tailLayDown` arrived within 6 s — the recorded events show what did.
- `verify_deploy.py` fails when the winning `0_master.hkx` lacks any of the four
  events, and warns at runtime if SI's `IdleActions.enabled` is on again.

## To check in game (lean)

1. Back to a wall, standing still: 벽에 기대기. Facing a table or bar counter:
   탁자에 기대기. Facing a railing: 난간에 기대기.
2. Hold it, then move to get up.
3. If the wrong kind or none appears, the `lean scan` line and the gate's `lean=`
   say why.

## To check in game (sit)

1. Sheathe, stand still, look at the floor: 앉기 / 눕기 appear within about 1 s.
2. Hold 앉기: the player sits; move to stand up slowly.
3. Same with 눕기.
4. Read `CIGAR.log` for `[Rest]`: the accepted flags, `reached` or `WARN not
   confirmed`, and the recorded events.
