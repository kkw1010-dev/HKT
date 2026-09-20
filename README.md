# CIGAR

**C**ontextual **I**nteraction, **G**ameplay **A**cceleration & **R**hythm.

CIGAR is an ESP-less SKSE plugin (`CIGAR.dll`, one DLL for SE, AE and VR) that
offers context prompts through SkyPrompt. It was developed on the TAKEALOOK
modlist (`C:\TAKEALOOK`, profile `TKL - MUNG ADDON`).

## Principles

- **No plugin file.** Nothing is added to the load order, and no other mod
  becomes a master.
- **Optional integrations are detected at runtime.** An interaction whose target
  mod is missing stays idle and says so in the log. No per-mod patches and no
  MCM toggles exist just to enable or detect one. This keeps the core shareable
  when NSFW-leaning targets (Bathing in Skyrim, SexLab/OStim keywords) are
  absent.
- **Streamlined Interactions (SI) is not a dependency.** When SI ships a module
  that CIGAR replaces, CIGAR turns that SI module off through a `settings.json`
  override in its own mod folder. Disabling CIGAR brings SI's version back.

History: this project started on 2026-09-16 as `SI-Extensions`, a Papyrus
quest in an ESL. It was renamed CIGAR and moved to an ESL-hosted module split on
2026-09-17, then rewritten as this DLL the same day. The Papyrus versions and
their in-game tests are in git history and in `docs/001`.

## Modules

| Module | Needs | Replaces in SI | Status |
|---|---|---|---|
| `Bathe` | Bathing in Skyrim - Renewed (optional) | `Bathe` (animation only; BiS dirt untouched) | DLL confirmed in game 2026-09-17; shower untested |
| `Dress` | — | `DressActions` water / bed / wardrobe undress (strips the Softbody SMP carrier) | Confirmed in game 2026-09-17 (water, beds, wardrobes, state-based dress) |
| `BaboKey` | BaboDialogue (optional) | — | Confirmed in game 2026-09-17 (retest passed) |
| `LockOn` | True Directional Movement (optional) | — | Confirmed in game 2026-09-17 (retest passed); split from `Grapple` 2026-09-20, split not tested |
| `Grapple` | Grapple (optional; a Patreon mod, so usually absent) | — | Confirmed in game 2026-09-19; split out of `LockOn` 2026-09-20, split not tested |
| `Deflate` | Fill Her Up (optional) | — | Confirmed in game 2026-09-17 (retest passed; FHU-side effects pending in FHU) |
| `Surrender` | Acheron (optional; Yamete Kudasai supplies the consequences) | — | Confirmed in game 2026-09-17 (retest passed) |
| `Eat` | Survival Mode + SMI (optional) | — | Confirmed in game 2026-09-18 |
| `WeaponSwap` | — (TDM's lock target when present) | — | Built 2026-09-18; returns to the previous loadout since 2026-09-19; not tested in game |
| `Execute` | Valhalla Combat (optional) | — | Tested in game 2026-09-19 |
| `Jujutsu` | — (Valhalla optional) | — | Built 2026-09-19, not tested in game |
| `Needs` | Private Needs - Orgasm (optional) | — | Built 2026-09-19, not tested in game |

Prompts use the player's SkyPrompt default keys, on both keyboard and gamepad:

| Where | Prompt |
|---|---|
| In water, strippable items worn | 탈의하기 |
| In water, nothing strippable worn, BiS on | 목욕하기 (dirt %) |
| Under a waterfall (BiS water restriction on) | 샤워하기 (dirt %) |
| Aimed at a bed or wardrobe/dresser and within 250 units, strippable items worn | 탈의하기 |
| At any bed or wardrobe while naked (remembered outfit in inventory), or after leaving water when CIGAR undressed the player | 착용하기 |
| Kidnapped by BaboDialogue, in the kidnap room, at a point where its hotkey acts | 행동 선택 |
| In combat, TDM not locked on | 록온 |
| In combat, TDM locked on or a hostile within 350 units, Grapple installed | 그래플 |
| Fill Her Up tracks an amount, not animating (hold the key) | 배출 (길게 누르기) |
| In combat below 40% health, not defeated (hold the key; slow motion and a pulsing prompt) | 항복 (길게 누르기) |
| Survival Mode hunger at or above the panel's stage (default 3), out of combat, suitable food carried | 먹기: <음식 이름> |
| In combat, enemy beyond the panel's distance (default 800) or fleeing, melee weapon or fists in hand | 원거리 무기: <무기 이름> |
| In combat, enemy inside that distance, bow or crossbow in hand | 근접 무기: <무기 이름> |
| Valhalla's execution key would execute the nearest stun-broken actor within 250 units | 처형: <이름> |
| A hostile humanoid within 250 units is guarding, or one perfect-parried in the last 1.5 s (any distance) | 유술: <이름> |
| Private Needs bladder / bowel at or above the panel's fill (default 50%), out of combat and scenes | 소변 보기 (N%), 대변 보기 (N%) |

Background: `docs/001-bathe-bis-integration.md`, `docs/002-dress.md`, and
`docs/003-immersive-interactions-analysis.md` (planned `Animals` module), and
`docs/004-babo-key.md`, `docs/005-lockon.md`, `docs/006-deflate.md`, `docs/008-surrender.md`, `docs/009-eat.md`, `docs/010-weapon-swap.md`, `docs/011-execute.md`, `docs/012-jujutsu.md`, and `docs/014-grapple.md`. The in-game control panel
(SKSE Menu Framework, optional) is described in `docs/007-control-panel.md`. To add
a module, see `docs/000-adding-a-module.md`. Session handoff: `HANDOFF.md`.

## Layout

```text
src/main.cpp        SKSE entry, lifecycle messages, co-save, ticker (one game-thread task per 100 ms: FastTick every time, Tick once a second)
src/Module.h        module interface (OnGameLoaded / Tick / OnAccepted) and gated logging
src/Prompt.*        SkyPrompt client and one sink per prompt (SkyPrompt 2.3.15 removes by sink)
src/Util.*          strip rules, worn description, Papyrus script-property reader, SI settings reader
src/Bathe.*         Bathing in Skyrim integration (properties read from its quest script at load)
src/Dress.*         state-based undress / dress, crosshair-based bed and wardrobe detection, co-saved outfit
src/BaboKey.*       BaboDialogue hotkey during kidnap events (quest stage, script state, kidnap-room cell)
src/LockOn.*        TDM target lock in combat (synthetic key press through the input event source)
src/Grapple.*       Grapple in combat, its key management, and the re-lock afterwards (Patreon mod; idles when absent)
src/TDMLock.*       TDM's lock shared by LockOn and Grapple: API pointer, lock key, and the busy flag during a re-lock
src/Deflate.*       Fill Her Up deflation as a hold prompt (key down/up forwarded to FHU's own handlers)
src/Surrender.*     Acheron surrender key below 40% health as a hold prompt, with slow motion and a text pulse
src/Eat.*           Survival Mode hunger: eats the cheapest suitable food by equipping it (SMI lowers hunger)
src/WeaponSwap.*    ranged weapon when the enemy is far or fleeing, melee weapon when it is near (favourites, then strongest)
src/Execute.*       Valhalla Combat execution: prompt only while its key would execute; presses its key (F15 in prompt-only mode)
src/Jujutsu.*       vanilla H2H kill move on a guarding humanoid; KillActorHandler hook keeps the victim alive
src/Settings.*      per-module switches, prompt keys, prompt-only switches, eat stage, weapon swap distance and Dress reach, saved to Data/SKSE/Plugins/CIGAR.json
src/Panel.*         SKSE Menu Framework pages (CIGAR / 1. 모듈, 2. 단축키, 3. 세부 설정): switches and live status, keys, module options
include/TDM/        True Directional Movement API, V1 part (ersh1/TrueDirectionalMovement @ 57b913a)
include/ValhallaCombat/  Valhalla Combat API, V2 part (BSD-3, D7ry/valhallaCombat)
include/SkyPrompt/  SkyPromptAPI header (MIT, QTR-Modding/SkyPromptAPI @ cb4e551)
lib/commonlibsse-ng alandtse/CommonLibVR branch ng (submodule)
tools/Build.ps1     build (VS 2026 Build Tools, Ninja, vcpkg at C:\TAKEALOOK\TOOLS\vcpkg), deploy, verify
tools/register_profile.py  enable the CIGAR mod in MO2 and drop old plugin entries (MO2 closed)
tools/sync_si_settings.py  keep replaced SI modules off and SI on the Power User preset
tools/verify_deploy.py     deployment assertions (exit 1 on any failure)
```

The mod is deployed to `C:\TAKEALOOK\mods\CIGAR\SKSE\Plugins\CIGAR.dll`
(with its PDB), directly above `[NoDelete] 0008 StreamlinedInteractions` in
MO2.

## Build

```powershell
git submodule update --init --recursive
powershell -ExecutionPolicy Bypass -File C:\TAKEALOOK\TKL-Agent\CIGAR\tools\Build.ps1 -Deploy
```

- The first build installs the vcpkg dependencies and compiles CommonLibSSE-NG,
  which takes a while. Later builds are incremental.
- Skyrim must be closed to deploy; MO2 may stay open.
- The script leaves no build servers behind: it uses Ninja, `/Z7` debug info,
  no telemetry, and stops any `mspdbsrv`/`vctip`/`MSBuild` it started.
- On a fresh profile, run `python tools\register_profile.py` once with MO2
  closed.

Licensing note for any future distribution: CommonLibSSE-NG (alandtse) is
GPL-3.0-or-later, so a distributed `CIGAR.dll` would have to ship under a
GPL-compatible license with its source.

## Runtime diagnostics

The plugin writes `CIGAR.log` next to `skse64.log`
(`Documents\My Games\Skyrim Special Edition\SKSE\`). The file is recreated on
every launch and records:

- the SkyPrompt client id;
- the integrations found (or why a module is idle);
- every change of each module's prompt-gate inputs;
- each prompt offered and every SkyPrompt event;
- worn slots when entering water, a bed or a wardrobe;
- the result of every action, including BiS's own `TryWashActor` return value.

A HUD notification appears when SkyPrompt is missing, when BiS is disabled in
its MCM, or when a replaced SI module has been switched on again.
