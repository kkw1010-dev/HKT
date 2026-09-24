# 027 · Fishing (absorbing Streamlined Fishing)

Status (2026-09-25): preparation only. Nothing is built; the design needs the user's decisions
listed at the end.

## What Streamlined Fishing is

`Streamlined Fishing` (parapets, Nexus 80683, v1.0.0), `StreamlinedFishing.esp` plus
`StreamlinedFishing.bsa`. The mod ships its `.psc` sources, and the BSA holds exactly these four
scripts and one SWF:

| File | What it does |
|---|---|
| `slf_utilityscript.pex` (quest `SLF_MainQuest`, `000800`) | `ChooseRod()`: every rod in `ccBGSSSE001_FishingRods` the player carries goes into a menu; one rod is picked without a menu; the chosen rod is equipped. |
| `quickitemmenu.pex` + `interface\quickitemmenu.swf` (quest `SLF_ItemMenu`, `000801`) | The rod menu. |
| `ccbgssse001_fishingactscript.pex` | Overrides the Creation Club Fishing Supplies script: `OnActivate` calls `SLF.ChooseRod()`, then `FishingSystem.StartPlayerInteraction(self, false)`. |
| `ccbgssse001_reellinescript.pex` | Overrides the reel-line trigger: if no rod is equipped it calls `ChooseRod()`, then `FishingSystem.OnFishingTriggerActivated()`. |

The ESP adds `SLF_ReelLinePerk` (`000802`, given by the ability `SLF_PlayerAbility`, `000804`),
whose one entry sets the activate label of `000DDA:ccbgssse001-fish.esm` (the reel-line
reference the fishing system moves to the fishing marker after a round) to "Cast Line".

## What actually runs on this modlist (read 2026-09-25)

Loose files beat BSA contents regardless of mod priority, and two loose scripts sit on top of the
fishing stack:

- **`Fish Anywhere With Water` ships a loose `ccbgssse001_fishingactscript.pex`.** Its `OnActivate`
  moves markers and calls `StartPlayerInteraction`, with no `ChooseRod()`. Streamlined Fishing's
  version is only in its BSA, so **activating the Fishing Supplies does not pick or equip a rod on
  this modlist.** Docs `018` ("No fishing rods") and `024` state the opposite, and the
  `verify_deploy.py` check that Streamlined Fishing is enabled guards a feature that is overridden.
- Only the reel-line half of Streamlined Fishing is live: the "Cast Line" label (perk, ESP) and
  the recast script (BSA; no loose copy exists and `StreamlinedFishing.esp` loads after the CC
  master).
- **`Fishing Preview` ships a loose `ccbgssse001_fishingsystemscript.pex`** (1771 lines of source).
  `Simple Fishing Overhaul` puts its own version of the same script (1882 lines; the
  `AnimatedFishing_Global` states, the player's cower/searching idles, bait from
  `AnimatedFishing_Bait`, `RodHeight`) only in `Simple Fishing Overhaul.bsa`, so **SFO's
  animations, bait and rod height are not running.** Fishing Preview's copy has no
  `AnimatedFishing` reference. `Immersive Fishing` (OAR) may still restyle the vanilla idles; its
  conditions were not read.
- Fish Anywhere starts fishing from a control listener: with a rod drawn, an attack press facing
  water casts a probe spell and, on water, places a supplies activator. The rod must already be
  equipped.

These conflicts exist whether or not CIGAR absorbs anything; they are modlist issues, recorded
here because a CIGAR fishing module would call into these same scripts.

## The fishing system entry points (Fishing Preview's live copy)

- `StartPlayerInteraction(ccBGSSSE001_FishingActScript akFishingSupplies, bool abContinueFishing)`
  (line 1066).
- `OnFishingTriggerActivated()` (line 1391) → `ReelLine()` (line 1572).
- `ShowReelLinePrompt()` (line 1663) shows `ccBGSSSE001_ReelLinePrompt` as a help message.
- `ReelLineRef` is moved to the fishing marker (line 1039) and back to its linked ref (line 894).
- `GetCurrentFishingRodType()` returns -1 with no rod; `IsFishingAllowed(rodType)`.

## Decisions needed from the user

1. **Which modlist fix for the two script conflicts.** Options: a small merged
   `ccbgssse001_fishingactscript` (Fish Anywhere's marker moves + Streamlined Fishing's
   `ChooseRod()`), or letting CIGAR equip the rod so the supplies script no longer matters; and,
   separately, whether SFO's animations are wanted (that needs Fishing Preview's preview call merged
   into SFO's system script, or one of the two dropped).
2. **What the CIGAR prompts are.** Candidates, following `docs/000` and the one-button rule:
   - 낚시하기 (hold) at Fishing Supplies or facing water with a rod carried: equip the best/last rod
     and start. This covers both the supplies and Fish Anywhere, and removes the rod menu.
   - 다시 던지기 after a round: Streamlined Fishing's Cast Line is already one activate press, so by
     the "nothing vanilla already does in one press" rule this is only worth a prompt if the user
     wants it off the E key.
3. **Which rod** when several are carried: last used (co-save), or a fixed order.
