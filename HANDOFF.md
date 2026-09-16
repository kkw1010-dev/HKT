# CIGAR — session handoff (2026-09-17)

Read this first, then `README.md`. The design rationale and test history are in
`docs/`.

## Where things stand

- `CIGAR.dll` is an ESP-less SKSE plugin (CommonLibSSE-NG alandtse `ng`,
  SE+AE+VR). It is deployed to `C:\TAKEALOOK\mods\CIGAR\SKSE\Plugins\` and
  enabled in the MO2 profile `TKL - MUNG ADDON`, directly above
  `[NoDelete] 0008 StreamlinedInteractions`.
- The git repo `C:\TAKEALOOK\TKL-Agent\CIGAR` is **local only** (no remote).
  "Push" has meant "commit" so far.
- Modules, all confirmed in game by the user:
  - **Bathe** (`src/Bathe.*`) uses Bathing in Skyrim - Renewed when present:
    목욕하기 in water once nothing strippable is worn, and BiS's own
    `TryWashActor`. The dirt reset was confirmed. The **shower** path
    (waterfall) is untested.
  - **Dress** (`src/Dress.*`) is state-based. At any bed or wardrobe/dresser
    it offers 탈의하기 when dressed and 착용하기 when naked (the remembered
    outfit from the inventory). In water it offers 탈의하기; after leaving
    water it offers 착용하기 only if CIGAR undressed the player. It keeps the
    Softbody SMP carrier (`HDTSMPObjectBase`) and no-strip/locked items on.
- SI overlap is handled by `mods\CIGAR\SKSE\Plugins\StreamlinedInteractions\settings.json`:
  - SI Bathe and DressActions water/bed/wardrobe are off;
  - SI's preset is pinned to Power User (2), because other presets re-enable
    modules when SI's menu opens.
  - `tools/sync_si_settings.py` enforces this on every deploy, and CIGAR
    warns in game if a switch comes back.
- Untested: gamepad buttons (the user has no plans to use a pad), and whether
  SI keeps the switches off after its menu is opened under Power User.

## The user's standing expectations (also in Claude memory)

- CIGAR must stay **ESP-less**. Integrations are optional and detected at
  runtime, and a missing target is skipped silently (logged). There are no
  masters, no per-mod patches, and no MCM toggles for detection.
- Execute the work instead of handing back manual steps. Ask only for what an
  agent cannot do, such as in-game observation.
- Make failures self-reporting: log the gate inputs, and notify once on silent
  blockers. Read `CIGAR.log` before asking the user to re-test.
- A new game is not a cost for this user; mention it in one line when it
  applies. DLL changes usually do not need one.
- Leave no build servers running (`tools/Build.ps1` stops
  `mspdbsrv`/`vctip`/`MSBuild` it started).
- Player-facing Korean is short and administrative. Repo docs are in English.

## How to work on it

```powershell
powershell -ExecutionPolicy Bypass -File C:\TAKEALOOK\TKL-Agent\CIGAR\tools\Build.ps1 -Deploy
```

- Toolchain: VS 2026 Build Tools, Ninja, and vcpkg at
  `C:\TAKEALOOK\TOOLS\vcpkg` (baseline pinned in `vcpkg.json`). An
  incremental build takes well under a minute.
- Skyrim must be closed to deploy; MO2 may stay open. Profile edits
  (`tools/register_profile.py`) need MO2 closed, and the tool refuses
  otherwise.
- Runtime log: `%USERPROFILE%\OneDrive\Documents\My Games\Skyrim Special Edition\SKSE\CIGAR.log`
  (recreated each launch).
- Adding a module: `docs/000-adding-a-module.md`.

## Pitfalls already paid for

- SkyPrompt 2.3.15 exports only `RequestClientID`/`SendPrompt`/`RemovePrompt`/`RequestTheme`.
  Removal is per sink, so each prompt is its own `PromptSlot`. The API header
  needs `UNICODE`.
- Event types: 0 accepted, 1 declined, 2 removed by mod, 3 timing out,
  4 timeout, 5 down, 6 up, 7 move. Act on 0 only.
- Unequip/equip is queued. Ignore the worn state for a few ticks after
  changing it (`kSettleTicks`), or the logic misreads it; this bug happened
  once.
- `RE::UI::GameIsPaused()` is non-const.
- CommonLibSSE-NG is GPL-3.0-or-later; mind this before any distribution.

## Backlog (the user's plan, in the order discussed)

1. **Animals** (petting cat/dog/horse; plan "A").
   `docs/003-immersive-interactions-analysis.md` recommends:
   - install Immersive Interactions as an optional provider and turn its MCM
     toggles off;
   - have a CIGAR module offer 쓰다듬기 on the crosshair target and dispatch
     `AR_QuestScript.fpetdog`/`fwavehorse` on `AR_Quest` (`000800:ImmersiveInteractions.esp`);
   - stress relief already flows II → Stress and Fear through KID
     (`AR_ReduceStressMEffect` → `Stress_Reduce25`), and CIGAR can add it for
     horses and cats.

   **Waiting on the user's decision** to install II (the archive is in
   `C:\TAKEALOOK\downloads`; not installed).
2. **Babo dialogue hotkey** ("D"). Reuse `C:\TAKEALOOK\TKL-Agent\BDSM_Dev`
   (`HANDOFF.md`): the interaction key is
   `BaboDialogueConfigMenu.NotificationKey` on quest `0x2FEA1B`, and
   `bSurrenderKey` is a bool, not a key. A prompt can simulate that key.
3. **Submit/Surrender** ("C"). BaboDialogue has surrender logic; investigate
   its entry points together with item 2.
4. **Simply Knock** ("B"). Not installed. II bundles a
   `simplyknockmainscript.pex` override, so check that interaction first.
5. **Private Needs** ("E"). Not installed. Prefer wrapping an existing needs
   mod over building a needs system.
