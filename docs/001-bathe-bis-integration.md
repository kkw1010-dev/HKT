# 001 · Bathe → Bathing in Skyrim - Renewed

> The tests below ran on the Papyrus builds (`SI-Extensions.esp`, scripts
> `SIX_*`, log `SIX_Bathe.log`). Since 2026-09-17 the same logic lives in
> `CIGAR.dll` (`src/Bathe.cpp`), and the undress/dress part is the `Dress`
> module (`002-dress.md`).
>
> Correction: the observed event types were misread below. The SkyPromptAPI
> header defines 5 = `kDown`, 3 = `kTimingOut` and 4 = `kTimeout`, not
> "shown" and "expired".

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

SI's WaterUndress prompt stays SI's. It was expected to stop once the body slot
is empty, but test 1 showed it can persist (see below), so both prompts can show
at the same time.

## Test 1 findings (2026-09-16)

- The startup notification appeared and SkyPrompt registration succeeded
  (client 4). `OnInit` fires twice on a new game, which is harmless because
  startup is idempotent.
- No SI-Extensions prompt appeared, because `mzinAPI.GetModState()` was `0`. BiS
  ships `mzinBathingInSkyrimEnabled` (`00000C`) with a default of 0 and is off
  until its MCM turns it on. The tick did not log its gate, so the log showed
  nothing. Every gate change is now logged, and the first water entry while BiS
  is off shows a notification.
- SI's own `목욕하기` still appeared. Opening SI's settings menu under the
  Interactive preset (1) rewrote `settings.json` with that preset's switches:
  Bathe, ItemUse potions and makelight on, KillMove and Observer off. The
  override is now pinned to the Power User preset (2), on the assumption that
  only Power User keeps per-module switches. This is unconfirmed, so the module
  reads SI's file on each water entry and warns if Bathe is on again.
- SI's Water Undress prompt kept appearing after the player undressed. SI
  evidently counts something still worn as clothing. Each water entry now logs
  every worn slot (30-61) to identify the item.
- `SIX_Bathe.log` lands in `overwrite/SKSE/Plugins/SI-Extensions/`.

## Test 2 result (2026-09-17): working

With BiS enabled in its MCM, the log recorded:

```text
gate water=TRUE waterfall=False BiS=TRUE dressed=False busy=False
entered water; worn: 60:HDTSMPObjectBase
offer event=0 dirt=20 sent=TRUE
prompt event type=5 event=0 action=0
prompt event type=0 event=0 action=0
wash shower=False result=TRUE
...
offer event=0 dirt=0 sent=TRUE      <- next water entry: BiS reset the dirt
```

- The prompt `목욕하기 (N%)` appears near the player's head (SkyPrompt anchors it
  to `refForm = PlayerRef`). Key `1` accepts it, and BiS washes the player and
  resets dirt from 20% to 0%. This worked twice in a row.
- SkyPrompt event types observed: `5` when the prompt is shown, `0` on accept,
  and `3` then `4` when the prompt went away without being accepted (most likely
  timing out, then timed out). Only `0` triggers an action.
- `SIBathe=0` read through `JsonUtil.GetPathIntValue` shows that JSON booleans
  read as 0/1. SI's Bathe stayed off under the Power User preset.
- Why SI's Water Undress prompt persists when naked: slot 60 holds
  `HDTSMPObjectBase` (the SMP physics carrier), which SI counts as clothing.
  This is a candidate for a follow-up module (an undress prompt that ignores
  non-clothing slot-60 items, or excluding that item from SI's check).
- The load path works: `READY (Load) client=5` after loading a save.

## Water Undress takeover (2026-09-17)

Reported in game: SI's `탈의하기` stayed up while naked. Pressing it made two
sword icons in Skyrim Party Sheet vanish and come back. The only worn item was
`HDTSMPObjectBase` (`000805:HDT SMP Object - Simple.esp`), which is:

- slot 60, `ArmorType = Clothing`, **NonPlayable**;
- tagged `SexLabNoStrip` and `OStimNoStrip`;
- kept equipped by GT Softbody's `DynamicSmpCollision` quest and
  `SMPCloakFFSelf` effect.

SI counts it as clothing and removes it, and Softbody puts it straight back.
The Party Sheet icons are most likely this item (the log showed no other
armour); held weapons are now logged on water entry to confirm that.

The user chose to replace SI's Water Undress as well
(`DressActions.enabled_water = false`):

- `탈의하기` (key `1`) is offered in water while any strippable item is worn.
  Strippable excludes non-playable items, the `SexLabNoStrip`, `OStimNoStrip`,
  `zad_Lockable` and `zad_QuestItem` keywords, and slots 31/40/41/43/50/51
  (hair, tail, long hair, ears, decapitation).
- Removed items are remembered (saved with the game). `착용하기` (key `1`) is
  offered after leaving the water and re-equips them.
- `목욕하기` now requires "nothing strippable worn" instead of "body slot
  empty", so undress and bathe take turns on key `1`.
- BiS itself also strips before its animation and redresses afterwards
  (`GetDressedAfterBathingEnabled`, default on). That restores only what BiS
  removed, so it does not conflict.

## Test 3 result (2026-09-17): undress/dress working

```text
entered water; worn: 30:종자의 투구 31:종자의 투구(kept) 32:종자의 갑옷 ... 60:HDTSMPObjectBase(kept) | left= right=
undress removed 종자의 투구 종자의 갑옷 종자의 장갑 종자의 장화 종자의 목도리 pendingDress=5
wash shower=False result=TRUE
dress equipped 종자의 투구 종자의 갑옷 종자의 장갑 종자의 장화 종자의 목도리
```

- Undress → bathe → dress worked twice, and the SMP carrier stayed on.
- The Party Sheet sword icons did not change. Both hands hold unnamed objects
  (`left= right=` rather than `-`), most likely the engine's unarmed "weapon".
  The user asked to leave this alone.

## Unverified until played

- The shower prompt (key `2`, under a waterfall) has not been exercised yet.
- Whether Power User keeps SI's Bathe switch off after SI's menu is opened again.
- Whether the Malignis `A5` set is selected by BiS's own animation settings (a BiS
  MCM choice, not SI-Extensions).
