# CIGAR — session handoff (updated 2026-09-17, evening)

Read this first, then `README.md`. The design rationale and the test history of
each module are in `docs/`, one file per module, and each file starts with its
status.

## Open items, in priority order

1. **Jujutsu / 유술, test 3** (`docs/012-jujutsu.md`). Test 2 settled it: swallowing KillMoveEnd keeps
   the victim alive (the essential flag does not). The victim now ends the throw in ragdoll. Check:
   `knocked the victim into ragdoll` followed by `ragdoll=true`, whether that looks right to the user,
   and the `try N:` lines of refused plays (about 4 in 10 were refused in test 2).
2. Backlog below.

Confirmed in game on 2026-09-19:
- the Execute prompt with actor names;
- the WeaponSwap return, and the arrows re-equipped with the bow;
- Grapple after a new game;
- unique prompt IDs in the kidnap room;
- the three panel pages;
- the F13/F14/F15 prompt-only keys, with G and K doing nothing.
Execute sometimes misses the first press; the user accepts this, because Valhalla's own key misses in the same way.

## Where things stand

- **The plugin.** `CIGAR.dll` is an ESP-less SKSE plugin (CommonLibSSE-NG
  alandtse `ng`, one DLL for SE, AE and VR).
  - It is deployed to `C:\TAKEALOOK\mods\CIGAR\SKSE\Plugins\` and enabled in
    the MO2 profile `TKL - MUNG ADDON`, directly above
    `[NoDelete] 0008 StreamlinedInteractions`.
  - The deployed DLL equals the build of `HEAD`, which `verify_deploy.py`
    checks.
- **The repo.** `C:\TAKEALOOK\TKL-Agent\CIGAR` is a **local git repo only**
  (no remote); "push" means "commit".
  - Branch: `master` only. The earlier `feat/menu-panel` worktree
    (`..\CIGAR-menu`) was merged and removed.
  - This machine has **no git identity configured**. Commits so far set it
    per command, without touching config:
    `GIT_AUTHOR_NAME=kkw GIT_AUTHOR_EMAIL=kkw1010@gmail.com
    GIT_COMMITTER_NAME=kkw GIT_COMMITTER_EMAIL=kkw1010@gmail.com git commit ...`,
    ending the message with
    `Co-Authored-By: Claude Opus 5 <noreply@anthropic.com>`.

### Modules

All modules except `WeaponSwap` and `Execute` are confirmed in game (2026-09-17; `Eat` on
2026-09-18: hunger 80 → 0 after 감자 수프).

| Module | Prompt(s) | Target mods | Doc |
|---|---|---|---|
| `Bathe` | 목욕하기, 샤워하기 | Bathing in Skyrim - Renewed | `001` |
| `Dress` | 탈의하기, 착용하기 | — | `002` |
| `BaboKey` | 행동 선택 | BaboDialogue | `004` |
| `LockOn` | 록온, 그래플 | True Directional Movement, Grapple | `005` |
| `Deflate` | 배출 (길게 누르기) | Fill Her Up Baka Edition | `006` |
| `Surrender` | 항복 (길게 누르기) | Acheron (+ Yamete Kudasai consequences) | `008` |
| `Eat` | 먹기: <음식 이름> | Survival Mode + SMI (Starfrost, Gourmet) | `009` |
| `WeaponSwap` | 원거리 무기, 근접 무기 | — (TDM lock target when present) | `010` |
| `Execute` | 처형 | Valhalla Combat | `011` |
| `Jujutsu` | 유술 | — (Valhalla optional) | `012` |

- **Bathe.** In water with nothing strippable worn, it calls BiS's own
  `TryWashActor`. The dirt reset and the waterfall shower were both
  confirmed.
- **Dress.**
  - State-based: at any bed or wardrobe it offers 탈의하기 when dressed and
    착용하기 when naked, using the outfit remembered in the co-save.
  - In water it offers 탈의하기; after leaving water it offers 착용하기, but
    only if CIGAR undressed the player.
  - It keeps the SMP carrier `HDTSMPObjectBase` and no-strip or locked items
    on.
- **BaboKey.**
  - Shown in the kidnap room (the player's cell equals the
    `CenterMarkerPlayer` alias's cell) while `BaboKidnapEvent` is at stage
    8–249, in any script state.
  - Offered again after every accept.
  - Accepting calls `BaboDiaMonitorScript.OnKeyDown(NotificationKey)`.
- **LockOn.**
  - 록온 shows in combat while TDM is unlocked.
  - 그래플 shows in combat while locked, or while a hostile is within 350
    units.
  - Both work by pressing that mod's key through `BSInputDeviceManager`
    (`Util::PressKey`), because TDM has no API to set the lock.
  - A grapple started while locked is followed by an automatic re-lock.
  - At load, Grapple's `TargetLockKey` is synced to TDM's key (258).
  - Prompt-only mode (default on) moves Grapple's `Hotkey` to F13, so G is
    free. When it is off, an unset `Hotkey` (every new game) is restored from
    the remembered key or `FH_Grapple_Plugin.ini`. Keys are read only at load
    and on the panel's key check button; nothing polls.
- **Deflate.**
  - A hold prompt. SkyPrompt's key down (5) and up (6) are forwarded to FHU's
    `sr_infDeflateAbility.OnKeyDown` / `OnKeyUp`.
  - Shown while FHU's `GetMostRecentInflationType(player) > 0`, once FHU's and
    SexLab's animating factions have been clear for 3 s.
- **Surrender.**
  - A `kHold` prompt in combat below 40% health, with 3 s of x0.3 slow motion
    (`BSTimer::SetGlobalTimeMultiplier`) and a white↔gold text pulse.
  - Accepting presses Acheron's surrender key, read from
    `Data/SKSE/Acheron/Settings.yaml`. Prompt-only mode (default on) moves
    that key from K to F14 through `AcheronMCM.SetSettingInt`.
  - Hidden (`yk-timeout`) while every hostile within 3000 units has
    `Kudasai_SurrenderTimeoutEFF`: YK's 3-minute rule, under which its
    surrender quest cannot fill.
  - Hidden (`babo-acheron-off`) while the BaboDialogue 6.2 Acheron patch has
    Acheron suspended (`AcheronSuspendedByUs`).
  - Yamete Kudasai 2.2.3 registers its own surrender key but never handles it.

### Shared machinery

- **`PromptSlot`** (`src/Prompt.*`):
  - one SkyPrompt sink per prompt;
  - keeps an offered prompt alive by re-sending it every 2 s;
  - offers it again after `kTimeout`;
  - optional repeat after an accept, hold mode (down/up), prompt type (e.g.
    `kHold`), and colour updates;
  - every prompt has a unique ID from `PromptID`;
  - every prompt lists its keyboard key: the lowest free of four key slots,
    whose keys are set in the control panel (default 1–4).
- **Ticks** (`src/main.cpp`): `Tick()` runs every 1 s and `FastTick()` every
  100 ms. Both run only while unpaused, and only for modules switched on.
- **Control panel.** Optional SKSE Menu Framework pages (CIGAR / 1. 모듈,
  2. 단축키, 3. 세부 설정;
  `src/Panel.*`, `src/Settings.*`, `docs/007-control-panel.md`):
  - per-module on/off switches;
  - each module's live gate and last log line;
  - the prompt keys;
  - the prompt-only switches for Grapple and Acheron;
  - the Dress reach.

  Choices are saved to `mods\CIGAR\SKSE\Plugins\CIGAR.json`. Switching a
  module off withdraws its prompts and calls `OnDisabled()`.
- **SI overlap.** `mods\CIGAR\SKSE\Plugins\StreamlinedInteractions\settings.json`
  turns off SI's Bathe and DressActions (water, bed, wardrobe) and pins SI's
  preset to Power User (2). `tools/sync_si_settings.py` enforces this on
  deploy, and SI keeps the switches off after its menu is opened.
- **Untested:** gamepad buttons (the user does not use a pad).

## The user's standing expectations (also in Claude memory)

- CIGAR stays **ESP-less**. Integrations are optional, detected at runtime,
  and skipped (with a log line) when absent. There are no masters, no per-mod
  patches, and no MCM toggles for detection.
- Prompts stay on screen for as long as their condition holds. Prefer broad
  gates, and log the narrower facts.
- Execute the work; do not hand back manual steps. Ask only for in-game
  observation or genuine decisions.
- Make failures self-reporting: log gate inputs and notify once on silent
  blockers. **Read the logs before asking the user to retest**, and give the
  user a short numbered test list with expected results.
- A new game is not a cost for this user (one line when it applies). DLL-only
  changes do not need one.
- Leave no build servers running.
- Player-facing Korean is short and administrative. Repo docs are in English.
  Reply to the user in Korean.

## How to work on it

```powershell
powershell -ExecutionPolicy Bypass -File C:\TAKEALOOK\TKL-Agent\CIGAR\tools\Build.ps1 -Deploy
```

- **Build.** VS 2026 Build Tools, Ninja, and vcpkg at
  `C:\TAKEALOOK\TOOLS\vcpkg`. An incremental build takes under a minute.
  Build.ps1 runs `check_menu_framework.py` and, with `-Deploy`,
  `sync_si_settings.py` and `verify_deploy.py`. `verify_deploy.py` fails on:
  - a stale DLL;
  - a missing dependency;
  - renamed script or property names in BaboDialogue, FHU, Grapple or
    Acheron;
  - an unpressable TDM or Acheron key.
- **Deploy.** Skyrim must be closed; MO2 may stay open.
- **Logs** (all under `%USERPROFILE%\OneDrive\Documents\My Games\Skyrim Special Edition\`):
  - `SKSE\CIGAR.log` is CIGAR's own log, recreated each launch. It has `gate`
    lines per module and every prompt event.
  - `Logs\Script\Papyrus.0.log` shows what the target mods' scripts did (FHU
    logs `[FillHerUp]`, YK logs `[Kudasai]`).
  - `SKSE\Acheron.log`, `SKSE\FH_Grapple_Plugin.log` (logging is off in its
    ini) and `SKSE\TrueDirectionalMovement.log`.
- **Reading a target mod.** Decompile its `.pex` with
  `housecarl_decompile_script` into a temporary `houseCARL - CIGAR_Tmp*` mod
  folder, copy what you need to the scratchpad, then delete that folder. Those
  folders are never in modlist.txt. Some mods ship `.psc` sources, but the
  `.pex` may be newer.
- **Adding a module.** Follow `docs/000-adding-a-module.md`, including a new
  `PromptID`.

## Pitfalls already paid for

- **SkyPrompt API** (installed DLL of 2026-03-04, API 2.0):
  - It exports only `RequestClientID`, `SendPrompt`, `RemovePrompt` and
    `RequestTheme`.
  - Removal is per sink. The API header needs `UNICODE`.
- **SkyPrompt interactions and keys:**
  - An (event, action) pair is one interaction. Shared IDs fire every owner;
    use `PromptID`.
  - Each event ID gets its own key slot, at most 4 per client.
  - `SendPrompt` on a queued prompt updates its text and colour and resets
    its lifetime.
- **SkyPrompt events:** 0 accepted, 1 declined, 2 removed by mod, 3 timing
  out (every frame; not logged), 4 timeout, 5 down, 6 up, 7 move. Down and up
  arrive for every prompt type.
- **Synthetic key presses.** Build `ButtonEvent`s with an empty user event and
  send them through `BSInputDeviceManager`. Input sinks such as TDM and
  Acheron react, and the game's own controls ignore them. Key codes: 0–255
  keyboard, 256+ mouse.
- **Papyrus calls.** `DispatchMethodCall2` works for events such as
  `OnKeyDown`. Alias scripts need the alias VM handle (`Util::Handle(alias)`).
  An auto property is read with `Object::GetProperty`, not
  `VirtualMachine::GetPropertyValue`.
- **Worn state.** Unequip and equip are queued, so ignore the worn state for a
  few ticks after a change (`kSettleTicks`).
- **Pausing.** `RE::UI::GameIsPaused()` is non-const, and ticks do not run
  while paused.
- **Licence.** CommonLibSSE-NG is GPL-3.0-or-later; mind it before any
  distribution. The user's CIGAR is personal use.

## Backlog (the user's plan, in the order discussed)


1. **Animals** (petting cat/dog/horse; plan "A").
   `docs/003-immersive-interactions-analysis.md` recommends:
   - install Immersive Interactions as an optional provider and turn its MCM
     toggles off;
   - offer 쓰다듬기 on the crosshair target and dispatch
     `AR_QuestScript.fpetdog`/`fwavehorse` on `AR_Quest`
     (`000800:ImmersiveInteractions.esp`).

   **Waiting on the user's decision** to install II. The archive is in
   `C:\TAKEALOOK\downloads` and is not installed.
2. **BaboDialogue hotkey, remaining branches.** The kidnap part is done
   (`BaboKey`). Merchant enthrall (Dibella stage >= 20) and Riekling Thirsk
   are not covered; see `docs/004-babo-key.md`.
3. **BaboDialogue's own surrender.** The Acheron / YK surrender is done
   (`Surrender`). BaboDialogue's `bSurrenderKey` branch
   (`BaboSexControllerManager.Surrender(crosshairRef)`) is not covered.
4. **Simply Knock.** Not installed. II bundles a `simplyknockmainscript.pex`
   override, so check that first.
5. **Private Needs.** Not installed. Prefer wrapping an existing needs mod.
