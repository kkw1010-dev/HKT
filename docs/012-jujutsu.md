# 012 · Jujutsu (유술): a kill move on a guarding humanoid, without the kill

Status (2026-09-19): in progress. This is the first CIGAR module with a game hook.

## Test 1 (2026-09-19) and what changed

- Two plays started (KneeThrow, SlamA): `pair started after 0.06 s`, and then
  `KillActor swallowed for victim at 2.07 s`. **The victim was already dead** at that tick
  (`payoff skipped: victim dead`, `ragdoll=true`). So swallowing KillActor alone does not keep the
  victim alive; something else in the kill move kills it. KillMoveEnd comes 5 ms after KillActor in
  KneeThrow, which makes it the first suspect, but deaths were not sampled finely enough to say.
- Four plays were refused (`SetupSpecialIdle returned false`). All four victims were blocking at
  that moment; both accepted plays had victims that had just lowered their guard.
- **Changes for test 2:**
  - A blocking victim gets `blockStop` sent to its graph, and the play is retried for 0.6 s.
  - KillMoveStart and KillMoveEnd are hooked too, and every victim event is time-stamped.
  - The victim's life state is sampled every 100 ms.
  - Attempts alternate two strategies, each announced on the HUD:
    - **A:** the victim's own Actor essential flag (`boolFlags.kEssential`, not the shared base)
      is set for the kill move and restored afterwards;
    - **B:** the victim's KillMoveEnd is swallowed as well, and its in-kill-move flag is cleared
      when the pair ends.
  Whichever keeps the victim alive and looks right becomes the only path, and the HUD notice goes.

## Test 2 (2026-09-19): KillMoveEnd is the kill

- **Strategy A (essential flag) died every time.** The victim turned `dead=true life=1` on the very
  tick its KillMoveEnd was passed through (2.90 s). One such play saw no KillActor at all before it.
- **Strategy B (KillMoveEnd swallowed) kept every victim alive.** The payoff landed (stun 56 of 112,
  health -6), and the user saw the Valhalla gauge fill and the victim become executable.
- **B's flaw, seen by the user:** the victim stands straight up at the end of the throw. The user
  suggested ragdoll as the least awkward ending.
- **Refusals:** about 4 of 10 plays were refused for the whole 0.6 s window, often with the guard
  already down, so blocking is not the only cause.

**Changes for test 3:**

- Strategy A and the HUD notice are gone. KillActor and KillMoveEnd are always swallowed for the victim.
- At the pair's end the victim's in-kill-move flag is cleared, and the victim is knocked into ragdoll
  with `AIProcess::KnockExplosion` (what Papyrus `PushActorAway` calls). The magnitude is 1.0, a
  placeholder, so it drops where it lies.
- 0.4 s later the log says whether it is in ragdoll (`WARN` if not).
- Every retry logs both actors' attack state, knock state, stagger, sync, sprint and ragdoll, and the
  distance, so the next log shows why a play is refused.


## What the user asked for

- When a humanoid enemy is guarding, a prompt plays one of the four vanilla
  hand-to-hand kill moves: KneeThrow, BodySlam, ComboA or SlamA.
- The victim survives. It loses its stamina, or, with Valhalla Combat, takes a
  large hit to its stun meter, and it takes a little damage.
- There is no weapon restriction for now ("just trigger it"). Whether it looks
  awkward is for the user to judge.
- Balance (cooldown, cost) is deferred until the function works. The numbers
  below are placeholders, not the user's.

## Why the victim would die, and how it doesn't

- The victim's clip in `animationdatasinglefile.txt` ends with `2_KillActor`.
  KneeThrow fires it at 1.928 s, SlamA at 1.967 s, ComboA at 2.768 s (after its
  PairEnd) and BodySlam at 4.136 s. The .hkx files carry no annotations. Check
  with `TKL-Agent/Toolchain/tools/anim_events.py`.
- `Jujutsu::InstallHook` (kDataLoaded) swaps slot 1 (`ExecuteHandler`) of
  `RE::VTABLE_KillActorHandler[0]`. While a 유술 is armed, a `KillActor` for the
  victim, or for the player, returns without calling the original. Any other
  actor's kill is passed through. The hook touches only atomics; the game thread
  logs.
- The hook stays armed until 2 s after the pair ends, because ComboA's
  KillActor comes after its PairEnd.

## Gate and play

- **Target:** a hostile humanoid (body part data `0x1D`) within 250 units that
  is blocking (`Actor::IsBlocking`). It must not be in a kill move, mounted or a
  teammate. It stays offered for 0.7 s after its guard drops.
- **Player:** movement controls on, not in a SexLab scene, humanoid, not
  mounted, not in a kill move.
- **Prompt:** `유술: <이름>`.
- **Idles:**
  - with ValhallaCombat.esp, its condition-free copies (AA3A–AA3D);
  - otherwise Skyrim.esm/Update.esm (0F9958, 100EF8, 820, 821). Update's
    BodySlam and KneeThrow carry `GetRandomPercent` conditions that may refuse
    a forced play.

  One is picked at random and played with `AIProcess::SetupSpecialIdle`
  (kActionIdle), as Valhalla plays its executions.
- **Payoff:** applied at the swallowed KillActor, or at the pair's end if none
  came.
  - With Valhalla: 50% of its max stun (`(base health + base stamina) / 2`)
    through `processStunDamage(timedBlock)`, which multiplies by
    `fStunTimedBlockMult` (1).
  - Without: stamina to 0.
  - Then 5% of max health, never below 1.

## Self-reporting (`[Jujutsu]` in CIGAR.log)

- At load: `idles from ...: KneeThrow=... BodySlam=...; hook=true valhalla= sexlab=`.
  The hook line itself is `KillActorHandler::ExecuteHandler hooked`.
- `start idle <id> on <name>`, then `SetupSpecialIdle returned`, then either
  `pair started after N s` or `WARN ... did not start a pair` (with a HUD notice).
- `KillActor swallowed for victim|player at N s`, then `payoff: ...`.
- `pair ended after N s; victim: dead= health= stamina= bleedout= knock= ragdoll= killmove= synced= blocking= stunned=`,
  and the same line 2 s later. A dead victim gives a `WARN` and a HUD notice.
- `tools/verify_deploy.py` fails the deploy if the idles' EditorIDs move or the
  active behaviour output's victim clips stop ending with `2_KillActor`.
