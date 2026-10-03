# 045 · Cleaving Attack: research (no code)

Status (2026-10-01): research in progress. The brief is the user's
(`C:\TAKEALOOK\_codex\work\cleave\BRIEF.md`, with the amendment that avoiding Precision has low weight).
Sections 1, 4 and 8 (and parts of 2, 7, 9, 10) are this session's work, checked against CIGAR's source,
CommonLibSSE-NG's headers and the load order. Sections 2, 3 and 5 use Codex's CX-20 (melee internals, Precision, vanilla Sweep),
re-checked against Precision's source and this modlist's settings; section 6 and the final recommendation
still wait for CX-21 (animation). "Confirmed" means read in source or data; "inferred" is marked.

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
- **H1, state: plausible, not proven, and not needed with Precision.** Codex CX-20
  (`C:\TAKEALOOK\_codex
esults\CX-20.md`) and this session's check of the sources:
  - The perk covers sideways power attacks with two-handed weapons. `BGSAttackData` has no sweep flag
    (confirmed, header). An older LE mod claims the same entry point works for one-handed plus shield
    (author's page, not source).
  - Calling `HandleEntryPoint` from CIGAR does not make the engine's later hit query true; it would take
    a perk record (CIGAR ships none) or a hook on the entry-point result, whose call site is not known.
  - Precision cancels it anyway: `AttackHooks::ApplyPerkEntryPoint` sets the `kSetSweepAttack` result to 0
    while the actor has an active collision (confirmed, `src/Hooks.cpp` lines 174-181).
- **Idle forms for the swing (confirmed, houseCARL, Skyrim.esm):** `PowerAttackLeft` 00019B22
  (`attackPowerStartLeft`), `PowerAttackRight` 00019B24 (`attackPowerStartRight`), `PowerAttackStanding`
  00019B26, `PowerAttackForward` 00019B25, under `PowerAttackRoot` 00013384.

## 3. Proven external implementations

- **Precision (GPL-3.0, open source; installed here as 2.0.4).** Confirmed in its `main` source:
  - Weapon collisions start on the attack animation's events; each contacted actor is queued for the
    game's own damage routine, and an actor already hit by that collision is rejected (CX-20's reading
    of `PrecisionHandler.cpp`, `ContactListener.cpp`, `PendingHit.cpp`; this session re-read the target
    cap, `ContactListener.cpp` lines 419-427, and `Utils::IsSweepAttackActive`).
  - **Target cap.** `uSweepAttackMode`: 0 unlimited (the source default), 1 max targets, with
    `uMaxTargetsNoSweepAttack` and `uMaxTargetsSweepAttack`. **This modlist's effective values**
    (`mods\TAKEALOOK - MCM and INI\MCM\Settings\Precision.ini`): mode 1, **3 targets without the Sweep
    perk, 5 with it**. So here one swing already hits up to three actors, perk or not.
  - **API** (`PrecisionAPI.h`, "copy this file into your own project", interface V4): pre-hit callback
    (can veto a hit), post-hit callback (`PrecisionHitData`, `RE::HitData`), and
    `GetAttackCollisionCapsuleLength(actor)` for the real reach of the blade. Nothing in it starts a swing
    or names the targets in advance.
  - Friendly fire: teammates and summons are skipped in combat by default; neutrals are not guaranteed.
- Dynamic Sweep Attack, Savage Cleave, Elder Souls Sweep Attacks: author pages only, no source read
  (CX-20). The last one is hidden as obsolete in favour of Precision.

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

- **With Precision (H2, confirmed for this modlist's settings):** CIGAR adds no hit code. The accepted
  prompt starts one broad swing; Precision's collision hits what the blade passes through, once per actor
  per collision, up to its target cap (3 here). CIGAR registers a pre-hit callback only to veto
  non-hostile targets during its own swing (brief 17.2-5, 20.8), and a post-hit callback to log each hit
  (target, once-per-actor check) so the acceptance criteria can be read from the log.
- **Without Precision:** no proven way to make one swing hit two actors without a perk record or an
  unidentified engine hook (section 2). Given the amendment (Precision avoidance has low weight) and the
  user's rule on unverified engine internals, the module is **off without Precision**, with one log line.
  The vanilla-sweep route stays a later, separate experiment.
- Unknown until tested: whether the user's OAR animation for the chosen power attack opens one collision
  window or several (several could mean more than one hit per actor), and whether its arc really crosses
  two enemies standing side by side.
- Not proposed: a damage sphere, damage calls per actor, any multiplier (brief 8, 23).

## 6. Proposed animation strategy

Sources: `C:\TAKEALOOK\_codex\results\CX-21.md` (file names and public references only), checked here against
the load order's winning records (houseCARL, 2026-10-01) and the vendored CommonLibSSE-NG. Nothing here was
seen in game.

**What the load order says (confirmed from records).**

- The vanilla sideways power attacks are fenced off. `PowerAttackLeft` 00019B22 and `PowerAttackRight`
  00019B24 are won by `Attack_MCO_DXP.esp`: they still send `attackPowerStartLeft` / `attackPowerStartRight`
  and still require `GetMovementDirection` 4 / 2, but their parent is now `DirectionalPowerAttacks`
  (FEAB3800), which passes only in **first person or while sneaking**. In third person the engine never
  reaches them.
- Third person goes through MCO's own branch (`MCO_PowerAttack` FEAB3801 and `MCO_DirectionalPowerAttacks`
  FEAB3804, both won by `mco_keytrace_tdm_patch.esp` and switched by two globals). `MCO_PowerAttackLeft` /
  `Right` send the same `attackPowerStartLeft` / `Right` events but are gated by a Keytrace magic effect (a
  held direction key), not by movement; forward and standing send `attackPowerStartInPlace`.
- The right-hand power attack root `PowerAttack` 000E8456 is won by `For Honor Power Attack.esp`.
- CX-21, from file names: the For Honor OAR sets hold `MCO_PowerAttackN.hkx` only, no `1hm_` / `2hm_` /
  `2hw_attackpowerleft/right.hkx`. So in this game a third-person power attack is an MCO clip chosen by
  stance, weapon and combo stage; the vanilla sideways clips are not what the player sees.

**Consequences.**

- H3 has to be read more narrowly: "a vanilla sideways power attack that OAR remaps" does not exist in this
  load order in third person. The swing is **whatever the game's own power attack is** for the weapon and
  stance; whether that clip is broad enough to cross two targets is a per-set question only a run answers.
- The first prototype's start was wrong for this game: it played the vanilla idle form 019B22 / 019B24
  through `SetupSpecialIdle`, whose own condition (moving sideways) fails for a standing player and whose
  branch is closed in third person. CX-21 also objects to that route (it is the paired-idle route, not an
  attack).
- A mod that removes MCO gives the vanilla tree back, so the public design cannot assume either tree. The
  swing must be started by asking the engine for a power attack, not by naming a clip or an idle.

**Strategy.**

1. Start: the engine's action route, `TESActionData` with `ActionRightPowerAttack` and the player as source
   (`Process()`; the route CIGAR already uses for `ActionBumpedInto`). The idle tree then picks the attack
   exactly as a key press would, in either tree, with the engine's stamina, attack data and power-attack
   flag. Expected in this game while standing: `attackPowerStartInPlace`, the stance's `MCO_PowerAttack1`.
2. Direction: not chosen by CIGAR. The vanilla tree picks it from movement, MCO from Keytrace's held key;
   forcing either would mean faking movement or a magic effect. The log records which event the graph got.
3. No new HKX and no behaviour patch. A separate Cleave clip would need both, and a public build cannot
   ship another author's clip; brief 9 prefers existing resources.
4. Weapon classes are enabled one at a time, each only after its clip is seen to cross two close targets in
   the hit window (CX-21's recommendation): greatsword and sword first, then battleaxe; warhammer, war axe
   and mace are separate tests; dual wield and shield setups stay out.
5. The prototype measures instead of assuming: route used, the `attackPowerStart*` event received, stamina
   before and after, `IsPowerAttacking()`, the attack data's event name, swing and hit-frame events, and
   Precision's hits per target.

Not verified: whether `Process()` starts a power attack on a standing player in this load order; that the
engine charges stamina once on that route; clip length and hit window per weapon class; first person.

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
- Answered in section 6: a sideways clip cannot be asked for in this load order; the engine's power-attack
  action is the start. Open: what it plays per weapon and stance, and whether that crosses two targets.
- Whether the engine can turn the swing into a kill move.
- Prompt priority among combat prompts with four key slots.

## Decision (2026-10-03): Cleave dropped by the user

After r17b the user dropped the Cleaving Attack. Reason: the prompt only asks the engine for an ordinary
power attack, so what plays and what it hits is decided by MCO and the OAR stance sets (r17b: two clean
cleaves in five, three single hits), not by CIGAR. Many-against-one is to be improved by offering vanilla
answers instead, starting with a prompt that equips Unrelenting Force when two or more enemies face the
player. The war stomp is not part of this decision (judged after r17c). The prototype stays on the
`cleave-proto` branch for reference; the Cleave prompt is switched off there.

## 12. Recommendation

**Go on with Cleave as a prompt that asks the engine for one ordinary power attack when one arc can cover
two or more hostiles; do not build a Cleave attack of CIGAR's own.**

- Detection: a primary target plus secondaries inside an arc band, held 0.3 s, no prompt when a non-hostile
  stands in the arc (section 4).
- Hits: Precision resolves them (sweep mode is on in this modlist: 3 targets, 5 with the perk). Without
  Precision there is no prompt; CIGAR never deals damage itself (section 5).
- Swing: the engine's power-attack action, no clip named, no new animation (section 6).
- Scope: author build first; greatsword and sword, then battleaxe. Public only after the run below, and
  only for classes whose clip is seen to cross two targets.
- Key slots (the user's decision): Execute, then Throw, then the crowd slot, which Cleave shares with the
  war stomp and Fus Ro Dah (review below).
- The honest limit: in this game the prompt adds no new capability; a power attack with Precision's sweep
  already hits the group. What Cleave adds is the cue and one press without the hold. If the run shows the
  stance clips do not read as a sweep, the prompt should be dropped rather than given a clip of its own.

**What the next run (r14, test build) has to answer**, from `CIGAR.log` alone: does the action route start
a power attack while standing and while moving, and which event does the graph get; is stamina charged
once and the hit flagged as a power attack; how many of the group Precision hits, once each; does a
friendly in the arc keep the prompt away; does the clip for greatsword and for sword cross both targets.

## Review: the user's three answers to many-against-one (2026-10-01, no code)

The user's idea: three prompts for three crowd situations: (1) Cleave, (2) a war stomp, (3) a Fus Ro Dah
prompt. Decided so far: Execute and Throw come before Cleave when they compete for a key slot (D27). The war
stomp's motion and effect are being researched by another session
(`C:\TAKEALOOK\_codex\work\cleave\warstomp-research.md`); the tier 2 part below was settled from it.

### Which situation raises which

| Tier | Situation (what the gate reads) | What the player gets |
|---|---|---|
| 1 Cleave | Two or more hostiles **in front, together, inside weapon reach**: one arc covers them (section 4) | An attack opportunity |
| 2 War stomp | Hostiles inside reach that **no single arc covers**: in front and behind, or three or more around (the spread of their angles is over about 180°) | Room, at melee range |
| 3 Fus Ro Dah | A crowd **ahead but beyond reach** (inside the shout's cone and range); only if the shout is known and off cooldown. Placement only, no health or danger condition (the user, D30) | The crowd broken up |

- **Borders.** The three are told apart by geometry alone (D30): inside reach and one arc → 1; inside reach,
  not one arc → 2; beyond reach, in the shout's cone → 3. Each needs the 0.3 s hold the Cleave gate already
  has, or the prompt would flip as enemies shuffle.
- **One slot for all three.** With four key slots and Execute, Throw, lock-on, grapple, weapon swap, potion
  and the rest already competing, the three crowd answers should share **one** slot: the gate shows the one
  that fits now. It also keeps the feature from reading as a combat system with three buttons (brief 0,
  12, 23). Order inside a fight: Execute, Throw (D27), then the crowd slot.

### Tier 3: equip, or equip and shout?

CIGAR's rule is that a prompt performs the whole action, not a preparation step (the user declined the
tool swap and questioned a mode toggle for the same reason). So the press should **shout**, not only equip:
equip Unrelenting Force if another power is in the voice slot, perform the shout through the game's own
path, then put the earlier power back (as WeaponSwap gives the weapons back). `QuestAction` already equips
a shout (the Greybeards' trial), which is the equip half.
- Unverified: how to perform the shout legitimately. The game decides the number of words by how long the
  shout key is held, so the honest way is a synthetic key hold (CIGAR's `Util::PressKey` presses and
  releases at once; a held press is not built). Casting the shout's spell directly would skip the voice,
  the animation and the cooldown, which the brief's "legitimate result" rules out.
- Needs: the shout known (how many words), the cooldown at zero (`GetVoiceRecoveryTime`), not silenced.

### Tier 2: war stomp (settled from the research, 2026-10-01; motion chosen by the user, D31)

Source: `C:\TAKEALOOK\_codex\work\cleave\warstomp-research.md` (worker A, read-only; nothing checked in game).

- **Role.** Only when **surrounded**: hostiles on more than one side within about 200 units. 360 degrees, no
  damage, buys about one second. It must not come up for a cluster in front (that is Cleave's), or it becomes
  "Cleave without damage" and fights for the same prompt.
- **Effect: possible without an ESP.** `DLC1VampireChangeStagger` (Dawnguard 02012D18): cast on self, no damage,
  a stagger of 0.5 through an explosion of radius 200 with no force. Dust and sound come from vanilla forms played
  directly (`NPCGiantAttackStompSD` 0006CB40, impact set `FXDragonTailstompImpactSet` 0003F819), not from
  `crGiantStompExplosion`, which deals 2 damage. All are looked up by FormID and plugin at runtime.
- **Brief 8 and 23.** A stagger ring with no damage is not the forbidden damage sphere, and it is not always on:
  it shows only when surrounded and plays as a body action (motion, dust, sound). With that limit tier 2 stands;
  the earlier "drop or postpone" opinion is withdrawn for the effect.
- **Motion: Bow Rapid Combo V3's kick (the user, D31).** Nexus 89308 by lSmoothl (Smooth), installed here. Its
  permission is "free to set my mod as a requirement", so CIGAR ships none of its files: an optional link,
  off with one log line when the mod is absent. How the clip is reached with a melee weapon:
  - *(a) An OAR submod of CIGAR's pointing at the kick's folder with `overrideAnimationsFolder`.* Not used.
    The documented use is another submod of the same mod; whether OAR resolves a path into another mod's
    folder is not documented in the references here and was not tried (estimate: it joins the name to the
    parent folder, so `..` might work, unproven).
  - *(b) Raising the slot's event from the DLL.* Confirmed from Hot Key Skill's behaviour patch: the kick's
    slot `HKS_CustomAnimC` is the state `HKSCustomC` (id 150) of `1HM_Behavior`, reached by the wildcard
    transition on the event `CustomStartC`. The event alone is not enough: every Bow Rapid Combo submod
    requires a bow in the right hand, so with a sword the slot would play Hot Key Skill's empty base clip.
  - *(d) Used: (b) plus a submod CIGAR writes at load from the player's own file,* the way Squeeze borrows
    EVG's clip: `CIGAR Stomp\Kick\...\HKS_CustomAnimC.hkx`, a copy of the installed kick with its
    annotations cleared, conditions "weapon drawn, right hand not a bow". Nothing of Smooth's is in the
    download; the copy exists only on a machine that has the mod. No other installed mod fills that slot.
  - *(c) A copy kept in CIGAR-Personal* is not needed.
  - The annotations are cleared because the clip's own hit is a single-target kick (Precision collision on
    the leg, a touch stagger and dust through Payload Interpreter); the stomp's effect is CIGAR's, at the
    clip's hit frame (0.33 s).
  - To see with the eyes: the clip was made for a bow in the left hand, so the arms hold that pose with a
    sword, a shield or a two-handed weapon; and it reads as a kick forward, not a stomp.
- **Re-arm, no meter.** Repeated staggers would lock enemies down. The prompt is spent on use and comes back
  when the situation changes (the `spent` / situation rule `PromptSlot` has), with a vanilla stamina cost if a
  cost is wanted (brief 12). No timer meter.
- **Unverified, one in-game check each:** whether the spell cast by the player staggers followers and neutral
  NPCs in the ring (its effect is not flagged hostile, so it should not start a fight, but it may still stagger
  them: then the gate must refuse when a friendly is inside 200, as Cleave's does); whether a 0.5 stagger
  interrupts an attack in progress; enemies immune to stagger (large creatures, some bosses) make the prompt do
  nothing visible, so the gate should count only actors that can be staggered.

### Conflicts to settle with the user

- Three prompts for crowds risk the "general combat overhaul" the brief forbids; the single shared slot is
  this session's answer.
- Settled (D30): tier 3 has no health or danger condition, so it does not meet the Potion prompt's moment.
- Tier 2's effect against brief 8 and 23, as above.
