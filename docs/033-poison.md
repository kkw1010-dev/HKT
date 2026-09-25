# 033 · Poison (독 바르기)

Status (2026-09-25): the user's addition. The first build crashed to desktop on accept (test run 1);
the fix below passed in game (test run 2).

## As built

- Gate (1 s): weapon drawn; the right-hand entry is a weapon (not a staff, not fists) without poison;
  a poison is carried; movement controls on; not in a SexLab scene.
- Pick: the most valuable poison carried (`GetGoldValue`).
- Accept (hold): doses = 1 through the `Mod Poison Dose Count` perk entry point (Concentrated Poison),
  `InventoryEntryData::PoisonObject` on the equipped entry, one poison removed. Logs
  `applied ... poisoned after=<bool>` and notifies if the weapon is not poisoned afterwards.

## The crash (test run 1, `crash-2026-09-25-16-10-16.log`)

`HandleEntryPoint(kModPoisonDoseCount, player, &doses)` passed no filter forms. That entry point has
three condition tabs (every PERK carrying it says `PerkConditionTabCount = 3`: perk owner, weapon,
poison), so the engine read `&doses` as the weapon and the next stack word as the poison and the result
pointer; the crash is inside PerkEntryPointExtender's condition hook, one frame above
`Poison::OnAccepted`. It is now called with the weapon and the poison. `Recharge` already passed its
filter forms.
