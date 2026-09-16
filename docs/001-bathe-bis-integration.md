# 001 · Bathe → Bathing in Skyrim - Renewed

## What SI's Bathe module does

Established from the DLL's string table (SI 1.0.6, 2025-06-21) and BiS scripts
decompiled with houseCARL. Only a running game can confirm this.

- Condition (SI's own MCM text): the player is undressed and standing in water.
- Action: plays the idle `mzinBatheA5_T1` and nothing else. SI hard-codes the path
  `Data/Meshes/Actors/Character/Animations/Bathing_in_Skyrim_Malignis/FNIS_Bathing_in_Skyrim_Malignis_List.txt`,
  which comes from *Malignis Animations - Bathing in Skyrim - Renewed* (installed).
  The idle record is `00039D:Bathing in Skyrim.esp`.
- The DLL has no Papyrus dispatch, no ModEvent and no reference to
  `Bathing in Skyrim.esp` or `mzinBatheQuest`, so SI never calls BiS.
- The animation carries `mzin_GetSoapy` / `mzin_GetUnsoapy` / `mzin_StopAnimation`
  annotations. The only BiS listener, `mzinPlayBathingAnimation`, is a magic-effect
  script that exists only while BiS itself runs a bath, so those events go
  nowhere after SI plays the idle.
- Result: the bath animation plays, but dirt, soap, follower bathing, tattoo fade
  and cum cleanup never happen.
- `StreamlinedInteractions.log` records `ScenesManager: Bathe loaded` only; no
  runtime line is written for prompts or actions.

## BiS entry points (2.7.8)

- `mzinAPI.GetBatheQuest()` → `mzinBatheQuest` (quest `mzinBatheQuest`).
- `mzinBatheQuest.TryWashActor(Actor, MiscObject washProp, bool shower, bool playerTeammates) → bool`.
  With `washProp = None`, BiS finds the best soap or cloth itself. It checks
  restrictions (shyness, devices, permissions), water or waterfall, shows its own
  message, plays its own animation set and handles followers.
- ModEvent `BiS_WashActor(Form actor, Form washProp, bool shower, bool teammates, bool animate, bool fullClean)`
  is an alternative that skips `TryWashActor`'s checks.
- `IsInWater` returns true everywhere when the MCM water restriction
  (`WaterRestrictionEnabled`) is off, and so does `IsUnderWaterfall`. SI-Extensions
  therefore gates the prompt with `PO3_SKSEFunctions.IsActorInWater` and only
  offers a shower prompt when the restriction is on.
- `IsActorAnimating(Actor)` is true while BiS's bathing spell is active.

## Design chosen

SI's `Bathe` module is turned off by an override `settings.json` in the
SI-Extensions mod. `SIX_BatheQuest` (start-game-enabled, ESL) polls once a second:

- Offer `목욕하기 (dirt%)` on key `1` when the player is undressed (no body slot
  32), in water, not in combat, not mounted, and BiS is enabled.
- Offer `샤워하기 (dirt%)` on key `2` when also under a waterfall.
- Offer each prompt once per water entry. Leaving the water, dressing, combat or
  mounting re-arms it, and ticks are skipped while BiS animates so that a finished
  bath does not bring the prompt back.
- On accept (SkyPrompt event type 0), call `TryWashActor(player, None, shower, true)`.

Keys are explicit keyboard DIK codes (as Camping Plus Plus does). SkyPrompt's
default slot keys are the same `1-4`. Gamepad buttons are not sent, because
SkyPrompt's device and key encoding for pads is not confirmed.

SI's WaterUndress prompt still appears while dressed in water. Once the player
undresses, it stops and the SI-Extensions prompt takes over, so the two never
show together.

## Unverified until played

- The prompt appears, and key `1` or `2` fires SkyPrompt event type 0.
- The event types SkyPrompt sends for timeout and decline (the log records every
  type).
- Where PapyrusUtil writes `SIX_Bathe.log` relative to the game folder.
- Whether the Malignis `A5` set is selected by BiS's own animation settings (a BiS
  MCM choice, not SI-Extensions).
