# 040: KuNeruNomu motions, future candidates

The user likes this Japanese modder's interaction motions and wants them kept for later (2026-09-29,
relayed by the orchestrator). This is a list of candidates only. Nothing here is built yet.

- **Source:** *EatingSleepingDrinking - KuNeruNomu -*, Nexus SE 65868, by gekkou1992 (Nexus shows
  "manager download only"). The two MAIN archives are kept unchanged in `C:\TAKEALOOK\downloads`,
  with MO2 `.meta` files:
  - `EatingSleepingDrinking - KuNeruNomu - NG` 2.3.0 (fileId 801130). Its ESP, DLL, widgets and
    settings hold no clips.
  - `Action Animations NG` 2.2.13 (fileId 801132). All 192 clips below are here, under
    `meshes/actors/character/animations/<folder>/`. It also has 8 behavior files, prop meshes in
    `meshes/animobjects/_ESD`, and the XML sets in `SKSE/Plugins/ActionAnimations`.
- **Use:** only as resources in the user's own game, for example an OAR submod that replaces a clip
  under a condition. The mod's ESP, DLL and Nemesis patch are not installed. CIGAR's Nexus edition
  never ships another mod's files (`docs/035`).
- **Structure:** most actions come as Start, then Mid (the loop), then End. The props (bottle, food,
  book) are AnimObjects that the mod's ESP defines. A clip reused without them plays with empty
  hands unless a matching AnimObject is provided.
- **Lengths:** measured with hkanno (`Toolchain/tools/anim_events.py --hkx`) on 2026-09-29. None of
  the clips carries its own annotations.

## Non-meal clips

### FillWaterBottle (24)

Filling a water bottle or skin at a water source, standing and crouching; per bottle type (generic, Flin, Matze, Mead, Shein, Sujamma). Start / Mid (loop) / End.

| Clip | Seconds |
|---|---|
| `ESDFillWaterBottle` | 0.50 |
| `ESDFillWaterBottleCrouching` | 0.50 |
| `ESDFillWaterBottleCrouchingEnd` | 0.47 |
| `ESDFillWaterBottleCrouchingMid` | 5.77 |
| `ESDFillWaterBottleEnd` | 0.53 |
| `ESDFillWaterBottleMid` | 8.03 |
| `ESDFillWaterFlinBottleEnd` | 1.03 |
| `ESDFillWaterFlinBottleMid` | 7.03 |
| `ESDFillWaterFlinBottleStart` | 1.00 |
| `ESDFillWaterMatzeBottleEnd` | 1.03 |
| `ESDFillWaterMatzeBottleMid` | 7.03 |
| `ESDFillWaterMatzeBottleStart` | 1.00 |
| `ESDFillWaterMeadBottle` | 1.00 |
| `ESDFillWaterMeadBottleCrouching` | 0.67 |
| `ESDFillWaterMeadBottleCrouchingEnd` | 0.37 |
| `ESDFillWaterMeadBottleCrouchingMid` | 5.70 |
| `ESDFillWaterMeadBottleEnd` | 1.03 |
| `ESDFillWaterMeadBottleMid` | 7.03 |
| `ESDFillWaterSheinBottleEnd` | 1.03 |
| `ESDFillWaterSheinBottleMid` | 7.03 |
| `ESDFillWaterSheinBottleStart` | 1.00 |
| `ESDFillWaterSujammaBottleEnd` | 1.03 |
| `ESDFillWaterSujammaBottleMid` | 7.03 |
| `ESDFillWaterSujammaBottleStart` | 1.00 |

### DrinkWaterFromWaterSource (6)

Drinking from a river or pond: crouching sip, or scooping handfuls. Start / Mid / End.

| Clip | Seconds |
|---|---|
| `ESDDrinkWaterCrouching` | 1.83 |
| `ESDDrinkWaterCrouchingEnd` | 1.87 |
| `ESDDrinkWaterCrouchingMid` | 2.53 |
| `ESDScoopHandfulsWater` | 2.00 |
| `ESDScoopHandfulsWaterEnd` | 1.30 |
| `ESDScoopHandfulsWaterMid` | 2.77 |

### WashBodyAnimations (10)

Washing the body standing (types A, B, HDT variant) and a 'peeping' variant. Long loops.

| Clip | Seconds |
|---|---|
| `ESDWashBodyPeepingEnd` | 1.03 |
| `ESDWashBodyPeepingMid` | 3.37 |
| `ESDWashBodyPeepingStart` | 1.00 |
| `ESDWashBodyStandingHDTEnd` | 1.23 |
| `ESDWashBodyStandingHDTLoop` | 17.70 |
| `ESDWashBodyStandingHDTStart` | 1.00 |
| `ESDWashBodyStandingTypeAEnd` | 1.37 |
| `ESDWashBodyStandingTypeAMid` | 13.03 |
| `ESDWashBodyStandingTypeAStart` | 1.67 |
| `ESDWashBodyStandingTypeBStart` | 14.30 |

### ReadBooksNotes (3)

Reading a note or book: enter, loop, exit.

| Clip | Seconds |
|---|---|
| `ESDIdleNoteRead` | 4.00 |
| `ESDIdleNoteRead_Enter` | 1.63 |
| `ESDIdleNoteRead_Exit` | 1.63 |

### MakeTowels (3)

Knitting (a towel): enter, loop, end.

| Clip | Seconds |
|---|---|
| `ESDKnitdoKnittingEnd` | 1.37 |
| `ESDKnitdoKnittingEnter` | 1.33 |
| `ESDKnitdoKnittingMiddle` | 4.03 |

### BedrollCampingAnimations (2)

Laying out and picking up a bedroll.

| Clip | Seconds |
|---|---|
| `ESDMakeBedroll` | 7.00 |
| `ESDPickupBedroll` | 1.83 |

### AbsorbDragonSoul (1)

Absorbing a dragon soul.

| Clip | Seconds |
|---|---|
| `ESDAbsorbDragonSoul` | 3.67 |

## Meal clips (143, folder `MealsAnimaitons` sic)

Eating and drinking with food-specific props; grouped by family (Start / Mid / End parts together).

| Family | Clips | Seconds (shortest-longest) |
|---|---|---|
| `ESDDefaultDrinking` | 1 | 4.5-4.5 |
| `KNNAnimObjectDrink` | 3 | 1.2-14.4 |
| `KNNAnimObjectDrinkMead` | 3 | 1.0-9.4 |
| `KNNAnimObjectEat` | 3 | 0.8-9.5 |
| `KNNAnimObjectEatBeef` | 1 | 8.3-8.3 |
| `KNNAnimObjectEatCheeseWedge` | 3 | 1.0-7.5 |
| `KNNAnimObjectEatCheeseWheel` | 3 | 1.0-6.8 |
| `KNNAnimObjectEatSoup` | 3 | 1.0-9.3 |
| `KNNDrinkFlin` | 3 | 0.8-14.0 |
| `KNNDrinkMatze` | 3 | 1.0-9.4 |
| `KNNDrinkShein` | 3 | 0.8-14.0 |
| `KNNDrinkSujamma` | 3 | 0.8-14.0 |
| `KNNDrinkWaterFromLargeBottle` | 3 | 1.0-8.9 |
| `KNNEatABeef` | 3 | 0.8-4.2 |
| `KNNEatABread` | 1 | 8.3-8.3 |
| `KNNEatAClamMeat` | 3 | 1.0-2.4 |
| `KNNEatAFlour` | 3 | 0.3-5.4 |
| `KNNEatAGourd` | 3 | 0.8-4.9 |
| `KNNEatAHalfBread` | 1 | 6.8-6.8 |
| `KNNEatAHoney` | 3 | 1.0-11.2 |
| `KNNEatAPotatoBread` | 3 | 1.0-5.0 |
| `KNNEatARawPotato` | 1 | 8.3-8.3 |
| `KNNEatAshyam` | 3 | 1.0-4.2 |
| `KNNEatBakedpotatoes` | 3 | 1.0-5.7 |
| `KNNEatBoarMeatCooked` | 3 | 1.0-5.0 |
| `KNNEatBoiledCremeTreat` | 3 | 1.0-9.0 |
| `KNNEatBraidedBread` | 3 | 1.0-5.0 |
| `KNNEatCabbage` | 3 | 1.0-4.2 |
| `KNNEatCarrot` | 3 | 1.0-5.4 |
| `KNNEatDumpling` | 3 | 1.0-8.2 |
| `KNNEatEidarCheeseWheel` | 3 | 1.0-7.5 |
| `KNNEatGarlicBread` | 3 | 1.0-5.7 |
| `KNNEatGoatMeatCooked` | 3 | 1.0-7.5 |
| `KNNEatGrilledChicken` | 3 | 1.0-5.4 |
| `KNNEatGrilledleeks` | 3 | 1.0-5.4 |
| `KNNEatHorkerMeatCooked` | 3 | 1.0-9.0 |
| `KNNEatHorseMeatCooked` | 3 | 0.9-6.8 |
| `KNNEatLeek` | 3 | 1.0-4.2 |
| `KNNEatMammothCheese` | 2 | 1.0-1.2 |
| `KNNEatMammothCheeseMidFirst` | 1 | 3.5-3.5 |
| `KNNEatMammothCheeseMidSecond` | 1 | 7.8-7.8 |
| `KNNEatMammothSteak` | 3 | 1.0-7.9 |
| `KNNEatMudCrabLegCooked` | 3 | 1.0-5.4 |
| `KNNEatNuttybread` | 3 | 1.0-4.2 |
| `KNNEatPie` | 3 | 1.0-5.9 |
| `KNNEatRabbitMeatCooked` | 3 | 1.0-5.4 |
| `KNNEatSoulHusk` | 3 | 1.0-5.4 |
| `KNNEatSweetroll` | 3 | 1.0-4.2 |
| `KNNEatTart` | 3 | 1.0-5.0 |
| `KNNEatTomato` | 3 | 1.0-4.2 |
| `KNNEatVenisonCooked` | 3 | 1.0-9.0 |
| `KNNMovingDrinking` | 1 | 6.3-6.3 |
| `KNNSittingSoup` | 1 | 1.5-1.5 |
| `KNNSittingSoupBase` | 1 | 5.7-5.7 |
| `KNNSittingSoupVar` | 2 | 5.7-5.7 |

## The same modder's other mods (listed only, nothing downloaded)

Checked on Nexus SE on 2026-09-29 (12 mods by gekkou1992). The ones with interaction or idle motions:

- *Simple Sit Idle Animation* (93975): random sitting idles for the player and NPCs.
- *RandomIdleAnimation* (48721): idles played at random.
- *Sauna House SE* (76078): a sauna house, which may have its own use animations. Not verified.

The rest are movement or combat replacers (female running and walking 49981, CGO dodge 49685, BFCO
katana 124372) and non-animation mods. Downloading any of them waits for the user.
