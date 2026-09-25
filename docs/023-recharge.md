# 023 · Recharge weapon

Status (2026-09-24): built as module `Recharge` and deployed; passed in game on 2026-09-25 (reported by the user; no log sent).

## On this load order

- The soul values are overridden by `Thaumaturgy.esp`: `iSoulLevelValuePetty` 600, `Lesser` 1200,
  `Common` 1800, `Greater` 2400, `Grand` 3000 (vanilla 250/500/1000/2000/3000). CIGAR reads the
  game settings at run time, so whatever wins is used.
- The perk entry point Mod Soul Gem Recharge is carried by Paragon's
  `APO_Enchanting_Artificer00` (x1.5) and `APO_Enchanting_Artificer50` (x2), winners in
  `Paragon - TKL Tweak.esp`, both with two condition tabs. CIGAR runs the entry point, so these
  count as in the inventory's own recharge.
- Reusable gems carry keyword `ReusableSoulGem` (`0ED2F1:Skyrim.esm`): Azura's Star, the Black Star,
  and Vigilant's two. Their souls sit on the instance (`ExtraSoul`); none has an empty form linked
  (`NAM0` on ordinary empty gems points to the *filled* form).

## As built

- `src/Recharge.cpp`. Each hand: the equipped entry's weapon, if enchanted (staves included; a
  two-handed weapon counted once). The charge the game uses is the hand's item-charge actor value
  (`RightItemCharge` / `LeftItemCharge`) while its permanent value is above 0, otherwise the
  instance's `ExtraCharge` against its enchantment's capacity. The gate line logs both.
- Offered (hold, `충전하기 (길게): <무기> (N%)`) out of combat, with movement controls enabled, for
  the hand at or below **25 %** (the lower one when both are). 25 % is CIGAR's choice.
- Gem: every filled instance in the inventory (player-filled `ExtraSoul` instances and pre-filled
  forms), quest-alias instances skipped, a pre-filled reusable form skipped (no empty form to give
  back). Worth = the soul's `iSoulLevelValue*` through the perk entry point. The smallest gem that
  fills what is missing wins; failing that, the largest.
- Accept: the gem is removed first (a reusable one is given back as its plain empty form), then the
  hand's charge value is restored by `min(worth, missing)` and the instance's `ExtraCharge` is set
  to match.
- The entry point's argument shape is read from the engine's table at load (names of its filter
  tabs); an unexpected shape skips perks and says so in the `ready` line, instead of calling the
  engine with the wrong arguments.
- Self-report: 1 s after an accept, the log compares the charge and the filled-gem count with what
  was expected, and a notification fires once if either did not move. A low weapon with no usable
  gem is explained once (how many filled, which skipped and why).

## Open

- In-game: that the item-charge actor value is what the HUD meter shows and that restoring it
  sticks after unequip (the log's `av` and `item` pairs answer this).
- The perk entry point's tab names on this runtime (the `ready` line).
