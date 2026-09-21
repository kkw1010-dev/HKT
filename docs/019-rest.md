# 019 · Rest: sit or lie down on the ground

Status (2026-09-22): floor sit, lie down, get up and ledge sit confirmed in game by the user.
Lean tested once in game: wall direction and the table hold were wrong, both fixed
and redeployed, not yet retested.

Absorbs the ground actions of SI's `IdleActions` (앉기, 눕기, 일어나기). SI's
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
- Wall (only when no table or rail): rays 40 units at waist (60) and chest (100)
  height, forward first, then back; both hitting is a wall.
- In game (2026-09-22) `IdleWallLeanStart` turned the player around before leaning
  back, so with the back to a wall it leaned on air. It is now sent only for a wall
  in front (the turn puts the back to it); with the back already to the wall CIGAR
  sends `IdleWallLeanEnterInstant`, which enters the lean without turning.
- The lean prompt was single press in the first build (its `SetPromptType(kHold)`
  was missing); fixed.
- The numbers are my choices. Accepting logs `lean scan: groundZ=.. front@25=..
  ... back waist=.. chest=.. -> leaning on ..`.
- While leaning the prompt reads 그만 기대기. The animation plays where the player
  stands; the player is not moved to the surface, so a lean from further away may
  float or clip.

## CIGAR's gate

- Offered (hold, 앉기 and 눕기) after 1 s of: looking down at least 0.6 rad (about
  35°; my choice, not the user's, logged as `pitch=`), not moving, out of combat,
  weapon sheathed, not in furniture, swimming, sneaking, midair, mounted or an
  animation-driven scene (`bAnimationDriven`), movement and look controls on, no
  menu or dialogue.
- While resting, 일어나기 (hold) appears after 2 s, so the enter animation is not
  cut short.
- Combat gets the player up. Moving or drawing a weapon ends the rest state
  without sending anything (the game already stood the player up).
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
2. Hold it, then 그만 기대기.
3. If the wrong kind or none appears, the `lean scan` line and the gate's `lean=`
   say why.

## To check in game (sit)

1. Sheathe, stand still, look at the floor: 앉기 / 눕기 appear within about 1 s.
2. Hold 앉기: the player sits cross-legged; 일어나기 appears; hold it to stand.
3. Same with 눕기.
4. Read `CIGAR.log` for `[Rest]`: the accepted flags, `reached` or `WARN not
   confirmed`, and the recorded events.
