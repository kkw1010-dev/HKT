# 007 · Control panel (SKSE Menu Framework)

CIGAR adds a page to SKSE Menu Framework (F1 by default), the way Streamlined
Interactions (SI) does. The framework stays optional: without it CIGAR runs as
before and the log says there is no panel.

## How SI does it (checked 2026-09-17)

- `StreamlinedInteractions.dll` has no import from `SKSEMenuFramework.dll`. It
  holds the strings `SKSEMenuFramework.dll` and `AddSectionItem`, so it looks
  the framework up at run time with `GetProcAddress`.
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
missing with no error. SI does not hit this because `StreamlinedInteractions`
sorts after `SKSEMenuFramework`. The vendored copy replaces the static with
`SKSEMenuFramework_Module()`, which is looked up when first used.
`CIGAR::Panel::Register()` runs at `kPostLoad`, after every plugin has loaded.

## What the pages show

Since 2026-09-18 the section `CIGAR` has three pages. The framework lists
items by name, so the numbers fix their order:

| Page | Contents |
|---|---|
| `1. 모듈` | 모듈 (switches, gate, last line) and 상태 |
| `2. 단축키` | 프롬프트 키 and 모드 단축키 |
| `3. 세부 설정` | 먹기 (start stage) and 탈의·착용 (reach) |

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
  it there would also move SI's and Grapple's keys.

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
