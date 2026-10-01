# 045 · Cleaving Attack: research (no code)

Status (2026-10-01): research in progress. The brief is the user's
(`C:\TAKEALOOK\_codex\work\cleave\BRIEF.md`, with the amendment that avoiding Precision has low weight).
Sections 1, 4 and 8 (and parts of 2, 7, 9, 10) are this session's work, checked against CIGAR's source,
CommonLibSSE-NG's headers and the load order. Sections 2, 3, 5 and 6 wait for Codex's CX-20 (melee
internals, Precision, vanilla Sweep) and CX-21 (animation); what is written there now is only what this
session confirmed itself. "Confirmed" means read in source or data; "inferred" is marked.

## 1. CIGAR architecture relevant to Cleave (confirmed, `src/`)

- **A module** is a singleton deriving from `Module` (`Module.h`), listed in `Modules()` (`main.cpp`), with
  a `Label` in `Panel.cpp` (no `needs` → combat page needs its name in `kCombatModules`). Switch on/off is
  `Settings::Enabled(name)`; `OnDisabled` must undo anything that outlives a prompt.
- **Ticks:** `FastTick` every 100 ms and `Tick` every 1 s, on the game's task thread, only while a game is
  loaded, unpaused, no blocking menu, no SKSE Menu Framework window and no BiS wash. A cleave gate
  belongs in `FastTick` (Jujutsu, Execute and Squeeze gate there).
- **Prompt:** one `PromptSlot` per prompt with its own `PromptID` (next free: 44). Combat prompts are
  single press; `tools/check_prompt_rules.py` fails the build unless the slot is in its `COMBAT` list.
  A decline hides the prompt until its condition has been false for 1 s or its `SetSituation` key changes
  (the key should be the cleave group, e.g. a hash of the member FormIDs). `OnAccepted` runs on the game
  thread.
- **Only four key slots** (`Settings::kPromptKeyCount`, keys 2-5 by default). In a fight these can already
  be up at once: 근접/원거리 무기, 록온, 그래플, 유술, 처형, 물약, 독, 투구 쓰기, 항복. A fifth prompt gets no slot
  (`AcquireKeySlot` returns -1) and is sent with no CIGAR key. Cleave therefore competes for a slot; there
  is no priority mechanism today (first offered, first served).
- **Combat utilities:** `Util::NearbyHostiles(actor, radius)` (living, 3D-loaded, hostile, high-process
  actors, nearest first), `Util::InScene`, `Util::IsBusy`; Squeeze's `Nearest()` already computes
  "ahead / to the side" of the player's heading for every high-process actor; Jujutsu's `MidAttack`
  (attack state or graph `IsAttacking`) and its `attackStop` before starting; WeaponSwap reads the worn
  weapon types; TDM's API gives the lock target (`TDM_API`, WeaponSwap and LockOn).
- **Starting an action on the player:** Jujutsu plays IDLE forms with
  `AIProcess::SetupSpecialIdle(player, kActionIdle, idle, true, false, target)` and needed retries because
  the graph refuses while `IsAttacking`; Execute presses another mod's key (`Util::PressKey`); Rest and
  Squeeze send graph events (`NotifyAnimationGraph`).
- **Events:** `BSAnimationGraphEvent` sinks on the player (Rest, PushProbe), `TESContainerChangedEvent`
  (ItemEquip, BookRead), hooks on three anim handlers (Jujutsu: KillActor, KillMoveStart, KillMoveEnd),
  a vtable hook on `PlayerCharacter::Update` (PromptAnchor), a call hook on Present (Prompt).
- **Clashes with existing modules:**
  - Jujutsu (a blocking humanoid in reach) and Execute (a stunned enemy) can hold at the same moment as
    a cleave group: three single-press prompts on three keys. Not wrong, but the brief's "the situation is
    the UI" argues for cleave yielding when the primary target is executable or guard-broken.
  - WeaponSwap changes the weapon; cleave must re-read the weapon on accept.
  - Potion in combat is a single press and takes a slot when health is low.
  - Squeeze, Rest, Observe are non-combat and never up with it.
  - Jujutsu's hooks swallow KillActor/KillMoveEnd for its victim only; a cleave swing that would
    kill-move a victim does not pass through them (different actor), but a kill move started by the
    engine on the cleave swing would lock the player into a paired animation: the brief's edge case 16.

## 2. Vanilla capabilities to reuse

- **Confirmed (load order, houseCARL):** perk `Sweep` (Skyrim.esm 03AF9E, winner ParagonPerks.esp) is one
  entry point effect, `SetSweepAttack` = 1 ("Sideways power attacks with two-handed weapons hit all
  targets in front of you"), with a condition on the perk owner tab and three on the weapon tab.
  CommonLib has the entry point as `BGSEntryPoint::ENTRY_POINT::kSetSweepAttack` (50).
- **Confirmed (headers):** `BGSAttackData` carries `attackAngle` and `strikeAngle` and a `kPowerAttack`
  flag; `Actor::GetReach()` and `TESObjectWEAP::GetReach()` exist; `HitData::Populate(aggressor, target,
  weapon)` exists.
- **H1, state:** partly confirmed. The entry point exists and is a plain "set to 1". What the engine does
  with it for a one-handed weapon, and whether CIGAR can make it true for one swing without shipping a
  perk record (CIGAR is ESP-less), is CX-20's question. Pending.

## 3. Proven external implementations

Pending CX-20. Confirmed here: **Precision is installed in this modlist** (`mods\Precision - Accurate
Melee Collisions`, enabled, with creature patches), so in the user's game melee hits already come from
weapon-trajectory collision, whatever the vanilla sweep flag says.

## 4. Proposed detection model (this session's comparison, section 21-E)

All five were weighed for a gate that runs every 100 ms on at most a dozen actors.

| Model | What it tests | Cost | False positives | Verdict |
|---|---|---|---|---|
| Cone (angle from facing, max range) | each hostile within R and within ±θ of the heading | trivial | accepts two enemies at opposite edges of a wide cone, and a far one behind a near one | too loose alone |
| Arc band (cone plus a min/max range band) | as cone, but both within one reach band | trivial | fewer; still ignores how far apart the two are | basis |
| Capsule along the heading | hostiles inside a forward box/capsule | trivial | built for a thrust, not a sweep: rejects side-by-side enemies | wrong shape |
| Weapon trajectory (node positions over the swing) | the blade's swept surface against actor bounds | high, and only known during the swing | lowest | right for the HIT, useless for the PROMPT (nothing swings yet) |
| Primary target plus secondaries | take the enemy most in front (or TDM's lock target), then others within a lateral gap of it and inside the arc band | trivial | low; matches "those two are standing together" | **recommended**, combined with the arc band |

Recommended gate (values are starting points to be tuned in play, not the user's):

1. Player: in combat, weapon drawn, a weapon class from the brief's "strong" list in the right hand,
   not mid-attack, not staggered, not in a kill move, on foot, third or first person.
2. Candidates: `NearbyHostiles` within reach R = the player's `GetReach()` plus a margin; humanoid or
   not is decided later (edge case 9), dead/bleeding-out/essential-down excluded.
3. Primary: the candidate with the smallest angle to the player's heading, within ±35°; if TDM is locked,
   its target when it qualifies.
4. Secondaries: candidates within the arc (±60° of the heading), within R, and within a lateral gap of
   the primary (about one and a half body widths, 120 units), with a height difference under 60.
5. Group = primary + secondaries; the prompt shows only for a group of two or more that has held for
   0.3 s (as Squeeze's contact), and the group's FormIDs are the decline situation key.
6. Fail-safe: anything unknown (no reach, no 3D, a friendly actor inside the arc between the player and
   the group) → no prompt. No rotation or movement of the player, ever.

The gate's inputs go to the log as a rounded `trace` line, like Squeeze's, so thresholds can be replayed
offline.

## 5. Proposed hit model

Pending CX-20. The two candidates from the brief's hypotheses:
- **H2 (Precision present):** if Precision hits every actor the blade passes through, once each, the hit
  model is "start a broad swing and let Precision resolve it"; CIGAR adds nothing to damage. To be
  confirmed from Precision's source or API.
- **Without Precision:** the vanilla sweep flag (H1) if it can be set for one swing, else no cleave
  (prompt off), given the amendment's low weight on Precision independence.
Not proposed: a damage sphere, calling damage functions per actor, or any multiplier (brief 8, 23).

## 6. Proposed animation strategy

Pending CX-21. Confirmed here: the user's player combat animations are replaced through OAR (For Honor
sets in combat, `player-oar-roles`), so a vanilla sideways power attack triggered by CIGAR will play
whatever clip the user's OAR setup maps to it (H3 stands). CIGAR's two proven ways to start a player
action are a graph event and `SetupSpecialIdle` with an IDLE form.

## 7. Compatibility risks (so far)

- Key-slot pressure in fights (section 1).
- Precision's presence changes the hit model; its absence must degrade to "no prompt", never to fake
  damage.
- Attack-speed and slow-time mods change swing timing; anything timed by CIGAR (not by the animation)
  would drift. Prefer animation events over timers.
- Other mods that start power attacks from keys (one-click power attack mods) may fight over the same
  action; to be checked against the load order when the trigger is chosen.

## 8. Edge cases (brief 17), first reading

| # | Case | Proposed handling |
|---|---|---|
| 1 | Essential / protected | In the group; the engine's normal hit rules apply (no kill) |
| 2 | Friendly in the arc | No prompt if a non-hostile actor stands between the player and the group or inside the arc (fail-safe); with Precision a friendly would really be hit |
| 3 | Neutral NPC | As 2 |
| 4, 5 | Summons, followers | As 2 (they are friendlies) |
| 6, 7 | Dead, dying, bleedout | Not candidates |
| 8, 9 | Large actors, non-humanoids | Prototype: humanoids only (brief 19); later by race size |
| 10, 12 | Doorway, wall beside | The gate cannot see walls cheaply; with Precision the blade hitting a wall is Precision's business (recoil). Accept for the prototype, log it |
| 11 | Stairs, height | Height difference limit in the gate |
| 13 | Overlapping NPCs | Both in the group; one hit each is the hit model's job |
| 14, 15 | Stagger of either side | Player staggered → no prompt; a swing interrupted by stagger simply ends |
| 16 | Kill move available | Unknown: whether the engine may start a kill move from this swing. To be answered by CX-20 |
| 17, 18 | Attack speed, slow time | No CIGAR timers in the swing |
| 19 | Dual wield | Out of the prototype (brief 10) |
| 20 | Reach modifiers | The gate uses the actor's reach, not a constant |

## 9. Smallest viable prototype (draft, to be fixed after CX-20/21)

Author build only: a `Cleave` module with the section 4 gate for two hostile humanoids, a single-press
prompt, and on accept one sideways power attack started the cheapest proven way, with Precision resolving
the hits. The log carries the gate trace, the swing start, and each `TESHitEvent` the player causes within
the swing (target, once-per-actor check), so the ten acceptance criteria of brief 20 can be judged from
the log.

## 10. Files expected to change (draft)

`src/Cleave.h/.cpp` (new), `src/Prompt.h` (PromptID), `src/main.cpp` (`Modules()`), `src/Panel.cpp`
(label, `kCombatModules`), `tools/check_prompt_rules.py` (`COMBAT`), `docs/`, `README.md`, `CHANGELOG.md`,
`TEST-next-ingame.md`. No change to existing modules for the prototype.

## 11. What remains uncertain

- H1: can the vanilla sweep be had for one swing without a perk record, and for one-handed weapons?
- H2: does Precision hit several actors in one swing by itself, once each, for any attack?
- How to start a sideways power attack on demand while standing or moving forward (the vanilla direction
  comes from movement input), and what the user's OAR setup plays for it.
- Whether the engine can turn the swing into a kill move.
- Prompt priority among combat prompts with four key slots.

## 12. Recommendation

Pending CX-20/21. Provisional: detection by primary plus secondaries inside an arc band (section 4),
hits left to Precision when present and no prompt otherwise, a vanilla sideways power attack as the
swing; prototype in the author build with log-only verdicts.
