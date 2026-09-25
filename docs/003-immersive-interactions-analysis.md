# 003 · Immersive Interactions (II) analysis — animal petting and stress

Source: `C:\TAKEALOOK\downloads\Immersive Interactions - Animated Actions-47670-1-78-1724933859.zip`
(Nexus 47670, v1.78). It was not installed as of 2026-09-17; the archive was
read from a scratch extract.

## What II is

- **Plugin:** `ImmersiveInteractions.esp`, a full ESP (header flags `0x0`,
  42 KB). Its masters are Skyrim/Update/DLC and
  **`Dynamic Activation Key.esp`**, which is installed (Papyrus version).
- **Trigger:** a perk `AR_AnimPerk` (`000802`) with Activate entry points.
  Each fragment (`AnimimationsReborn_Fragments`) calls a function on the quest
  script `AR_QuestScript` of quest **`AR_Quest` (`000800`)**, for example
  `fpetdog(akActor, akTargetRef)` and `fwavehorse(akActor, akTargetRef)`.
- **Toggles:** perk entries are gated on MCM globals such as `AR_PetDog` and
  `AR_PetHorse`, plus `AR_SaluteGuards` and others. Twenty entries also check a
  DAK global (`05000801` in II's master list). Switching a feature off in II's
  MCM therefore turns its activation replacement off.
- **Animations:**
  - FNIS list `ImmersiveInteractions/FNIS_ImmersiveInteractions_List.txt` and
    a behavior file, so a Pandora regeneration is needed.
  - DAR folders `_CustomConditions/19931..19937` (OAR reads them).
  - Every DAR folder keys on the global **`AR_DogUp` (`00AA13`)**. II sets it to
    1–7, plays a vanilla idle (`idlegreybeardwordteach`, `idlesearchbody`,
    `idlekneelenter`, …) which DAR swaps for the real animation, then resets it
    to 0. Examples: 1 = child/dog-up petting, 2 = horse/dog petting,
    3 = chest/lever, 7 = tracking.
- **Animal functions:**
  - `fpetdog`: freezes the dog (`SetDontMove`), casts `AR_ReduceStressSpell` on
    the player, picks one of 3 variants (`AR_SmallDogs` forces the lying one),
    and plays dog idles (`dogbarktalk`, `idledoglay`) with player idles.
  - `fwavehorse`: horses get a wave/pet. Cows are milked (2 × milk, once per cow
    through `Interact_AlreadyUsed`). It does **not** cast the stress spell.
  - Both honour `AR_NoFirstPerson` and a `busy` flag. Neither checks
    `AR_PetDog`/`AR_PetHorse`; those only gate the perk.
- **Bundled foreign scripts:**
  - `simplyknockmainscript.pex` would override Simply Knock's script if Simply
    Knock is installed (relevant to the planned "Simply Knock" module).
  - `_Camp_InstinctsEffects.pex` would override Campfire's. Neither Simply
    Knock nor Campfire is installed today.

## Stress and Fear (S&F) link — already there

- S&F (`Stress and Fear.esp`, installed) keeps a conditional ability
  `Stress_CombatTrackerSpell` (`000813`) on the player. Its effects
  `Stress_SpellReduce10/25/50` run `Stress_SpellTracker`, which changes the
  global `Stress_Total` (`000801`) and shows S&F's own message. Each effect is
  conditioned on
  `HasMagicEffectKeyword(Stress_Reduce10/25/50 | Stress_Increase…)`.
- `Stress and Fear_KID.ini` tags **`AR_ReduceStressMEffect`** (II) with
  `Stress_Reduce25`. The KID list also tags
  `Dirt_MagicEffectSoapy`, `RestedSkillEffect`, altar effects and others.
- Result: whenever II casts `AR_ReduceStressSpell` (dog petting, applause,
  hugs, `AR_Ref_AliasScript` cases), S&F lowers stress by 25 with no patch.
- S&F's own equivalent is equipping `Stress_Potion_Reduce25` (a potion with the
  keyword), as its vampire-feed handler does. Stress can be lowered the same
  way without II.

## Recommendation (for the user's plan A–C)

1. **Install II as an optional provider, then turn its features off in its
   MCM.** Turn off at least Pet Dogs and Pet Horses (and every other toggle if
   CIGAR should own the trigger). Regenerate Pandora for its FNIS list.
2. **Add a CIGAR `Animals` module (ESP-free).**
   - When the crosshair is on a living, non-hostile dog, horse or cow, offer
     `쓰다듬기` (cows: `우유 짜기`).
   - On accept, dispatch `AR_QuestScript.fpetdog` / `fwavehorse` on `AR_Quest`
     via `DispatchMethodCall2`. This reuses II's animations, DAR switching,
     timing and stress spell exactly.
   - Resolve `AR_Quest` at runtime (`LookupForm(0x800, "ImmersiveInteractions.esp")`).
     If it is missing, the module idles and logs why (the same rule as `Bathe`).
   - Classify targets by race (`DogRace`, `DogCompanionRace`, `HorseRace`,
     `CowRace`, …) plus II's `AR_ModdedDogs` / `AR_SmallDogs` lists.
3. **Stress (plan B):**
   - Dog petting already lowers stress through II → S&F.
   - For horses (and any target II does not cover, such as Menagerie cats),
     CIGAR can cast II's `AR_ReduceStressSpell` itself after the action, found
     at runtime.
   - Without II, fall back to S&F's `Stress_Potion_Reduce25` equip.
   - Enable each only when its plugin is present.
4. **ESP-free (plan C):**
   - **Yes for CIGAR.** Every piece above is a runtime lookup plus a VM
     dispatch plus a SkyPrompt prompt, with no masters and no patches.
   - II itself keeps its own ESP; it is a separately installed provider, like
     BiS. A full slot could be saved by ESL-flagging it (all records are in
     its own range; verify with houseCARL before compacting).
   - Replicating II's animations inside CIGAR (no II at all) would mean shipping
     its assets and behavior data, so it is not recommended.
5. **Known gaps:**
   - II has no cat animations; a cat would get only the player idle (the
     dog-specific target idles fail silently).
   - `fwavehorse` has no stress effect unless CIGAR adds one.
   - II reacts to activation, so keep II's toggles off where CIGAR prompts.

Not yet verified: the exact behavior of II's perk entries with every toggle
off (read from the globals in the conditions, not tested in game), and whether
`fpetdog` behaves well when called outside its perk fragment.
