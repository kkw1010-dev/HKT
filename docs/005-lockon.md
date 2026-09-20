# 005 · LockOn: the TDM target lock prompt in combat

Status (2026-09-20): tested in game, including the split from the grapple prompt
(see below): 록온 and 그래플 were offered together on different keys, both presses
landed, and 록온 stayed off the screen while Grapple took the lock again.

- **Results.** Lock-on worked. The prompt left when combat ended.

## The split (2026-09-20)

Until 2026-09-20 one `LockOn` module owned both the 록온 and the 그래플 prompts.
They are now two modules, because their target mods are not equally likely to be
there: nearly every setup has True Directional Movement, while Grapple is a
Patreon mod that most do not have. As one module, a player without Grapple saw a
switch labelled 록온·그래플 whose second half could never fire.

- `LockOn` (this doc) needs only TDM.
- `Grapple` (`docs/014-grapple.md`) needs only Grapple, and uses TDM when it is
  there.
- `src/TDMLock.*` holds what they share: TDM's API pointer, its lock key, and a
  busy flag Grapple raises while it takes the lock again after a grapple, so the
  two never press TDM's key in the same frame. Each module resolves it on every
  game load, so either can be switched off without the other losing the lock.

No behaviour changed for a setup that has both mods, except that the 3 s quiet
time after a press is now per module: accepting 록온 no longer holds 그래플 back.

## Behaviour

| Condition | Prompt | Accept |
|---|---|---|
| In combat, movement controls enabled, TDM not locked | 록온 | press TDM's lock key |

- **After a press.** The prompt stays quiet for 3 s, so a lock that finds no
  target is not offered again at once. One second later the log records
  `after lock press: locked=...`.
- **While Grapple re-locks.** 록온 is not offered while `TDMLock::Busy()` is set.
- **Timing.** The module runs in `FastTick()` (100 ms).

**Why a lock drops by itself.** TDM releases the lock in these cases:

- the target leaves `fTargetLockDistance` (2000 on this modlist; ×2 or ×4 for
  large targets);
- line of sight is lost (`bTargetLockTestLOS = 1`);
- the target dies or bleeds out, where TDM switches to another target or
  unlocks;
- Grapple's wind-up (`docs/014-grapple.md`).

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
258, from `TAKEALOOK - MCM and INI`. `TDMLock::Resolve()` re-reads it on every
game load, so a key changed in TDM's MCM applies at the next load.

TDM's lock key is never moved to a hidden key: there is no prompt-only mode for
it, because middle mouse is not a key CIGAR's prompts want.

## Self-reporting

- On load: `TDM api ok, lock key ... from ... pressable=...`, or
  `True Directional Movement not found; this module idles`. When TDM's key
  cannot be pressed, a HUD message appears once.
- Every change of combat, locked, movable, quiet, relock and key-usable is
  logged as a `gate` line.
- Every press is logged, and so is the lock state on the following tick.
- `tools/verify_deploy.py` (`check_lockon`) checks TDM's `RequestPluginAPI`
  export and that its lock key is pressable.
