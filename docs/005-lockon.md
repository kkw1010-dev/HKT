# 005 · LockOn: TDM target lock and Grapple prompts in combat

Status (2026-09-17): built and deployed. Not yet tested in game.

## Behaviour

| Condition | Prompt | Accept |
|---|---|---|
| In combat, movement controls enabled, TDM not locked | 록온 | press TDM's lock key |
| In combat, movement controls enabled, TDM locked, Grapple usable | 그래플 | press Grapple's hotkey |

After a press, both prompts stay quiet for 3 s, so a lock that finds no target
is not offered again at once. The next tick logs `after lock press:
locked=...`.

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

On every load, CIGAR compares `TargetLockKey` with TDM's key. If they differ,
it sets the property and calls `UpdateGlobals()`, which pushes the key to the
DLL through `FH_Grapple_UpdateKeys`. On this modlist both are already 258.

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
