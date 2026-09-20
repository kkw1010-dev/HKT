# 014 · Grapple: the 그래플 prompt

Status: split out of the `LockOn` module on 2026-09-20. The prompt itself was
confirmed in game on 2026-09-17 and again on 2026-09-19 (test 2 below);
**the module split is not yet confirmed in game.**

Grapple is a Patreon mod, so most setups will not have it. The module then logs
`Grapple not found; this module idles` and shows nothing. That is why it is its
own module and not half of `LockOn`: see the split note in `docs/005-lockon.md`.

## Behaviour

| Condition | Prompt | Accept |
|---|---|---|
| In combat, movement controls enabled, Grapple usable, and TDM locked or a hostile within 350 units | 그래플 | press Grapple's hotkey |

- **After a press.** The prompt stays quiet for 3 s.
- **Timing.** The module runs in `FastTick()` (100 ms).
- 록온 and 그래플 can be up at once. They use different event IDs, so SkyPrompt
  gives them different keys.

**Grapple does not need a lock.** Grapple (`FH_Grapple_Plugin.dll` 1.2.0,
with its own SkyPrompt client) picks its target on the hit (`[GrappleHit]`).
It uses TDM only to turn toward the target (`[GrappleTurn] Injected TDM
lock-on key`) and releases the lock for the wind-up (`TDM lock released early
(wind-up phase)`). Grapple's reach is not published, so 350 units is a close
melee distance chosen here.

**Without True Directional Movement** the module still works: the prompt is then
offered on a hostile in reach alone, there is no re-lock, and `TargetLockKey` is
left as it is because there is no lock to release.

## Re-lock after a grapple

When 그래플 is accepted while locked, CIGAR waits for the grapple to start:
`bIsSynced` becomes true, or the controls go off. It then waits for the grapple
to end. When the player is unlocked, still in combat, and has had controls for
0.5 s, CIGAR presses the lock key once.

- **Grapple never starts** (a miss, or on cooldown). After 3 s CIGAR restores
  the lock that the wind-up released.
- **Timeout.** The wait gives up after 20 s.
- **Prompts.** 그래플 is not offered while the re-lock is pending, and neither
  is 록온: the module raises `TDMLock::SetBusy(true)` for the whole wait, so the
  two modules never press TDM's key in the same frame. Switching the Grapple
  module off in the control panel clears the flag.

## Grapple's keys

Grapple (`FH_Grapple.esp` + `FH_Grapple_Plugin.dll` v1.2.0) is keyed from its
MCM quest `FHGrapple_Quest` (`000800`, script `FH_Grapple`):

- `Hotkey` is the grapple key (34 = G here). The DLL handles the key itself.
- `ModifierEnabled` / `ModifierKey`: a single synthetic press cannot hold a
  modifier, so the grapple prompt is off when a modifier is enabled.
- `TargetLockKey` is the key Grapple presses to release TDM's lock during a
  grapple (`[GrappleTurn] Injected TDM lock-on key`). It must equal TDM's
  lock key.

On every load, CIGAR compares `TargetLockKey` with TDM's key (when TDM is
present), and restores an unset `Hotkey` (see test 2). If either changed, it
sets the properties and calls `ApplySettings()`. That registers the hotkey and
calls `UpdateGlobals()`, which pushes the keys to the DLL through
`FH_Grapple_UpdateKeys`; the DLL saves them to `FH_Grapple_Plugin.ini`.

### Test 2 (2026-09-18, new game): 그래플 never appeared

`CIGAR.log` showed `Grapple ... key=-1 ... lockKey=-1` and "Grapple has no
keyboard hotkey; the grapple prompt is off". The cause is in Grapple:

- a new game starts `FH_Grapple`'s MCM properties at `Hotkey = -1`;
- its `OnConfigInit` pushes that to the DLL (`[UpdateKeys] kbKey=-1` in
  `FH_Grapple_Plugin.log`), so the key from `FH_Grapple_Plugin.ini` (34) was
  dropped;
- CIGAR read the key only at load, so rebinding G in the MCM two minutes later
  did not bring the prompt back.

Fix:

- CIGAR reads the INI's `kbKey` at `kDataLoaded`, before any game starts
  (`Grapple::ReadIni`).
- At load, an unset `Hotkey` is restored from that value (or from the last key
  seen in this session), then `ApplySettings()` pushes it to the DLL.
- A notification appears once when no usable key exists.
- `verify_deploy.py` fails when the INI has no usable `kbKey`.

## Prompt-only mode

Shared with `Surrender` and `Execute`; the table and the general rules are in
`docs/008-surrender.md`. For Grapple:

| Hidden key | How CIGAR sets it | Why it is safe |
|---|---|---|
| F13 (`0x64`) | `FH_Grapple.Hotkey` property + `ApplySettings()` | the DLL's input sink compares the keyboard scan code only (disassembly of `FH_Grapple_Plugin.dll` 1.2.0 at `0x2a70`–`0x2b89`); the key global is read nowhere else, so its SkyPrompt QTE does not use it |

With 프롬프트 전용 on (the default), G stays free for CIGAR's prompts and the
key the mod had before is kept in `CIGAR.json` (`promptOnly.grapple.manualKey`),
restored when the switch is turned off. Keys are checked at load, when the
switch changes, and from the control panel's 모드 키 다시 확인 button; by the
user's choice nothing is checked periodically.

## NPC grapples

Grapple's own `bEnableNPCGrapple` (in `FH_Grapple_Plugin.ini`) lets NPCs grapple
the player. CIGAR does not read or change it; it was switched off by hand for
유술 test 12 on 2026-09-19 and switched back on on 2026-09-20 (see
`docs/012-jujutsu.md`).

## Self-reporting

- On load: `TDM present/absent (lock key ...)` and `Grapple quest=... script=...
  dll=... key=... modifier=... lockKey=... knownKey=...`, or `Grapple not found;
  this module idles`. A missing usable hotkey shows a HUD message once.
- Every change of combat, locked, movable, quiet, relock, key-usable and
  hostile-near is logged as a `gate` line.
- Every press is logged, and so is the lock state after a re-lock press.
- `tools/verify_deploy.py` (`check_grapple`) checks Grapple's DLL, the script
  names read above, and that the INI's `kbKey` is a key CIGAR can restore on a
  new game.
