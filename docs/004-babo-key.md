# 004 · BaboKey: BaboDialogue's interaction hotkey during a kidnap

Status (2026-09-17): tested in game.

- **Test 1.** The prompt appeared in `BaboSlaverNobleHouse01` at stage 8 and
  opened the cage options. It stayed hidden outside the room and during
  scenes.
- **Two gaps.**
  - After cancelling the options box, the prompt never came back.
  - It did not come back after a rest on the next day either.
- **Fix (see the gate below).**
  - The prompt is now offered again after each accept.
  - The prompt is kept alive.
  - The prompt is no longer limited to the four key states.

- **BaboDialogue 6.2 Acheron patch (2026-09-17).** The mod
  `BaboDialogue - Acheron Patch` overrides `BaboDiaMonitorScript.pex`. It
  adds one call in `OnUpdateGameTime` and leaves `OnKeyDown`, its properties
  and states unchanged (its own gate, `tools/verify.py` in
  `TKL-Agent\BaboDialogue 6.2 Acheron Patch`, passes). `verify_deploy.py`
  now checks the winning `.pex` across all enabled mods, not only
  BaboDialogue's own folder.
- **`NotificationKey` = -1** on this modlist (no key bound in BaboDialogue's
  MCM). BaboDialogue then registers no key, so only CIGAR's prompt reaches
  `OnKeyDown`. CIGAR passes -1, which equals `NotificationKey`, so the
  handler proceeds.

## What the key does in BaboDialogue

`BaboDiaMonitorScript` (quest `BaboMonitorScript`,
`7E22B8:BaboInteractiveDia.esp`) registers `BDConfig.NotificationKey` and
handles it in `OnKeyDown(keyCode)`:

1. It returns when a menu is open, when `keyCode != NotificationKey`, or when
   the player is in `BaboNPCAnimating`.
2. If `BaboKidnapEvent` (`3BE3E7`) is running at stage >= 8, it calls
   `BaboKidnapEvenScript.KeyPress()` and stops there.
3. Otherwise it goes on to the Riekling Thirsk event, merchant enthrall,
   surrender, and a self-comment line. CIGAR does not cover these.

Stage 8's quest fragments call `KidnapperAddPerk()` and show BaboDialogue's own
hotkey tutorial (`BaboHotkeyTutorialMessage`, "Hoetkey"). Stage 250 releases
the player, and stage 255 shuts the quest down.

`KeyPress()` is state-dependent. It does something in only four states of
`BaboKidnapEvenScript`:

| State | Key result |
|---|---|
| `BaboKidnapCabin` | options box: struggle, magic, shout, talk, wait (00:00–08:00: time skip instead) |
| `BaboKidnapBanditCave` | the same options, but only while `bCaptured`; otherwise a comment line only |
| `BaboSlaverCabin` | cage options: call, persuade, rest one hour, meditate |
| `baboslaverinterval` | ends the wait and returns to `BaboSlaverCabin` |

Every other state (`BaboKidnapStandby`, `BaboKidnapPunishment`,
`babokidnapcabinguard`, `babokidnaphanged`, `BaboSlaverStandby`,
`BaboSlaverDisposed`, `baboslaverdisposedstandby`, `baboslaverdied`,
`BaboKidnapCabinPunishment`, the `BaboChangeLocationEvent07*` drunk events)
returns false at once.

## Where it happens

The quest's markers (`CenterMarkerPlayer` and the others) are filled from the
location alias `NewLocation` by location ref type `BaboKidnapVictimMarkerP`
(`460FD0`). Every location carrying that ref type is a BaboDialogue interior:

- `BaboKidnapperHouse01`–`04`
- `BaboKidnapperCave01`–`03`
- `BaboKidnapperTower01A`
- `BaboSlaverHouse01`–`04`
- `BaboSlaverNobleHouse01`
- `BaboEventOrcHouse`
- `BakaEventMorthalRoom`, `BakaEventRoriksteadRoom`, `BakaEventDragonsBridgeRoom`
  (drunk events)
- `BaboEventDLC2ThirskMeadHallInterior` (Riekling)

So "the player's cell is the cell of `CenterMarkerPlayer`" identifies the
kidnap room for whichever location the event picked, with no cell list
hard-coded.

Related globals, logged for diagnosis:

- `BaboKidnapTiedUp` follows `CurrentlyCaptured()`;
- `BaboKidnapScenarioe` is the scenario: 4 cabin, 10 bandit cave, 20 slaver
  cabin.

## CIGAR's gate

The 행동 선택 prompt is offered while all of these hold:

- `BaboKidnapEvent` is running, with 8 <= stage < 250;
- the player's cell equals the `CenterMarkerPlayer` reference's cell;
- the player is in neither `BaboNPCAnimating` nor `SexLabAnimatingFaction`
  (the latter only when SexLab is loaded).

The script state is not part of the gate:

- **Why.** The state changes during rests, punishments and scenes. The user
  wants the prompt for as long as they are in the room, and a press in a state
  whose `KeyPress()` returns false does nothing.
- **Logging.** The state, `bCaptured`, and the tied and scenario globals are
  still logged.
- **Controls.** Player controls are not checked either, because
  BaboDialogue's `StuckControl()` keeps a tied player's controls off.

The prompt is set to repeat (`PromptSlot::SetRepeat`). After an accept it is
offered again as soon as the gate holds, so cancelling BaboDialogue's options
box brings it straight back.

On accept, the gate is checked again. CIGAR then calls
`BaboDiaMonitorScript.OnKeyDown(NotificationKey)` on the monitor quest through
`DispatchMethodCall2`, so BaboDialogue's own checks and comment lines still
apply. Passing the configured value makes the call work even when no key is
bound (`-1`).

Every form is read from script properties at load. Only the monitor quest ID
(`7E22B8`) and SexLab's faction (`E50F`) are fixed.

## Self-reporting

- On load, `CIGAR.log` records what resolved (`Babo monitor=... key=...`).
  When BaboDialogue is installed but the scripts do not match, a one-time HUD
  message appears ("바보 납치 연동 실패").
- While the event runs, every change of stage, state, captured, tied,
  scenario, player cell, room cell and animating flags is logged as a `gate`
  line.
- `tools/verify_deploy.py` fails the build if the winning BaboDialogue scripts
  no longer contain the names read above.

## Not covered

- The Riekling Thirsk event (`BaboEventRiekling.KeyPress`) is a separate quest
  with its own sequence menu, and it runs at any stage.
- The merchant enthrall (Dibella stage >= 20) and surrender (`bSurrenderKey`)
  branches belong to backlog items 2 and 3.
