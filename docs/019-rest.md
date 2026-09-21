# 019 · Rest: sit or lie down on the ground

Status (2026-09-21): built and deployed; **not yet tested in game.**

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

## To check in game

1. Sheathe, stand still, look at the floor: 앉기 / 눕기 appear within about 1 s.
2. Hold 앉기: the player sits cross-legged; 일어나기 appears; hold it to stand.
3. Same with 눕기.
4. Read `CIGAR.log` for `[Rest]`: the accepted flags, `reached` or `WARN not
   confirmed`, and the recorded events.
