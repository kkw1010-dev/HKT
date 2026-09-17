# 005 · LockOn: TDM target lock and Grapple prompts in combat

Status (2026-09-17): tested in game.

- **Results.** Lock-on worked. Grapple worked while locked. The prompts left
  when combat ended.
- **No re-lock.** After a grapple the lock was not taken again, because
  Grapple releases it for the wind-up.
- **Fixes.** An automatic re-lock (below) was added, and 그래플 is now also
  offered without a lock.

- **Test 2 (2026-09-18, new game).** 그래플 never appeared. `CIGAR.log`
  showed `Grapple ... key=-1 ... lockKey=-1` and "Grapple has no keyboard
  hotkey; the grapple prompt is off". The cause is in Grapple:
  - a new game starts `FH_Grapple`'s MCM properties at `Hotkey = -1`;
  - its `OnConfigInit` pushes that to the DLL
    (`[UpdateKeys] kbKey=-1` in `FH_Grapple_Plugin.log`), so the key from
    `FH_Grapple_Plugin.ini` (34) was dropped;
  - CIGAR read the key only at load, so rebinding G in the MCM two minutes
    later did not bring the prompt back.
- **Fix.**
  - CIGAR reads the INI's `kbKey` at `kDataLoaded`, before any game starts.
  - At load, an unset `Hotkey` is restored from that value (or from the last
    key seen in this session), then `ApplySettings()` pushes it to the DLL.
  - The key is re-read every second, so an MCM change applies at once.
  - A notification appears once when no usable key exists.
  - `verify_deploy.py` fails when the INI has no usable `kbKey`.

## Behaviour

| Condition | Prompt | Accept |
|---|---|---|
| In combat, movement controls enabled, TDM not locked | 록온 | press TDM's lock key |
| In combat, movement controls enabled, Grapple usable, and TDM locked or a hostile within 350 units | 그래플 | press Grapple's hotkey |

Both prompts can be up at once. They use different event IDs, so SkyPrompt
gives them different keys.

- **After a press.** Both prompts stay quiet for 3 s, so a lock that finds no
  target is not offered again at once. One second later the log records
  `after lock press: locked=...`.
- **Timing.** The module runs in `FastTick()` (100 ms).

**Grapple does not need a lock.** Grapple (`FH_Grapple_Plugin.dll` 1.2.0,
with its own SkyPrompt client) picks its target on the hit (`[GrappleHit]`).
It uses TDM only to turn toward the target (`[GrappleTurn] Injected TDM
lock-on key`) and releases the lock for the wind-up (`TDM lock released early
(wind-up phase)`). Grapple's reach is not published, so 350 units is a close
melee distance chosen here.

**Re-lock after a grapple.** When 그래플 is accepted while locked, CIGAR waits
for the grapple to start: `bIsSynced` becomes true, or the controls go off. It
then waits for the grapple to end. When the player is unlocked, still in
combat, and has had controls for 0.5 s, CIGAR presses the lock key once.

- **Grapple never starts** (a miss, or on cooldown). After 3 s CIGAR restores
  the lock that the wind-up released.
- **Timeout.** The wait gives up after 20 s.
- **Prompt.** 록온 is not offered while the re-lock is pending.

**Why a lock drops by itself.** TDM releases the lock in these cases:

- the target leaves `fTargetLockDistance` (2000 on this modlist; ×2 or ×4 for
  large targets);
- line of sight is lost (`bTargetLockTestLOS = 1`);
- the target dies or bleeds out, where TDM switches to another target or
  unlocks;
- Grapple's wind-up (above).

CIGAR changes none of these; while in combat, 록온 is offered again after a
drop.

## Why a key press

True Directional Movement (ersh1, `src/ModAPI.cpp`, `src/Papyrus.cpp`) can
read the lock state (`IVTDM1::GetTargetLockState`), but neither API can set
it. Target lock is toggled only in `InputEventHandler::ProcessEvent`, a sink on
`BSInputDeviceManager`:

- on a button *down* whose code equals `Settings::uTargetLockKey`;
- with key codes as keyboard scan code, mouse + 256, or a gamepad index;
- only when the game is not paused and movement controls are enabled.

`Util::PressKey` therefore builds a `ButtonEvent` pair (down, then up) with an
empty user event and sends it through the same event source. TDM sees a real
press, while the game's own controls ignore an event without a user event.
Gamepad codes are not supported.

The TDM lock key comes from `Data/MCM/Settings/TrueDirectionalMovement.ini`
(`[Keys] uTargetLockKey`), falling back to TDM's shipped `settings.ini`, then
to 258 (middle mouse). This is the same order TDM uses. On this modlist it is
258, from `TAKEALOOK - MCM and INI`.

## Grapple

Grapple (`FH_Grapple.esp` + `FH_Grapple_Plugin.dll` v1.2.0) is keyed from its
MCM quest `FHGrapple_Quest` (`000800`, script `FH_Grapple`):

- `Hotkey` is the grapple key (34 = G here). The DLL handles the key itself.
- `ModifierEnabled` / `ModifierKey`: a single synthetic press cannot hold a
  modifier, so the grapple prompt is off when a modifier is enabled.
- `TargetLockKey` is the key Grapple presses to release TDM's lock during a
  grapple (`[GrappleTurn] Injected TDM lock-on key`). It must equal TDM's
  lock key.

On every load, CIGAR compares `TargetLockKey` with TDM's key, and restores
an unset `Hotkey` (see test 2). If either changed, it sets the properties and
calls `ApplySettings()`. That registers the hotkey and calls `UpdateGlobals()`,
which pushes the keys to the DLL through `FH_Grapple_UpdateKeys`; the DLL
saves them to `FH_Grapple_Plugin.ini`.

## Self-reporting

- On load: `TDM api ok, lock key ...` and `Grapple quest=... key=...
  modifier=... lockKey=...`. When TDM's key cannot be pressed, a HUD message
  appears once.
- Every change of combat, locked, movable, quiet and grapple-ready is logged
  as a `gate` line.
- Every press is logged, and so is the lock state on the following tick.
- `tools/verify_deploy.py` checks:
  - TDM's `RequestPluginAPI` export;
  - that TDM's lock key is pressable;
  - Grapple's DLL and the script names read above.

## Prompt-only mode (2026-09-18)

Shared with `Surrender`; see also `docs/008-surrender.md`.

The user wants no manual hotkeys for Grapple and Acheron's surrender, so that
G is free for CIGAR's prompts. With 프롬프트 전용 on (the default, one switch per
mod in the control panel), CIGAR moves that mod's key to a key no ordinary
keyboard sends, and presses that key itself when the prompt is accepted:

| Mod | Hidden key | How CIGAR sets it | Why it is safe |
|---|---|---|---|
| Grapple | F13 (`0x64`) | `FH_Grapple.Hotkey` property + `ApplySettings()` | the DLL's input sink compares the keyboard scan code only (disassembly of `FH_Grapple_Plugin.dll` 1.2.0 at `0x2a70`–`0x2b89`); the key global is read nowhere else, so its SkyPrompt QTE does not use it |
| Acheron | F14 (`0x65`) | `AcheronMCM.SetSettingInt("iSurrenderKey")` on `AcheronMain` (`0x800`) | `EventHandler::ProcessEvent` compares the scan code only (`Scrabx3/Acheron`, `EventSink.cpp`); the setting is in memory at once and written to `Settings.yaml` on the next game save |

- The key the mod had before is kept in `CIGAR.json`
  (`promptOnly.<mod>.manualKey`) and restored when the switch is turned off.
- **When keys are checked.** Only at load, when a switch changes, and when
  the control panel's 모드 키 다시 확인 button is pressed. By the user's
  choice nothing is checked periodically, so a key changed in a mod's MCM
  during play reaches CIGAR only through that button (or the next load).
- **On a check with the switch on.** A key set in the MCM is remembered and
  moved back to the hidden key.
- **On a check with the switch off.** The MCM's key is used.
- **How the key is read.** Grapple's comes from its MCM script property.
  Acheron's is read back with `AcheronMCM.GetSettingInt` (asynchronous, one
  at a time), because Acheron writes `Settings.yaml` only on a save.
- **Gate.** The surrender gate reads `no-key` while the key is unbound.
- TDM's lock key (middle mouse) is unchanged.
