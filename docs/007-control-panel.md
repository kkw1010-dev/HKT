# 007 · Control panel (SKSE Menu Framework)

CIGAR adds a page to SKSE Menu Framework (F1 by default). The framework stays optional: without it CIGAR runs as
before and the log says there is no panel.

## The framework (checked 2026-09-17)

- CIGAR has no import from `SKSEMenuFramework.dll`; it looks the framework up
  at run time, so the panel is optional.
- The installed framework is 3.8.0 (Nexus 120352). It exports
  `AddSectionItem`, `AddWindow`, `RegisterHudElement`, `RegisterInpoutEvent`
  (sic), `GetMenuFrameworkVersion` and the whole cimgui surface (`ig*`, `Im*`).
- The client header is `include/SKSEMenuFramework/SKSEMenuFramework.h`, vendored
  from `Thiago099/SKSE-Menu-Framework-2-Example` @ `cd1f277` (MIT). All 1,384
  names it resolves are exported by 3.8.0. The framework DLL itself is
  "all rights reserved" and is never copied.

## The patch to the vendored header

Upstream resolves the framework handle when the DLL is loaded, before any
function runs:
`static auto menuFramework = GetModuleHandle(L"SKSEMenuFramework");`.
SKSE loads `CIGAR.dll` before `SKSEMenuFramework.dll`, so that handle would be
null. Every wrapper would then silently do nothing, and the panel would be
missing with no error. The vendored copy replaces the static with
`SKSEMenuFramework_Module()`, which is looked up when first used.
`CIGAR::Panel::Register()` runs at `kPostLoad`, after every plugin has loaded.

## What the pages show

Since 2026-09-18 the section `CIGAR` has three pages. The framework lists
items by name, so the numbers fix their order:

| Page | Contents |
|---|---|
| `1. 모듈` | 모듈 (switches, gate, last line) and 상태 |
| `2. 단축키` | 프롬프트 키 and 모드 단축키 |
| `2. 단축키` note | prompt-only switches now also cover Valhalla's execution key (F15) |
| `3. 세부 설정` | 먹기 (start stage), 무기 전환 (switch distance, default 800) and 탈의·착용 (reach) |

Each page logs `control panel: page … drawn for the first time` once.

- **모듈**: one switch per module in `Modules()`. Modules the panel has no
  label for still get a switch under their own name. Under each switch the
  panel shows:
  - the optional integration it needs;
  - the last gate (`조건`);
  - the last log line (`최근`), which also explains an idle module, for
    example "Bathing in Skyrim - Renewed not found".
- **프롬프트 키**: the keyboard keys of CIGAR's four prompt key slots
  (default 1, 2, 3, 4, SkyPrompt's own default), chosen from a list of scan
  codes, with a reset button. See "Prompt keys" below. The panel warns when
  two slots share a key, or when a slot equals Grapple's or Acheron's
  surrender key.
- **모드 단축키**: prompt-only switches for Grapple and Acheron's surrender
  (default on), the current keys, and a 모드 키 다시 확인 button that re-reads
  both mods' keys. See "Prompt-only mode" in `docs/005-lockon.md`.
- **탈의·착용**: the bed/wardrobe reach (100–600, default 250) that used to be
  `kPlaceRange` in `Dress.cpp`. It is saved when the slider is released.
- **상태**: whether SkyPrompt is connected, and where the settings came from.

A module that is switched off skips `Tick()` and `FastTick()`, and its prompts
are taken off the screen at once (`Prompts::WithdrawAll`). It then gets
`OnDisabled()`, which undoes anything that outlives a prompt: `Surrender` ends
its slow motion, and `Deflate` releases a held key. `OnGameLoaded()` still runs, so
switching it back on works without a reload. The switches are not in the
co-save, so no new game is needed.

## Prompt keys

SkyPrompt 2.3.15 (source: `QTR-Modding/SkyPrompt` @ `9ac377a`, 2026-03-04)
takes an optional per-prompt key list, `Prompt::button_key`, as pairs of
device and key:

- `InteractionButton::GetKey()` uses the key listed for the current device.
  Otherwise it falls back to `settings.json` `keys[device][index]`, where
  `index` is the position of the prompt's sub-manager (one per event ID, at
  most `n_max_buttons` = 4 per client).
- The keys are fixed when a prompt is queued. Re-sending a queued prompt
  updates only its text, colour and progress.
- `settings.json` holds one key list for every SkyPrompt client, so changing
  it there would also move every other client's keys (Grapple's, for one).

CIGAR therefore lists a keyboard key for every prompt it sends
(`PromptSlot::Offer`):

- **Slots.** A prompt takes the key slot its event ID already holds, or the
  lowest free one, and keeps it until the prompt is withdrawn or times out.
  So the prompts on screen always have distinct keys, and the first one to
  appear gets slot 1.
- **Gamepad.** No gamepad key is listed, so gamepads keep SkyPrompt's
  defaults.
- **Changing a key.** `Settings::SetPromptKey` saves the file and takes every
  CIGAR prompt off the screen (`Prompts::WithdrawEverything`). Each prompt is
  offered again on its next tick with the new key.
- **Log.** Every `offer` line in `CIGAR.log` shows `slot=` and `key=` (the
  scan code).

## Settings file

`Data/SKSE/Plugins/CIGAR.json`, read through MO2's VFS:

```json
{ "dress": { "placeRange": 250.0 }, "modules": { "Dress": { "enabled": true } },
  "prompt": { "keys": [2, 3, 4, 5] },
  "promptOnly": { "grapple": { "enabled": true, "manualKey": 34 } } }
```

A missing or broken file means the defaults (everything on, keys 1–4).
A prompt key outside 1–255 keeps that slot's default, and the log says
which one applied. `verify_deploy.py` fails on invalid or repeated prompt keys. `tools/Build.ps1 -Deploy` copies `dist/CIGAR.json` into the
mod folder only when none is there. That keeps the panel's saves in
`mods\CIGAR` instead of MO2's overwrite, and never resets the player's choices.

## Self-reporting

- Build time: `tools/check_menu_framework.py`, run by `Build.ps1` after every
  build, fails the build when any of these hold:
  - the lazy-handle patch is missing (checked against the unpatched upstream
    header: it fails as intended);
  - a function the header or `Panel.cpp` resolves is not exported by the
    installed framework;
  - the winning `SKSEMenuFramework.ini` does not enable Korean;
  - the primary font has no Hangul. In this modlist the winner is
    `TAKEALOOK - Font Edit`; the framework's own ini has
    `EnableKorean = false`.
- `CIGAR.log` records one of these:
  - `control panel: registered CIGAR/{1. 모듈, 2. 단축키, 3. 세부 설정} …`
  - `… SKSE Menu Framework is not loaded; no panel`
  - `… lacks AddSectionItem/igCheckbox`

  It also records `control panel drawn for the first time` when the page is
  first opened, every switch change, every settings save or write failure, and
  the loaded values.

## Not verified yet

Only a running game can show these; the log lines above answer them in one
launch:

- that registering at `kPostLoad` is early enough for the framework;
- that the page draws;
- that Hangul renders.

## Merging with parallel work

This was built on branch `feat/menu-panel` (worktree `..\CIGAR-menu`) while
another session was adding `BaboKey`/`LockOn` on `master`. The overlapping
edits are small and sit apart from that work:

- `src/main.cpp`: `Panel.h`/`Settings.h` includes, a `Settings::Enabled` check
  in `RunTick`, and a `kPostLoad` case.
- `src/Module.h`: `Log`/`LogGate` also keep a locked copy for the panel
  (`ShownLine`/`ShownGate`).
- `src/Prompt.h/.cpp`: `Prompts::WithdrawAll` and a slot registry filled in the
  `PromptSlot` constructor.
- `src/Dress.cpp`: `kPlaceRange` → `Settings::PlaceRange()`.
- `tools/Build.ps1`: runs the panel checks, and deploys the default
  `CIGAR.json`.

Merged into `master` on 2026-09-17, together with `Deflate` and `Surrender`.
The only conflict was in `RunTick`, which now skips both `FastTick()` and
`Tick()` for a switched-off module. `Panel.cpp` has Korean labels for all six
modules. New modules need nothing extra: they appear through `Modules()`, and
a module without a label is shown under its own name.

## The release variant (2026-09-20)

Confirmed in game on 2026-09-20 with the release build: the module page showed
the descriptions and no `조건:` / `최근:` line. The panel logs nothing about its
own contents, so that was checked by eye rather than from `CIGAR.log`.

The panel has two faces, chosen at compile time by the `CIGAR_RELEASE` option
(`cmake --preset dist`):

| | author build (default) | release build |
|---|---|---|
| switch label | `목욕 (Bathe)` | `목욕` |
| under it | what the module does, the integration it waits for, the live `조건:` gate string and the last `최근:` log line | what the module does, and the integration it waits for |
| 상태 block | SkyPrompt, the settings-file path, the log path | SkyPrompt, and a line asking for the log when reporting a problem |

The gate string and the log line are author-side diagnostics: they read
`combat=true locked=false movable=true quiet=false`, which explains a missing
prompt to whoever wrote the module and to nobody else. A player gets a sentence
saying what the module is for instead, which is what the release build ships.

The descriptions live in the `kLabels` table next to each module's title, so a
new module adds its own in the same place (see `docs/000-adding-a-module.md`).

`tools/make_release.py` refuses to package a DLL that still carries the
`조건: %s` format string, so an author build cannot be shipped under a release
name by mistake.

## Release panel text (the user, 2026-09-25, CIGAR 2.0)

The release build keeps the 1.0 rule: the panel shows what each feature does and nothing technical.
Beyond the author-only gate and log lines, the release now also hides, on page 2, the hidden-key
codes, the remembered manual keys and the current key summary (each prompt-only switch reads one line:
on, the mod's own hotkey is freed and the action is prompt-only; off, the mod keeps its hotkey), and
on page 3 the potion-detection, stagger-gauge and game-speed internals, replaced by plain sentences.

## Language and editions (2026-09-26)

Every panel string has Korean and English (`src/Text.*`); page 1 has the language choice, and the panel
is registered at kDataLoaded so the page titles follow the resolved language. The 2.0.1 Nexus edition hid
the prompt-only section and the other-mod options; since 2.1.0 there is one release build, which shows
them. See `docs/035-nexus-edition.md`.
