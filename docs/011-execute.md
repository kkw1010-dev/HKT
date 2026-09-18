# 011 · Execute: Valhalla Combat's execution as a prompt

Status (2026-09-19): tested in game. Three targets were executed. Twice the first
press started no kill move and a second press about 3 s later did
(`WARN ... no kill move`, then `kill move started`). The user accepts this,
because Valhalla's own key misses in the same way. Target names showed as
FormIDs; `Util::NameOf` now uses the reference's display name.

The user asked for Valhalla Combat's execution as a CIGAR prompt that shows
only when an execution can actually happen. The background research is in
`TKL-Agent/Installation and Modification/cases/007-valhalla-combat-reference-and-cigar-execution.md`.

## Source used

- The installed DLL is Valhalla Combat 1.3.3 (Dec 2022). The source is
  `D7ry/valhallaCombat`. Commit `f5a9056` (2022-12-21) already has the V2 API
  with `isActorStunned`. `executionHandler.cpp` and `inputEventHandler.cpp`
  are identical from that commit to master (2024-03), so master describes the
  installed behaviour.
- The installed DLL's strings confirm it reads `Data\MCM\Settings\ValhallaCombat.ini`,
  registers `OnConfigClose` on `ValhallaCombat_MCM` and exports
  `RequestPluginAPI`.

## How Valhalla executes (what the gate mirrors)

1. Its input sink compares every raw key (keyboard scan code, mouse + 256)
   with `iExecutionKey`. On key down it calls `tryPcExecution`. The key is used
   for nothing else, so a synthetic press through `BSInputDeviceManager`
   (`Util::PressKey`) reaches it.
2. `tryPcExecution` takes the nearest stun-broken actor strictly within 250
   units.
   - **Bug:** the loop `return`s, not `continue`s, at the first stale entry
     (unloaded, dead, in a kill move, not in high process). A press can then
     do nothing.
3. `attemptExecute` refuses when:
   - stun is off;
   - the player is not humanoid (race body part data `0x1D`), is dead, is in a
     kill move or is mounted;
   - the victim is mounted, a teammate, essential or paralysed;
   - the victim's race is not in the RaceMapping INIs;
   - the weapon (attacking weapon, else right hand, else left hand) is a bow
     or crossbow.
4. It then picks a kill move by race category and weapon:
   - **Humanoid:** a dual wield (weapon in both hands, the right one not
     two-handed) always has a kill move, and so does an attack from behind
     (victim heading angle beyond ±90°). From the front a staff has none.
   - **Every other category:** unarmed has no kill move.

## Gate

The prompt reads `처형: <이름>`. It shows when every one of these holds:

- the API (V2) is present, the race map is loaded, stun is on, and the
  execution key is a keyboard or mouse key;
- movement controls are on and the player is not in a SexLab scene;
- the player passes step 3's checks;
- the actor Valhalla would pick (nearest `isActorStunned` actor within 250
  units) passes every refusal and has a kill move for the weapon.

Otherwise the gate logs which check failed in `why`: `stun-off`, `no-key`,
`no-controls`, `sexlab`, `player`, `no-stunned-target`, `target-mounted`,
`target-teammate`, `target-essential`, `target-paralysed`, `race-unmapped`,
`ranged-weapon`, `staff-front`, or `unarmed-vs-<category>`.

The race map is read the way Valhalla reads it: every `.ini` in
`Data/SKSE/Plugins/ValhallaCombat/RaceMapping`, sections by category name,
values `Plugin|0xFormID`, first category wins.

## Key handling (prompt-only)

- Prompt-only is on by default: `[Stun] iExecutionKey` is set to **F15 (0x66)**,
  a key no keyboard sends. When prompt-only is switched off, the remembered
  manual key comes back. If there is none, the key stays unset (−1) and the
  prompt is off (`why=no-key`).
- CIGAR writes the INI at `kPostLoad`, before Valhalla reads it at
  `kDataLoaded`, so the first launch needs no reload.
- At game load, from the panel's prompt-only switch and from the 모드 키 다시 확인
  button, the INI is read again. Any change is written, and
  `valhallaCombat_MCM.OnConfigClose` is dispatched on `ValhallaCombat_MCM_Quest`
  (`000D62|ValhallaCombat.esp`); this is Valhalla's native that re-reads the file.
- `Util::IniSetInt` rewrites one line. It keeps every other line, the BOM, the
  CRLF line endings and the final newline, which a standalone test on a copy
  of the real file confirmed (2026-09-19).

## Self-reporting

- At load: `Valhalla api= races= sexlab=`, `race map: N races from M files`,
  and `Valhalla iExecutionKey ...`.
- `gate target= ok= why= key= stun=`.
- `execute <target> (<FormID>) key 102 pressed=true`, then
  `kill move started on <target>`. Or, 2 s later, a `WARN` naming the likely
  cause (the stale-entry bug), with a one-time HUD notice
  `CIGAR: 처형 키 입력 후 처형 미발동. 로그 확인`.
- `tools/verify_deploy.py` fails the deploy if:
  - the DLL stops reading the INI, the key or the race map;
  - the MCM script loses `OnConfigClose`;
  - the race map has no humanoids;
  - stun is off;
  - any of the 69 kill-move sections Valhalla reads has no idle from a
    present plugin. Valhalla itself spells one section `Ginat-2HW`, and the
    shipped INI matches it.
