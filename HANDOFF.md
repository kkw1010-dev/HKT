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
  - **Bathe** (`src/Bathe.*`) uses Bathing in Skyrim - Renewed when present
    (the shower was confirmed on 2026-09-17):
    목욕하기 in water once nothing strippable is worn, and BiS's own
    `TryWashActor`. The dirt reset was confirmed. The **shower** path
    (waterfall) is untested.
  - **Dress** (`src/Dress.*`) is state-based. At any bed or wardrobe/dresser
    it offers 탈의하기 when dressed and 착용하기 when naked (the remembered
    outfit from the inventory). In water it offers 탈의하기; after leaving
    water it offers 착용하기 only if CIGAR undressed the player. It keeps the
    Softbody SMP carrier (`HDTSMPObjectBase`) and no-strip/locked items on.
  - **BaboKey** (`src/BaboKey.*`, `docs/004-babo-key.md`) offers 행동 선택
    in the BaboDialogue kidnap room while the kidnap quest is at stage 8–249.
    It is offered again after each accept. Accepting it calls
    `BaboDiaMonitorScript.OnKeyDown`.
  - **LockOn** (`src/LockOn.*`, `docs/005-lockon.md`) offers two prompts in
    combat:
    - 록온 while TDM is not locked on;
    - 그래플 while locked on or with a hostile within 350 units.

    Each accept presses that mod's key through `BSInputDeviceManager`. A
    grapple started while locked is followed by an automatic re-lock. At load,
    CIGAR syncs Grapple's TDM lock key to TDM's.
  - **Deflate** (`src/Deflate.*`, `docs/006-deflate.md`) is a hold prompt
    shown while Fill Her Up tracks an amount, once FHU's and SexLab's
    animations have been clear for 3 s. SkyPrompt's key down/up events are
    forwarded to FHU's `OnKeyDown`/`OnKeyUp`.
  - **Surrender** (`src/Surrender.*`, `docs/008-surrender.md`) is a hold
    prompt in combat below 20% health that presses Acheron's surrender key
    (K), with 3 s of x0.3 slow motion and a gold pulse.
    - Yamete Kudasai 2.2.3 registers its own surrender key but never handles
      it; Acheron does.
    - The prompt is hidden while every nearby enemy still has YK's 3-minute
      surrender timeout.
  - **In-game test 2026-09-17.** All four worked. Every Bathe, Dress and panel
    check passed, as did the shower check and the SI switches after SI's menu
    was opened. The follow-up fixes (see each doc's Status) are built and
    deployed but not yet retested.
  - **Prompts are kept on screen** while their gate holds: `PromptSlot`
    re-sends them every 2 s, which resets SkyPrompt's lifetime, and offers
    them again after a `kTimeout`.
- An optional SKSE Menu Framework page (CIGAR / 설정; `src/Panel.*`,
  `docs/007-control-panel.md`) switches modules on and off, shows each
  module's live gate and last log line, and sets the Dress reach. Choices are
  saved to `mods\CIGAR\SKSE\Plugins\CIGAR.json`. A module switched off gets
  `OnDisabled()`: Surrender ends its slow motion, and Deflate releases a held
  key.
- SI overlap is handled by `mods\CIGAR\SKSE\Plugins\StreamlinedInteractions\settings.json`:
  - SI Bathe and DressActions water/bed/wardrobe are off;
  - SI's preset is pinned to Power User (2), because other presets re-enable
    modules when SI's menu opens.
  - `tools/sync_si_settings.py` enforces this on every deploy, and CIGAR
    warns in game if a switch comes back.
- Untested: gamepad buttons (the user has no plans to use a pad). SI keeps the
  switches off after its menu is opened (confirmed 2026-09-17).

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
- Hold prompts (`PromptSlot::SetHoldMode`) use 5 down / 6 up instead of
  accepted; SkyPrompt sends them for any prompt type.
- Event types: 0 accepted, 1 declined, 2 removed by mod, 3 timing out,
  4 timeout, 5 down, 6 up, 7 move. Act on 0 only.
- Unequip/equip is queued. Ignore the worn state for a few ticks after
  changing it (`kSettleTicks`), or the logic misreads it; this bug happened
  once.
- `RE::UI::GameIsPaused()` is non-const.
- Modules have `Tick()` (1 s) and `FastTick()` (100 ms); both run only while
  unpaused.
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
2. **Babo dialogue hotkey** ("D"). The kidnap-event part is done as
   `BaboKey`, pending an in-game test. The key's other branches (merchant
   enthrall, Riekling Thirsk) are not covered; see `docs/004-babo-key.md`.
3. **Submit/Surrender** ("C"). The Acheron / Yamete Kudasai surrender is done
   (`Surrender`). BaboDialogue's own `bSurrenderKey` branch
   (`BaboSexControllerManager.Surrender(crosshairRef)`) is not covered.
4. **Simply Knock** ("B"). Not installed. II bundles a
   `simplyknockmainscript.pex` override, so check that interaction first.
5. **Private Needs** ("E"). Not installed. Prefer wrapping an existing needs
   mod over building a needs system.
