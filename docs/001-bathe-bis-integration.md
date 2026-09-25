# 001 · Bathe → Bathing in Skyrim - Renewed

> The tests below ran on the Papyrus prototype (2026-09-16/17). Since 2026-09-17 the same logic
> lives in `CIGAR.dll` (`src/Bathe.cpp`), and the undress/dress part is the `Dress` module
> (`002-dress.md`).
>
> Correction: the observed event types were misread in the first tests. The SkyPromptAPI header
> defines 5 = `kDown`, 3 = `kTimingOut` and 4 = `kTimeout`, not "shown" and "expired".

## Why BiS itself is called

Playing a bath idle alone is not a bath. The Malignis bathing animations (`mzinBatheA5_T1`, idle
`00039D:Bathing in Skyrim.esp`) carry `mzin_GetSoapy` / `mzin_GetUnsoapy` / `mzin_StopAnimation`
annotations, and the only BiS listener, `mzinPlayBathingAnimation`, is a magic-effect script that
exists only while BiS itself runs a bath. Without BiS running it, the animation plays but dirt,
soap, follower bathing, tattoo fade and cum cleanup never happen. So CIGAR asks BiS to wash.

## BiS entry points (2.7.8)

- `mzinAPI.GetBatheQuest()` → `mzinBatheQuest` (quest `mzinBatheQuest`).
- `mzinBatheQuest.TryWashActor(Actor, MiscObject washProp, bool shower, bool playerTeammates) → bool`.
  With `washProp = None`, BiS finds the best soap or cloth itself. It checks
  restrictions (shyness, devices, permissions), water or waterfall, shows its own
  message, plays its own animation set and handles followers.
- ModEvent `BiS_WashActor(Form actor, Form washProp, bool shower, bool teammates, bool animate, bool fullClean)`
  is an alternative that skips `TryWashActor`'s checks.
- `IsInWater` returns true everywhere when the MCM water restriction
  (`WaterRestrictionEnabled`) is off, and so does `IsUnderWaterfall`. CIGAR
  therefore gates the prompt with its own in-water test and only offers a shower
  prompt when the restriction is on.
- `IsActorAnimating(Actor)` is true while BiS's bathing spell is active.

## Design chosen

Once a second:

- Offer `목욕하기 (dirt%)` when nothing strippable is worn, in water, not in combat,
  not mounted, and BiS is enabled.
- Offer `샤워하기 (dirt%)` when also under a waterfall.
- Offer each prompt once per water entry. Leaving the water, dressing, combat or
  mounting re-arms it, and ticks are skipped while BiS animates so that a finished
  bath does not bring the prompt back.
- On accept (SkyPrompt event type 0), call `TryWashActor(player, None, shower, true)`.

## Test 1 findings (2026-09-16)

- SkyPrompt registration succeeded (client 4).
- No bathe prompt appeared, because `mzinAPI.GetModState()` was `0`. BiS ships
  `mzinBathingInSkyrimEnabled` (`00000C`) with a default of 0 and is off until its
  MCM turns it on. The tick did not log its gate, so the log showed nothing. Every
  gate change is now logged, and the first water entry while BiS is off shows a
  notification.
- Each water entry now logs every worn slot (30-61).

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
- SkyPrompt event types observed: `5` on key down, `0` on accept, and `3` then `4`
  when the prompt went away without being accepted. Only `0` triggers an action.
- The load path works: `READY (Load) client=5` after loading a save.

## Water undress (2026-09-17)

The only item worn while naked was `HDTSMPObjectBase` (`000805:HDT SMP Object - Simple.esp`):

- slot 60, `ArmorType = Clothing`, **NonPlayable**;
- tagged `SexLabNoStrip` and `OStimNoStrip`;
- kept equipped by GT Softbody's `DynamicSmpCollision` quest and
  `SMPCloakFFSelf` effect.

Counting it as clothing would keep an undress prompt up forever (a strip removes it and Softbody
puts it straight back). So CIGAR's own undress rules (now `Dress`) leave it on:

- `탈의하기` (key `1`) is offered in water while any strippable item is worn.
  Strippable excludes non-playable items, the `SexLabNoStrip`, `OStimNoStrip`,
  `zad_Lockable` and `zad_QuestItem` keywords, and slots 31/40/41/43/50/51
  (hair, tail, long hair, ears, decapitation).
- Removed items are remembered (saved with the game). `착용하기` (key `1`) is
  offered after leaving the water and re-equips them.
- `목욕하기` requires "nothing strippable worn" instead of "body slot
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
- Two sword icons in Skyrim Party Sheet did not change. Both hands hold unnamed
  objects (`left= right=` rather than `-`), most likely the engine's unarmed
  "weapon". The user asked to leave this alone.

## Unverified until played

- The shower prompt (under a waterfall) has not been exercised yet.
- Whether the Malignis `A5` set is selected by BiS's own animation settings (a BiS
  MCM choice).
