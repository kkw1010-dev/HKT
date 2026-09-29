# 038 · NPC push-through (review of an outside plan, 2026-09-28)

**Status: stage 0 (probe build and clip candidates) done 2026-09-28; waiting for the in-game probe (P1-P5).** A GPT-written brief ("CIGAR NPC Push-Through Interaction v0.1",
`Downloads\# CIGAR NPC Push-Through Interactio.txt`) proposes a hold prompt that lets the player
push past a non-hostile NPC with an arm or shoulder, the NPC reacting with the vanilla bump. This
file records the review and its evidence so the next session does not re-derive it.

## Verdict

Adopt the objective, change the architecture. The brief's fixed goal stays: while the prompt is held,
the Dragonborn visibly pushes or slips past a non-hostile humanoid, and walking goes on. Two of the
brief's assumptions do not hold, and a part already in CIGAR solves its hardest problem.

## Evidence

### Player motion: the Offset Movement Animation layer, already driven by CIGAR

- `Offset Movement Animation - Nemesis - Modders Resource` (Nexus 110408, by Xing and GiraPomba)
  adds a looping animation that blends over walking, sneaking, a drawn weapon, riding and sitting.
  Start with the graph event `OffsetGPMA`, stop with `OffsetGPMAStop`; OAR picks the clip.
- `iGPMAOffsetType` = 1 plays the clip on the right arm only, 2 on the left arm only; the game keeps
  the other arm. That is the brief's Push_Right / Push_Left.
- `bGPMAInstalled` is always true when the behaviour patch is applied: a runtime check for a soft
  integration.
- CIGAR's Helmet module already uses it (`iGPMAAnimationType`, `OffsetGPMA`, `OffsetGPMAStop`;
  `docs/028-helmet.md`). The event and variable are in this modlist's Pandora output, so **no
  behaviour edit and no Pandora run** is needed (a run is forbidden here, see HANDOFF, Jujutsu).

### EVG Squeeze is not a locomotion clip

- `EVG Animated Traversal` (Nexus 63232, 2.1, installed) ships Squeeze as
  `OpenAnimationReplacer\EVG Animated Traversal\Squeeze\mt_leverfloorpull.hkx`: the vanilla floor
  lever pull replaced through OAR, condition `CurrentFurniture` = `EVGAnimatedTraversal.esl` 0x812.
  The movement is added by Animation Motion Revolution. It is a full-body furniture animation
  (SSE packfile, `hk_2010.2.0-r1`, 31,152 bytes).
- That is the opposite of the brief's section 10 (lower body keeps the game's locomotion). Using it
  means the whole HKX edit pipeline (section 24 D), and CIGAR ships no other mod's files, derived
  ones included. The EVG author allows reuse "however you want" when EVG is a requirement, but a
  derived clip inside CIGAR's package still breaks the asset-free rule.

### NPC reaction: the vanilla bump exists and is addressable

- `HighProcessData`: `bumpedState` (`BUMP_TYPE`: kNone, kSmall, kBig), `bumpTimer`,
  `lastBumpDirection`.
- Condition functions `IsSmallBump` (274) and `GetLastBumpDirection` (642); default object
  `kActionBumpedInto` (90) in `BGSDefaultObjectManager`.
- Walking into an NPC already gives a small bump; sprinting gives a big one. The brief's section 2.2
  ("only sprint displaces") is half right: the missing parts are the reaction's strength and the
  player's gesture.
- `TESActionData::Create()` and `Process()` are in CommonLib, so performing `ActionBumpedInto` on an
  NPC can be called. **Whether it plays the big bump and moves the NPC is unverified.** Writing
  `bumpedState` directly is engine internals and is not proposed.

## Reused parts

| Need | Part |
|---|---|
| Upper-body clip while walking, one arm | Helmet's `OffsetGPMA` call and its log line |
| Hold that lasts while held | Observe's `kHoldAndKeep` prompt |
| Humanoid, hostility, scene gates | Jujutsu's target checks, `Util::InScene` |
| Third-person prompt position | PromptAnchor |
| Soft integration | `bGPMAInstalled`; the module waits without it (page 3, Mod integrations) |

## Failure cases and countermeasures

- Prompt near every NPC (walking up to talk): show it only when **blocked**: movement input, speed
  collapsed for about 0.3 s, a non-hostile humanoid within about 70 units in a narrow forward cone.
- Several NPCs in a row: SkyPrompt needs a visible prompt to press, so it appears when blocked; while
  held (`kHoldAndKeep`) it stays as long as a non-hostile humanoid is near, so the next NPC needs no
  new press. Release ends it.
- NPC pushed with no gesture (section 7.4, "invisible force"): without `bGPMAInstalled` the module
  waits; if `OffsetGPMA` is not accepted, no bump is sent that time.
- Crime or hostility: never reuse Jujutsu's `KnockExplosion` or a stagger; only the vanilla bump,
  which sprint collisions already use without crime. The probe logs hostility and crime.
- Bump loop: one bump per NPC, then a cooldown (about 1.5 s).
- Combat, drawn weapon, mounted, swimming, seated, first person: hidden (non-combat, third person
  first, as in the brief's section 30).

## Open decisions (the user's)

- The clip. A: a vanilla clip that already has a one-arm gesture, played through GPMA with no edit
  (look for candidates with `Toolchain\tools\hkx_preview.py`). B: a clip made or licensed by a human
  animator, shipped as CIGAR's own asset (the package allowlist would grow an OAR folder).
  C: an EVG-derived clip, only as a separate optional add-on. D: generated (the brief advises
  against it).
- Bump strength: small or big.
- Whether the NPC's bump line ("Watch it!") is welcome, once the probe shows whether it plays.
- Distance, blocked time and cooldown: starting values above, to be recorded as the user's choice.
- The prompt name.

## Next step, when the user says go

One log-only probe build: an author-panel button that performs `ActionBumpedInto` on the NPC under
the crosshair and logs `Process()`, `bumpedState`, the NPC's position before and after, hostility and
crime. In parallel, a list of vanilla one-arm clip candidates from `hkx_preview.py`. Both decide GO
or HOLD before any module code.

## Stage 0 (2026-09-28): probe build and clip candidates

The user said go (through the orchestrator). Nothing below offers a prompt.

### The probe (`src/PushProbe.*`, author build `cc0b6e2`, deployed)

- Author panel, `5. 세부 설정` → "탐침: NPC 밀기 (docs/038)". A button arms the probe; it fires on the
  NPC under the crosshair when the menu closes, so the measurement is not paused with the menu.
  - **행동 ActionBumpedInto:** `TESActionData::Create()`, source = the NPC, target = the player,
    action = default object 90, `Process()`.
  - **앞 / 뒤 / 왼쪽 / 오른쪽:** the graph events of the IDLE children of `BumpedIntoRoot`
    (Skyrim.esm 03DE4E), sent with `NotifyAnimationGraph`: `NPC_BumpFromFront`,
    `NPC_BumpedFromBack`, `NPC_BumpedFromLeft`, `NPC_BumpedFromRight`.
  - **자연 부딪힘 관찰:** every 100 ms, the high-process actors within 400 units; a change of
    `bumpedState` opens the same measurement, so walking and sprinting bumps are logged as the game
    makes them.
- Each bump is followed for 3 s: samples at 0.25 / 0.5 / 1 / 2 s, the NPC's animation graph events
  (up to 40), dialogue lines within 1,500 units (`TESTopicInfoEvent`, with the topic and its
  subtype), hostility, combat and the player's crime gold in the NPC's crime faction. It ends with
  `RESULT ... DISPLACED | NOT DISPLACED` (20 units or more at any sample) and `PROBLEM:` when
  hostility, combat or crime appeared.
- Read only: `HighProcessData::bumpedState`, `lastBumpDirection`, `bumpTimer`. No engine value is
  written.
- **Small versus big.** The IDLE tree has no small/big split: `ActionBumpedInto` (03DE4D) →
  `BumpedIntoRoot` → four direction idles chosen by `GetLastBumpDirection` on the NPC. The engine
  keeps the strength in `bumpedState`; the watch mode shows what walking and sprinting set, and the
  results show what each one does to the NPC.
- In-game items: `TEST-next-ingame.md` P1-P5.

### One-arm clip candidates (vanilla, no edit)

Preview page: `C:\TAKEALOOK\_staging\push-clip-candidates\index.html` (hkx_preview, GT Softbody's
female skeleton). Numbers from `arm-metrics.json`: how far each hand moves and reaches forward
(+y), in units, root motion removed. GPMA plays one arm with `iGPMAOffsetType` 1 (right) or 2 (left),
so the side of the clip must match.

| Clip | Length | Arm | Reach forward | Fit |
|---|---|---|---|---|
| `mt_activatedoor` (same numbers as `mt_activatepickup`) | 2.7 s | left | 35 | best left push: an open hand pushed forward |
| `shd_blockbash` | 0.37 s | left | 62 | a fast shove; strong, may read as aggressive |
| `idletake` | 2.4 s | right | 44 | right reach; calm |
| `idlegive` | 2.4 s | right | 38 | right reach; calm, hand turned up |
| `mt_pointclose` | 2.2 s | left | 27 | a point, not a push |
| `idlewave` | 2.1 s | right | 30 | a wave; "excuse me" at most |
| `dialogueneutralsubtled` | 5.0 s | right | 15 | small talk gesture, too weak |
| `h2h_attackpowerforwardlefthand` | 1.7 s | both | 46 | a punch; rejected |

No single vanilla clip mirrors: a right push and a left push come from different clips
(`idletake` right, `mt_activatedoor` left). The clips are Bethesda's; shipping a copy in an OAR
folder would add files to the package allowlist (the user's call, open decision B/A above).

## The user's decisions (2026-09-28, through the orchestrator)

Recorded as the user's choices; the numbers were the review's starting values and the user took them.

- Clips: compare **A** (vanilla one-arm clips, no edit) and **C** (EVG Squeeze, from the user's own
  install) in game, author build only.
- Bump strength: **small**.
- The bumped NPC's line: **not allowed** for now.
- Blocked detection: a non-hostile humanoid within **70** units in the forward cone, speed collapsed
  for **0.3 s**; one bump per NPC, then **1.5 s** cooldown.
- Prompt name: **"비켜 지나가기 (누르고 있기)"**; English "Squeeze Past (hold)" (Claude's wording).

## Must a vanilla clip ship in an OAR folder? (answered 2026-09-28)

Yes, as a renamed copy, or not at all:

- **How the clip is played.** Offset Movement Animation plays one clip file,
  `meshes\actors\character\animations\GPMAOffsetAnimation.hkx` (its mod ships a default). The event
  `OffsetGPMA` starts it and `OffsetGPMAStop` ends it. What changes the motion is an OAR submod that
  replaces **that file** while its conditions hold. CIGAR's Helmet does exactly this: `CIGAR - Helmet
  Motions` holds `OpenAnimationReplacer\CIGAR Helmet\Helmet Unequip\Actors\Character\Animations\
  GPMAOffsetAnimation.hkx` (a copy of Helmet Toggle 2's clip) with the condition
  `iGPMAAnimationType == 2`; CIGAR sets the variable, then sends the event.
- **OAR binds a replacement by path and file name.** A submod's `.hkx` must mirror the original's path
  (`character/GPMAOffsetAnimation.hkx`). `overrideAnimationsFolder` only lets a submod use **another
  submod's** files, which must themselves be named `GPMAOffsetAnimation.hkx`. There is no key that
  maps a differently named file (`mt_activatedoor.hkx`) or a path inside a BSA onto the original.
  (OAR 3.0.0 schema, `housecarl:open-animation-replacer` §1-3. Whether OAR can read a submod packed in
  a BSA was not checked; it does not matter, because the name still has to be the GPMA one.)
- **Not shipping a file** would take CIGAR extracting the clip from `Skyrim - Animations.bsa` at run
  time into its own OAR folder. OAR reads its submods when the game starts, so the first launch would
  have no clip, and CIGAR would need a BSA reader and would write into the game's Data folder (MO2's
  overwrite). Possible, not worth it.
- **Shipping a copy.** It is a Bethesda file reused in a mod for the same game, which Skyrim mods on
  Nexus routinely do (animation replacers ship vanilla-derived clips); the current Nexus and Bethesda
  wording was not re-read for this answer. It changes CIGAR's package: `make_release.py`'s allowlist
  grows an OAR folder with a `config.json` and one `GPMAOffsetAnimation.hkx` per clip. The Helmet
  clips never shipped because they are another mod's; a vanilla clip is not.
- **The EVG route (C)** cannot point at EVG's files either: its Squeeze file is named
  `mt_leverfloorpull.hkx` and replaces the lever-pull, so a test copies it into CIGAR's own submod as
  `GPMAOffsetAnimation.hkx`. That copy is for the user's game only; it never ships.

## A/C comparison test plan (stage 0b)

- **Assets** (author build only, inside `mods\CIGAR`, never packaged): an OAR mod `CIGAR Push Test`
  with one submod per candidate, each a `GPMAOffsetAnimation.hkx` copy keyed on a CIGAR-only
  `iGPMAAnimationType` value. Helmet Toggle 2 uses 1-12 and CIGAR's Helmet 2 and 6, so the test takes
  3801-3806:

  | Value | Clip | Source | `iGPMAOffsetType` |
  |---|---|---|---|
  | 3801 | `mt_activatedoor` | vanilla (A) | 2, left arm |
  | 3802 | `shd_blockbash` | vanilla (A) | 2, left arm |
  | 3803 | `idletake` | vanilla (A) | 1, right arm |
  | 3804 | `idlegive` | vanilla (A) | 1, right arm |
  | 3805 | EVG Squeeze | user's install (C) | 0, both arms and upper body |
  | 3806 | EVG Squeeze | user's install (C) | 2, left arm |

- **Probe buttons** (PushProbe, author panel): one per row. Arming waits until the player moves, then
  sets `iGPMAOffsetType` and `iGPMAAnimationType`, sends `OffsetGPMA`, and sends `OffsetGPMAStop` after
  the clip's length (2.7 / 0.4 / 2.4 / 2.4 s; the Squeeze length is read from the file). Logged: GPMA
  installed (`bGPMAInstalled`), each variable set, the event accepted, the player's graph events
  during the clip, the stop, and whether the player kept moving (speed before, during, after).
- **In-game items** (for the checklist): walk past an NPC and fire each row once; the user compares by
  eye. The log only proves each clip played on a moving player and stopped; which reads as "squeeze
  past" is the user's judgement.

## Stage 0 results (r5, the user's run of 2026-09-28 17:42-18:34, `CIGAR.log`)

One session: a new game started at 17:51:57 (no save was loaded in this log). All P and G items were
run; the user picked **3805 (EVG Squeeze, upper body)**.

### NPC side: GO with the direct graph events; ActionBumpedInto is dropped

| Route | Result |
|---|---|
| `ActionBumpedInto` through `TESActionData` (P4) | `Process()` returned **false**; nothing moved, no events. The user's "not sure it worked" is right: it did nothing. Dropped |
| Graph events `NPC_BumpFromFront` / `...FromBack` / `...FromLeft` / `...FromRight` (P5) | Accepted 9 of 15. Every accepted one **displaced** the NPC 50-84 units, no hostility, combat or crime, and **no dialogue line** |
| Natural walking bump (P2, engine `small`) | Displaced in most windows (75-198 units, partly the NPC's own walking), not in some (0-20 units) |
| Natural sprint bump (P3, engine `big`) | Displaced 51-63 units |

- **Refusals:** all six refused events were on 미카엘 while he was in dialogue or his bard scene (head
  tracking, `IdleDialogueLock`, scene lines of subtype 14). 신미어 accepted all four. The behaviour graph
  refuses the event in those states; nothing else was touched.
- **NPC Animation Remix 2.1.0** ships no `bump_*` clips (71 OAR submods, none on the bump clips), and an
  OAR replacement cannot make a graph refuse an event. It is not the cause of the partial result.
- **Dialogue.** Natural bumps made the NPC talk: 12 `ActorCollidewithActor` lines (subtype 98, one from a
  mod topic `JB1CDORearBump`), plus hello and idle lines. The direct events produced none (14 of 15
  windows; the other was 미카엘's scene line). That meets the user's "no NPC line" by construction.
- **Small or big.** The direct events play the same four bump idles the engine uses for both; the
  displacement is the idle's own (50-84 units), close to the natural ones. There is no separate small
  or big version to choose.

### Player side: GO with EVG Squeeze; the combination below needs one choice

- Every gesture started while walking, with `bGPMAInstalled`, both variables set and `OffsetGPMA`
  accepted, and stopped with both variables back at 0. Movement kept in all but the first 3801 run,
  which is a false alarm (the player was just starting to move, speed before 0).
- **Probe fault found:** arming a new gesture while one still plays dropped the old one without its
  stop or result line (3803, 3804). It did no harm (the next start overwrote the variables); the next
  probe build stops the running gesture first.
- **What the Squeeze clip does** (decoded, stripped copy): the shoulder line turns about 100° within
  0.5 s, holds until about 1.9 s, and is back at 2.67 s. The left hand leads (up to 45 units ahead),
  the right hand trails (up to 28 behind). 3805 and 3806 are the same clip with different masks: 3805
  plays both arms and the upper body, 3806 the left arm only.

### Combining 3805 and 3806 (the user asked for it)

Offset Movement Animation plays one clip at a time with one of three masks (both arms and upper body,
right arm, left arm). There is no "upper body plus left arm" mask, and this machine can decode `.hkx`
but not write animation tracks yet. Three ways, cheapest first:

1. **3805 cut short** (no file edit): stop it early, while the turn is half done (about 50° at 0.3 s),
   so the body shoulders past instead of turning fully sideways, then the layer blends back. Values to
   compare: 0.3, 0.5 and 0.8 s.
2. **Pick by the gap** (no file edit): the NPC straight ahead (a tight gap) gets the full 3805; an NPC
   standing off to the left gets 3806 (the left hand leads, no turn). An NPC off to the right has no
   mirrored arm-only clip: 3805 there too, or no gesture.
3. **A real blend** (a new clip): the Squeeze turn scaled down plus the left-arm lead, the right arm left
   free. It needs an `.hkx` writer that is not on this machine (docs/038 research item D) and possibly a
   new mask in the behaviour patch (a Pandora run, which this modlist avoids). HOLD.

Recommendation: test 1 and 2 together in one short round (a probe build with three cut lengths and the
gap rule), then build the module on the user's pick. The NPC side is settled (direct events, skip NPCs
in dialogue, scenes or furniture).

## Stage 0d results (r8b, the user's run of 2026-09-29 19:43-20:31, `CIGAR.log`)

Probe build `7822eab` (in the deployed `e277228`): after contact, (가) halves the player's controller
capsule, (나) sets the no-collision flag on that NPC's controller body; either is undone once the
player is 30 past and 70 away from the NPC, or after 3 s. All runs were in the Bannered Mare; every
contact was found by distance (`no controller bump reported`), never by the controller's bump field.

| Item | The user | Log |
|---|---|---|
| Q1 (가) open floor | fail | 2 runs on 미카엘, DID NOT PASS (one POPPED 53 against 11 expected after the undo) |
| Q2 (나) open floor | pass | PASSED, no pop, the NPC did not move |
| Q3 (가) door frame | pass | PASSED, but nothing was changed (below) |
| Q4 (나) door frame | pass | PASSED |
| Q5 wall gap | fail, "only (나) worked" | (가) DID NOT PASS; (나) first try DID NOT PASS (side contact at -59, 3 s limit, max +6 past), then 3 of 3 PASSED |

- **(가) never did anything.** Every (가) start logged `shape0 type 9 (not a capsule, left alone)` for
  both shapes: the player controller's shapes are not `hkpCapsuleShape`, so nothing was shrunk. Its
  two "passes" were NPCs standing off to the side. That matches the user's note ("(가) never
  worked, so I can't judge it"). **Dropped from the probe** (2026-09-29, the orchestrator relaying
  the user).
- **(나): 6 of 7 passed.** No pop after any undo (moved 50-58 in 0.6-0.7 s against 47-59 expected),
  no stuck time except 0.3 s in the one failure, and the NPC's z stayed within ±2 (no fall through
  the floor). The one failure grazed the NPC at 59 to the side: "past" is measured along the
  player-to-NPC line at the start, which is diagonal to the walk for a side contact, so the player
  moved 116 while "past" stayed at +6 and the 3 s limit ended it.
- **The gesture.** Started while already touching, 3805/3806 kept the walk (KEPT MOVING in all (나)
  runs). Started from 70-80 away at a run, 3805 dropped the speed (28 of 83; the first (가) runs).
- **The user picked (나)** (SQ-PICK, 2026-09-29).

## Stage 1 plan: (나) as the feature (built 2026-09-29, `src/Squeeze.*`, not yet tested in game)

A new module `Squeeze` (non-combat page); `PushProbe` stays in the author build as the probe.

- **Prompt.** "비켜 지나가기 (누르고 있기)" / "Squeeze Past (hold)" (the user's name, 2026-09-28), shown
  when blocked: moving input, a non-hostile humanoid within 70 units in the forward cone, speed
  collapsed for 0.3 s (the user's values). Type `kHoldAndKeep` with the same 0.5 s ring as Deflate
  (`docs/042`): a tap does nothing, the double tap declines (hidden until the condition has been false
  for 1 s). While held, each NPC touched in turn is squeezed past without a new press.
- **What it does per NPC.** Set the no-collision flag on that NPC's controller body (the r8b code,
  `Ghost`), play the bump graph event matching the side (`NPC_BumpFrom*`, r5: displaces 50-84, no line,
  no hostility), and the gesture by the gap (3805 ahead or right, 3806 left) only when already within
  touching distance, so the walk keeps its speed.
- **Undo.** Restore the filter word once the player is 30 past and 70 away, with "past" measured
  along the player's heading, not the start line (the Q5 failure). The time limit becomes "3 s with no
  forward progress" instead of 3 s flat. Also restore on release, decline, combat, a menu, cell change,
  load and the module being switched off. Only one NPC is ghosted at a time; a new one first restores
  the previous.
- **Skips.** NPCs in dialogue, a scene or furniture (the graph refuses the bump there, r5), hostile or
  in combat, mounted, and anything not humanoid.
- **Self-reporting.** One start and one undo line per NPC with the filter word before and after;
  after every undo the module reads the word back and logs a WARN (one notification per session) if it
  is not the old value. `verify_deploy.py`: the module's panel toggle and prompt ID. The PushProbe
  `RESULT` line format is kept so the r8b numbers stay comparable.
- **In game (r10):** open floor, door frame, wall gap with a side contact, two NPCs in a row on one
  hold, a tap (nothing), a decline (hidden while blocked).

Open for the user before the public build:
- **The gesture clip.** 3805/3806 are EVG Animated Traversal's Squeeze copied from the user's own
  install; CIGAR never ships another mod's file. The public build needs a clip CIGAR may ship (a
  vanilla one, option A) or no gesture; the author and personal builds can keep the EVG copy.

### As built (2026-09-29)

The plan above, with these values and differences (each logged, so r10 can check them):

- **Blocked:** moving input, the nearest actor ahead within **80** units and **50** to either side,
  speed under **50** u/s, for **0.3 s** (the user's). The user's starting reach was 70; r8b measured
  contacts at up to 73 ahead, centre to centre, so 80 is mine and flagged to the user. Corridor and
  speed are mine.
- **Hold:** `kHoldAndKeep` with key down and up; nothing changes until the key has been down 0.5 s
  (`ring filled` in the log). While held, every NPC met in the corridor is squeezed past, one at a
  time; the next one first lets the previous go.
- **Release does not restore the NPC being passed.** Restoring while the two overlap pushes them
  apart (the probe's POPPED); that NPC is let go by its own rule instead: 30 past along the player's
  heading and 70 away, no forward progress for 3 s, a 10 s cap, the NPC gone, a cell change, combat,
  the module switched off or a load. The log's `STILL OVERLAPPING` marks an undo closer than 40.
- **Skips** (logged as the gate's `npc=- (<name>: <why>)`): dead, not humanoid, hostile or in combat,
  mounted, in a scene, talking to the player, in furniture, in a SexLab/OStim scene. The player: combat,
  weapon drawn, seated, swimming, mounted, first person, a scene.
- **Bump:** the side the player comes from, relative to the NPC's heading; once per NPC with a
  1.5 s cooldown.
- **Gesture:** author build only until the user decides D17 (the public clip): 3805 straight ahead or
  on the right, 3806 on the left, full 2.87 s, skipped when Helmet's clip is playing on the same layer or
  Offset Movement Animation is missing. The release build has no gesture and ships no clip.
- **Self-reporting:** `start:` with the filter word before and after and whether the no-collision bit
  stuck (if not, the pass ends at once); `RESULT squeeze on <npc> (<why>): PASSED | DID NOT PASS` with
  the filter restored and read back; a WARN and one HUD notice per session if the read-back differs.
