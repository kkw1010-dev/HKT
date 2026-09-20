# 015 · Potion: drinking from a prompt

**Status:** built 2026-09-20, **not tested in game.**

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

- The item must be an `AlchemyItem` that `IsMedicine()`, is not a poison, is not
  food, is not a quest object, and is carried.
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
- `gate need=... ratio=... urgent=... movable=... sexlab=... quiet=...
  potions=... pick=...` on every change.
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
