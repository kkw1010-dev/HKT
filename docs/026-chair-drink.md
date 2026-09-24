# 026 · Chair drink

Status (2026-09-24): built as module `ChairDrink` and deployed; not yet tested in game.

## The user's decision (2026-09-24)

SI's chair eat/drink (IdleActions; the only evidence is the vanilla idles `ChairEatingStart` /
`ChairDrinkingStart`) is kept as roleplay, narrowed: **inns and houses only, alcohol only, only
when the player carries it, and the drink is really drunk.** No eating.

## As built

- `src/ChairDrink.cpp`, once a second. Offered (hold, `마시기 (길게): <술>`) when:
  - `Rest::InChair`: the game's sitting state in furniture that is not a work station (see
    `docs/019-rest.md`, the same fix that keeps pass time off chopping blocks);
  - the current location or a parent carries `LocTypeInn`, `LocTypeHouse` or
    `LocTypePlayerHouse`;
  - not in combat;
  - an alcoholic drink is carried: an AlchemyItem, not a poison or quest item, with any of
    `MAG_FoodTypeAle`, `MAG_FoodTypeWine` (Gourmet, which wins vanilla ale, mead and wine here),
    `OCF_AlchDrinkAlcohol`, `VendorItemDrinkAlcoholModerate`, `VendorItemDrinkAlcoholStrong`,
    `_SH_AlcoholDrinkKeyword`. The cheapest one is picked.
- Accept: `ChairDrinkingStart` to the player's graph (the vanilla chair drinking loop, which brings
  its own tankard; there is no stop event, so it runs until the player gets up), then
  `ActorEquipManager::EquipObject` drinks one bottle so its effects apply. Accepting again while the
  idle runs drinks another bottle without restarting the idle.
- Getting up is the game's own chair exit on movement input.
- Self-report: 1 s after the accept, the bottle count must have dropped; with movement input held
  3 s while still seated after the idle, a WARN and a notification say the idle kept the player in
  the chair. `verify_deploy.py` fails if `0_master.hkx` loses `ChairDrinkingStart`.

## Open

- In game: the idle plays from a player-activated chair, and moving still gets up out of it.
