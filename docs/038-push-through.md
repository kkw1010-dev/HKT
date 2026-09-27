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

### The probe (`src/PushProbe.*`, author build `f9fd8e0`, deployed)

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
