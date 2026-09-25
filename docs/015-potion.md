# 015 · Potion: drinking from a prompt

**Status:** confirmed in game 2026-09-20 (test 2). Test 1 the same day showed no
prompt at all; the cause is below, and the fix is what test 2 ran on.

This module takes over the potion half of Streamlined Interactions' `ItemUse`
module, which the deployed SI override now turns off. SI's other `ItemUse`
actions (recharge a weapon, equip a weapon, armour or spellbook, make light)
stay with SI; `ItemUse.enabled` itself is left on.

## Why it was rewritten rather than copied

SI's DLL is closed source and its licence forbids reverse-engineering it, so
nothing was disassembled. What was read is its `settings.json`, a plain config
file, which names the six potion actions and the thresholds the user was
running. The logic below is CIGAR's own, written against the game's data.

## Behaviour

One prompt at a time, 마시기: <물약 이름>, with the bar's percentage for the
three actor-value needs (마시기: 소량의 체력 물약 (38%)). SkyPrompt shows at
most four prompts per client, so six potion prompts would crowd out every other
module.

Needs are checked in this order, and the first that holds wins:

| Order | Need | Holds while |
|---|---|---|
| 1 | 체력 | health at or below the panel's threshold (default 50%) |
| 2 | 수중 호흡 | swimming, submerged at 0.85 or more, with no water-breathing effect active |
| 3 | 기력 | stamina at or below its threshold (default 50%) |
| 4 | 마나 | magicka at or below its threshold (default 50%) |
| 5 | 해독 | an active effect from a poison |
| 6 | 질병 치료 | an active effect whose source spell is of type Disease |

Each of the six has its own switch in the control panel (3. 세부 설정 → 물약).
Every default matches what the user was running in SI: all six on, health 50%,
urgent health 20%, stamina and magicka 50%.

Shared gates: movement controls enabled, not in a SexLab scene, and 3 s of quiet
after a drink.

## Finding a potion

Potions are recognised by **what their effects do**, never by form ID, so an
alchemy overhaul's potions (Apothecary on this modlist) work with no patch.

- The item must be an `AlchemyItem` that is carried, is not food, is not a
  poison, and is not a quest object. **Not `IsMedicine()`** — see test 1.
- **One hostile or detrimental effect rules the whole bottle out**, however good
  the rest of it is.
- For 체력 / 기력 / 마나 / 수중 호흡, a matching effect has the need's actor
  value as its `primaryAV` and one of the archetypes that raise a value
  (`kValueModifier`, `kDualValueModifier`, `kPeakValueModifier`). Water
  breathing is actor value 57.
- For 해독 and 질병 치료, a matching effect has the `kCurePoison` or
  `kCureDisease` archetype.

**Strength** is the restored amount: magnitude for an instant restore, magnitude
× duration for a restore over time, the duration alone for water breathing, and
1 per curing effect.

**Which bottle.** Normally the *weakest* potion whose strength covers what is
missing from the bar, so a grand potion is not spent on a scratch; if none
covers it, the strongest carried. Below the panel's 체력 위급 threshold
(default 20%) the strongest is always taken.

## Timing

The gate runs in `FastTick()` (100 ms), because in combat a health prompt one
second late is a health prompt that arrived after the swing. The inventory is
the expensive part, so it is only read when the need changes, when the bottle
that was picked is gone, and once a second otherwise.

## Drinking

Accepting equips the potion (`ActorEquipManager::EquipObject`), which is what
the inventory menu does, so animation mods that react to drinking see the same
event. The need is re-checked at the accept, because the situation may have
changed while the prompt was up.

## Self-reporting

- On load: `ready: health=...(50%/20%) stamina=... ... sexlab=...`, the switches
  and thresholds actually in force.
- `gate need=... ratio=... urgent=... movable=... sexlab=... quiet=... alch=...
  potions=... pick=...` on every change. `alch=` counts the alchemy stacks the scan
  examined, so an empty pack and a filter that turned everything down are not the
  same line (test 1).
- Every drink logs the bottle, its strength, the need and the ratio.
- Three seconds later the log records what the need became. A need that did not
  ease is a WARN and one HUD notification per session: the bottle did not do
  what its effects said it would.

## Not carried over from SI

- **SI's `cooldown` (20 s).** What it covers in SI is not documented, and its
  DLL was not disassembled. CIGAR uses the same 3 s settle as `Eat`, for the
  same reason: the equip and the effect need a moment before the gate is read
  again. Nothing else limits how often a potion may be drunk.
- **SI's `slow_time_hp_pot`.** It is off in the user's settings. Adding it would
  mean choosing a time multiplier, which is the user's call rather than a value
  to invent; `Surrender` already has the slow-motion machinery to reuse if it is
  wanted.
- **The non-potion `ItemUse` actions**: recharge a weapon, equip a weapon,
  armour or spellbook, make light. They remain SI's.

## Test 1 (2026-09-20): the prompt never appeared

`CIGAR.log` had the gate firing correctly — `need=health ratio=0.45` down to
`ratio=0.29` over two minutes — and `potions=0` on every line. So the need was
read right and the inventory scan rejected everything.

The cause was `AlchemyItem::IsMedicine()` in the filter. The Medicine flag is
set on **27 ALCH records in this entire load order** (mostly follower-mod
items), and on none of the healing potions: vanilla's own `RestoreHealth02`
(`03EADE:Skyrim.esm`) carries `Flags = 0`, and so does Apothecary's override of
it. CommonLibSSE reads that flag as `flags >> 16`, which is also true for
poisons, so the test was wrong in both directions.

It is gone. What makes an `AlchemyItem` a potion here is that it is neither food
nor poison and that its effects serve the need — checks that hold whatever an
overhaul does with the flags. The restore-health effect itself was verified
against the data at the same time: `MAG_AlchRestoreHealth` (`03EB15:Skyrim.esm`,
Apothecary's version) has archetype `PeakValueModifier`, actor value `Health`,
and flags `NoArea, PowerAffectsMagnitude` — no Detrimental and no Hostile — so
it passes.

**Made self-reporting.** The gate line now carries `alch=`, the number of
alchemy stacks actually examined, beside `potions=`. A need that holds with
nothing picked also logs once per session what was turned down and why:

```
no potion for health: 31 alchemy stacks examined, 12 food, 6 poison, 0 harmful,
13 without an effect for it; rejected: ...
```

`alch=0 potions=0` means an empty pack; `alch=31 potions=0` means the filter
turned everything down, and the line names the bottles. The first version could
not tell those two apart, which is what cost a run.

## Test 2 (2026-09-20): confirmed

`CIGAR.log` 19:23-19:26:

- health at 44% offered `마시기: 물약 - 체력 회복 최하급 (44%)`; accepting logged
  `drank 물약 - 체력 회복 최하급 (0003EADD, strength 50) for health at 0.49`, and
  three seconds later `after drinking for health: need is now -`. Twice.
- with health back up, the next need in the order took over: at stamina 27% the
  prompt became `마시기: 물약 - 지구력 회복 최하급 (43%)`, and that need cleared
  the same way.
- the prompt took whichever key slot was free (1 and 2 across the four offers),
  so it shares SkyPrompt's four slots with the other modules as intended.

The test-1 diagnostic proved itself in the same run. With the healing potions
drunk up, a health need logged:

```
no potion for health: 2 alchemy stacks examined, 0 food, 0 poison, 0 harmful,
2 without an effect for it; rejected: 물약 - 매지카 회복 최하급(no effect for
this need), 물약 - 지구력 회복 최하급(no effect for this need)
```

which says plainly that the pack was out of health potions rather than that the
filter had gone wrong again.

Not yet exercised in game: 해독, 질병 치료, 수중 호흡, and the 체력 위급
threshold picking the strongest bottle.

## Test 3 (2026-09-25)

Passed (the user; the log shows `drank 물약 - 수중 호흡 하급 ... for waterbreathing at 0.95`):
수중 호흡 and the 체력 위급 pick. Not yet run:

- **해독.** No cure-poison potion exists on this modlist: the only `ALCH` with a `CurePoison`
  effect is `SU04HoneyRum`, a food item, which the module skips. A player-made potion works; the
  only ingredients carrying Cure Poison are 헝거 혓바닥 (`7E03837D`, mihaildeadrapack.esp) and
  칼날주둥이 물고기 (`22066559`, Saints and Seducers), and Cure Poison is the only effect they share,
  so brewing the two gives a clean 해독 potion. A poison state: `player.cast 000638B2 player`
  (`crSpider01PoisonBite`, spell type Poison, which `Poisoned()` recognises).
- **질병 치료.** Starfrost puts `GetRandomPercent <= 2` (or 5) on the Stage01 disease effects, so a
  Stage01 spell added from the console usually stays inactive; the user saw no disease. Survival
  Mode is off here, so its Stage02/03 effects are inactive too. `MAG_DiseaseRattlesTrap`
  (`0010A24E`, 경련) has a non-survival effect with no random condition. The potion is
  `000AE723` (물약 - 질병 치료).

Results the same day (log 09:56-10:24):

- **질병 치료 passed**: `player.addspell 0010A24E` → `need=curedisease ... pick=물약 - 질병 치료`,
  held, `drank 물약 - 질병 치료`, and `need is now -` after it.
- **해독 not reached.** Brewing the two ingredients gave a 해독 potion, but no poison state was ever
  seen: `player.cast 000638B2 player` (also with 7 and 14 as the target) applied nothing, since a
  console cast of a Touch spell does not reach the caster, and every poison-type spell on this
  modlist is Touch. In a fight with a summoned 설원 거미 the Potion gate never left `need=-`. The
  frostbite spiders' poison is only on their power bites (`crSpider02PoisonBite`, 30-35% of attacks)
  and lasts 3 s. Whether a bite landed is unknown, because nothing logged it.
- **Added for the retest:** every poison-like effect on the player (poison spell type, a poison
  `AlchemyItem`, or an effect resisted by PoisonResist) now logs once as `poison-like effect: ...
  counted=<bool>`; and the author build's panel (3. 세부 설정 → 물약) has **시험: 독 10초 걸기**,
  which casts `DLC2crScribPoisonBite` (`04020E92`, 10 s) on the player with `CastSpellImmediate`.
