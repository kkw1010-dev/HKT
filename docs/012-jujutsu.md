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

- **Target:** a hostile humanoid (body part data `0x1D`) within the panel's 유술 거리 (3. 세부 설정, 100–400, default 250 by the user's choice) that
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

## Distance check (2026-09-19)

The user suspects the refused plays were targets beyond 250 units: bandits never stop moving. The
prompt already requires the target within reach at the press, so a target out of reach at the press
logs `accept ignored`, not a refusal. The engine may still refuse a pair that has drifted during the
0.6 s of retries. So the distance is logged at the press (`start ... distance=`), on every retry
(`try N: ... distance=`) and with the number of tries on success (`pair started ... after N tries`).
Refused and accepted distances can be compared in one log.

## Test 3 (2026-09-19)

- **Survival and payoff hold.** Every accepted play left the victim alive, with the stun share and
  health loss applied.
- **The victim stood up before going down** (the user's report). The log shows why: the victim's own
  clip ends at its KillMoveEnd (about 1.9-4.3 s), and it returns to standing then. The knock-down
  waited for the player's side of the pair, which ends 0.3-0.5 s later and longer on some idles.
  **Fix:** the KillMoveEnd hook queues the knock-down for the next frame, on the victim's own
  would-be death moment. The 0.4 s ragdoll check now counts from the knock.
- **Refusals.** Accepted plays were at 100, 121, 145, 176, 178 and 193 units, on five bandits. All
  five refused plays were on one bandit, at 66-153 units. The user suspects that close range fails.
  Short range may be part of it, but 120-153 failed on that bandit while 121 worked on another, so the
  log cannot separate range from that bandit's situation yet. Refusals now also log the height
  difference, which way each actor faces the other, the victim's race and its weapon.
- **Hidden while executable.** At the user's request, a target Valhalla reports stun-broken gets the
  처형 prompt only; 유술 skips it.

## Test 4 (2026-09-19)

- **The knock-down at KillMoveEnd looks natural** (the user), and hiding 유술 for stun-broken targets works.
- **Refusals are not about range.** Accepted plays ranged 86-195 units and refused ones 44-227.
  One bandit was refused six times and then accepted on the seventh try, and bandits that were not
  blocking were refused too.
- **The user's suspect is NPC Block Loop Fix,** an OAR replacer of the NPC block idles. It applies only
  while the NPC is moving (`IsMovementDirection != 0`, player excluded), and its clips fire `blockStop`
  every second. Test 4's `try` lines were written after the call, so they showed the state an accepted
  play had already changed, and nothing about movement.
- **Changes for test 5:** each try now logs both actors' state from before the call and before
  `blockStop`: attack, knock, stagger, sync, kill move, sprint, ragdoll, the graph's `Speed`,
  `IsBlocking` and `IsAttacking`, facing, distance and height. Comparing refusals by `speed` separates
  "moving" from "not moving". Running once with NPC Block Loop Fix unticked separates the mod from
  movement itself.

## Test 5 (2026-09-19): baseline with NPC Block Loop Fix ON

Log kept at `docs/testlogs/2026-09-19-jujutsu-test5-blockloopfix-ON.log`; CIGAR.log is recreated
on every launch.

- **39 attempts on 17 bandits: 34 played, 5 refused (13%).** Test 3 refused 5 of 10, but that was
  mostly one bandit.
- **A refusal holds for the whole attempt.** All 8 retries (0.6 s) fail together, and a play almost
  always lands on its first try; only 3 of 34 needed retries (5, 7 and 8 tries). So the deciding
  state lasts longer than 0.6 s and is fixed per attempt.
- **Not movement.** The victim's graph `Speed` at refused tries ranged 0-450, and plays were accepted
  at 0-508. The victim's own attack (`gAttack`), the player's speed, the distance (43-178 refused,
  47-174 accepted) and the facing do not separate the two either.
- **Clustering.** Three of the five refusals were one bandit, three attempts in a row (KneeThrow,
  ComboA, SlamA); it then took two plays at once. By idle: SlamA was refused 3 of 9, KneeThrow 1 of
  11, ComboA 1 of 14 and BodySlam 0 of 5. That is too few to blame SlamA.
- **Next:** the same test with NPC Block Loop Fix unticked, compared against these numbers.

## Test 6 (2026-09-19): NPC Block Loop Fix OFF, so it is not the cause

Log kept at `docs/testlogs/2026-09-19-jujutsu-test6-blockloopfix-OFF.log`.

- **41 attempts: 32 played, 9 refused (22%),** against 13% with the mod on. Taking the mod out did
  not help, so NPC Block Loop Fix is ruled out.
- **Refusals come in runs on one NPC.** One NPC was refused three times in a row (14:36:20-24),
  another five times in a row over 5 s (14:37:43-47). Every other NPC played on the first try. The
  user saw the refusals while NPCs were dodging.
- The list runs TK Dodge RE, including on NPCs. Its Nemesis patch adds a `TKDodgeState` state
  machine and the graph variables `bIsDodging`, `bInIframe` and `bIframeActive` to `1hm_behavior`
  and `magicbehavior`. A victim inside that state machine probably has no transition into the
  paired kill move.
- **Changes for test 7:**
  - each try logs the victim's and the player's `dodge` (`bIsDodging`) and `iframe` (`bInIframe`);
  - while the victim is dodging, the retry window stretches from 0.6 s to 1.5 s, so a dodge is waited
    out rather than refused.

  If the refused tries show `dodge=true` and the refusals drop, the cause is confirmed.

## Test 7 (2026-09-19): the player's own attack is the cause

Log kept at `docs/testlogs/2026-09-19-jujutsu-test7-dodge.log`.

- **30 attempts: 24 played, 6 refused.** The victim was dodging (`bIsDodging`) in only 2 of the 6
  refused attempts, and in both the refusal had started before the dodge. **Dodge is not the cause.**
  The user saw plays fire mid-dodge.
- **Pooled over tests 5-7:** in 14 of 15 refused attempts, the **player's** graph `IsAttacking` was
  set on every retry (attack state Draw or Hit), for the whole 0.6-1.5 s. At the first try, the player
  was mid-attack in 13 of 20 refused attempts and 20 of 90 played ones. The victim's attack, block,
  movement, the distance and the facing all overlap between the two groups.
- **Reading:** the engine does not start a paired idle while the requesting actor is in an attack.
  Holding or chaining attacks keeps it refused. This also fits the first-press misses of Valhalla's
  execution (see `011-execute.md`), where the key comes just after the stun-breaking blow.
- **Change for test 8:** if the player's graph is attacking, `attackStop` is sent to the player
  before each try. While the player is attacking or the victim is dodging, the retry window runs to
  1.5 s. The `try` line says `(sent attackStop to the player)`.

## Test 8 (2026-09-19): attackStop works; the window is now 0.3 s

Log kept at `docs/testlogs/2026-09-19-jujutsu-test8-attackstop.log`.

- **23 attempts: 20 played, 3 refused.** Of the attempts where the player was mid-attack at the
  press, 18 of 21 played, and all of those played on the first try with `attackStop`. Before, in
  tests 5-7, only 20 of 33 such attempts played (61%). This confirms the player's attack as the cause.
- **Remaining refusals:**
  - Two had the player attacking again on every retry, despite `attackStop` each time (the attack
    button held, or input buffered). Cutting those for 1.5 s is what the user saw as attacks stopping
    awkwardly after a failed grapple.
  - One had neither actor attacking nor dodging and is unexplained.
- **Retries were worth little.** 19 of 20 plays started at the press, and one was saved at 0.6 s.
- **Change (recommended to the user):** the retry window, and with it the `attackStop`s, is now
  0.3 s, with no extension for attacks or dodges. It cuts the swing in progress at the press and
  leaves the next one alone.

## Test 9 (2026-09-19 22:25): every kill move refused after Private Needs was installed

Log: `testlogs/2026-09-19-needs-test1-jujutsu-missing.log`. 11 presses, 54 tries, **0** accepted by
`SetupSpecialIdle` (test 8: 20 of 68 tries, 20 of 23 presses played). Valhalla's execution still
played on the same victim (FF001520) a minute later, so both actors can still enter a paired idle.

What changed between test 8 (15:24) and test 9: the Private Needs - Orgasm install (case 008, 15:37),
a Pandora regeneration for its FNIS list (15:40), the CIGAR `Needs` module, and a new session.

Ruled out, with evidence:

- **CIGAR code.** `Jujutsu.cpp` is unchanged since 16daf79; the Needs commit touched only
  `Util.cpp` (a new helper) and `main.cpp` (registration).
- **Pandora output.** Against the pre-install backup: `1hm_behavior.hkx`, which holds every
  `pa_KillMove*` state, is byte-identical; `animationdatasinglefile.txt` differs only in line order
  (the sorted files are identical; the H2H kill-move clip entries hash the same); `0_Master.hkx`
  only gains the `FNIS_Private_Needs_Behavior` reference. The Engine.log sets differ only by
  `FNIS_Private_Needs_List`; Pandora's order is nondeterministic between runs, which explains the
  many same-size files that differ (`mt_behavior.hkx`).
- **MO2 profile.** Against the `.bak_20260919_pno` files, the only change is the PNO mod and plugin.
- **PNO.dll's player vtable hook.** It hooks slot 0xAF on AE (0xAD on SE), which CommonLib names
  `UpdateCharacterControllerSimulationSettings`, but its thunk calls the original first with every
  argument register untouched, then runs PNO's ejection update.
- **PNO's SkyPatcher lines.** Only a hidden wet-self perk on female NPCs, keywords and projectile flags.

Not yet decided, because the logs do not carry it: the player's side. The try line now also logs
the player's right/left hand objects, race, sex, movement/fighting/activate controls,
`PNO_Animation_Idx`, `bAnimationDriven` and every active effect not from the base game.

## Test 10 (2026-09-19 23:20): diagnostic build, 5 of 13 presses played

Log: `testlogs/2026-09-19-jujutsu-test10-diag.log`. Player: Nord (UBE), female, iron axe and shield,
every control enabled, `PNO_Animation_Idx` 0, not animation-driven; a PNO urination finished 14 s
before the first press. So neither the weapon, a PNO lock on the player, nor a PNO effect decides it:
the plays and refusals had the same player state.

Across all tests, plays start on the **first try** (tests 5-8: 107 of 110); retries saved 3. So the
0.3 s window of 16daf79 is not the cause. What fell is the first-try rate: tests 5-8 about 80-100%,
test 9 0 of 11, test 10 5 of 13. Tests 5-6 pressed mostly while not attacking and still played
(idle presses 9/13, 25/26, 18/20, 10/10); tests 9-10 idle presses played 5 of 23.

Open lead: presses while not attacking go through `mt_behavior.hkx`, which the 15:40 Pandora run
rewrote: the same strings, reordered (Pandora's order is nondeterministic), so its state and
transition arrays are in a different order than in the build tests 5-8 ran on. `1hm_behavior.hkx`
(attacks, kill-move states) is byte-identical. Decisive check: put the pre-install
`mt_behavior.hkx` back (it carries no PNO content) and press while not attacking.

Test 11 setup (2026-09-19): `mods\MUNG - Pandora Output NEW\meshes\actors\character\Behaviors\mt_behavior.hkx`
was replaced by the pre-install copy (md5 cb3a5b03...), the file tests 5-8 ran on. The 15:40 Pandora
copy (md5 3168ff4d...) is kept at `build\mt_behavior.pandora-20260919-1540.hkx` (git-ignored) to
swap back. Neither copy has PNO content (PNO's FNIS list lives in `0_Master.hkx` and the character
files), so PNO keeps working. Any later Pandora run overwrites this experiment.

## Test 11 (2026-09-19 23:40): pre-install mt_behavior.hkx

Log: `testlogs/2026-09-19-jujutsu-test11-mtbehavior.log`. 8 of 12 presses played (6 on the first
try); presses while not attacking 7 of 11 (standing 6 of 8). Test 10, same build with the 15:40
`mt_behavior.hkx`: 5 of 13 (standing 1 of 7). Better, but below tests 5-8 and too few presses to
decide. NPC grapples (Grapple's own `bEnableNPCGrapple`) also hit the player during presses; the user
saw the 유술 payoff (Valhalla stun damage, no ragdoll) land while no kill move was visible.

For test 12, NPC grapples are off: `mods\Grapple\SKSE\Plugins\FH_Grapple_Plugin.ini`
`bEnableNPCGrapple = false` (the user's temporary choice; the original INI is kept at
`build\FH_Grapple_Plugin.ini.bak_20260919_npcgrapple`). The DLL reads the key through the wide INI
API; the MCM has no NPC-grapple switch.

Open: a payoff without a visible kill move means the payoff can fire on a KillMoveEnd that is not
from the 유술 play; check against a clean log once NPC grapples are off.
