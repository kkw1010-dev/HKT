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

Since 2026-09-27 the section `CIGAR` has five pages (three until then, with every module on
`1. 모듈`). The framework lists items by name, so the numbers fix their order:

| Page | Contents |
|---|---|
| `1. 전투` | CIGAR's own combat modules (switches, gate, last line) |
| `2. 비전투` | CIGAR's own non-combat modules |
| `3. 모드 연동` | modules that idle without another mod |
| `4. 단축키` | 프롬프트 키 and 모드 단축키; the prompt-only switches also cover Valhalla's execution key (F15) |
| `5. 세부 설정` | 상태, 언어, 프롬프트 위치 (third-person right offset, default 15), 먹기 (start stage), 무기 전환 (switch distance, default 800) and 탈의·착용 (reach) |

Which module goes on which page is in "Module pages" below.

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
- **프롬프트 위치**: 오른쪽 간격 (3인칭), a slider over 0 / 10 / 15 / 20 / 30, default 15.
  Saved as `prompt.rightOffset` when the slider is released; it moves the prompts from the next
  frame. See "Prompt placement" below.
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

### Rejected: modifier keys and arrow keys (the user, 2026-09-27)

A Nexus user (IAMTOKKO, 2026-09-26) asked for modifier-key combinations and arrow keys as
prompt keys. The author first answered that they were "on my list". The user then **rejected
both.** Their reason: other mods that show SkyPrompt prompts must be able to take the arrow keys,
Grapple's QTE for one, so CIGAR must not claim them. SkyPrompt's own `settings.json` also uses
Left and Right (`cycle_L` 203, `cycle_R` 205) to cycle between prompts. Do not add either. The
correcting reply is drafted in `dist/nexus-page/replies-2026-09-27.md`.

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

## Prompt placement (2026-09-27; done, N6 and N7 passed in game)

The user asked for three things:
- in third person, prompts a little farther from the face
- in first person, prompts right under the crosshair
- a smooth move between the two when the camera switches

What decides the position today:

- **SkyPrompt places every prompt.** CIGAR only chooses the `refid` it sends (`Prompt.cpp`
  sends the player, 0x14). SkyPrompt API 2.0 (`include/SkyPrompt/API.hpp`, identical to
  `QTR-Modding/SkyPromptAPI` main on 2026-09-27) has no position or offset field.
- **Attached prompts** (SkyPrompt 2.4.0 `src/Renderer.cpp` `GetAttachedObjectPos`). For an actor,
  the anchor is the head node, moved 15 units × scale sideways from the camera. On screen it is then
  moved right by the prompt size + 10 px, and the theme's `marginX`/`marginY` are subtracted.
  SkyPrompt recomputes this itself every frame.
- **Unattached prompts** (`refid` 0) sit at the theme's fixed point: `xPercent × width − marginX`,
  `yPercent × height − marginY`. This modlist has 0.78 / 0.775 / 0 / 0, in `TAKEALOOK - MCM and
  INI/SKSE/Plugins/SkyPrompt/settings.json`.
- **All of these are theme values.** They belong to the player's SkyPrompt theme and apply to every
  SkyPrompt client (Grapple's QTE, Camping++, Read It Now), not to CIGAR alone.
- **A queued prompt keeps its `refid`.** Changing the anchor means withdrawing the prompt and
  offering it again, and SkyPrompt then jumps. CIGAR cannot feed a per-frame screen position.
- **First person** can be detected with `RE::PlayerCamera::IsInFirstPerson()` (CommonLib).

### Probe: a marker ahead of the head (2026-09-27, `src/PromptAnchor.*`)

The user chose a third-person-only test. First person keeps attaching to the player.

- **The marker.** One disabled `XMarker` (Skyrim.esm 0x3B), placed once per save with
  `PlaceObjectAtMe(base, forcePersist=true)` and kept through the co-save record `ANCH` v1.
  - At load CIGAR reads the marker's `GetBoundMin/Max`, the values SkyPrompt's `GetOBB` uses. If
    they are zero, it enables the marker and moves it with `SetPosition`. If those are zero too,
    prompts stay on the player. Every step is logged.
- **Every frame.** A `PlayerCharacter::Update` vtable hook (slot 0xAD, chained as Acheron and
  Grapple do) moves the marker to head + facing direction × d. It subtracts SkyPrompt's lift
  (bounds top + 10 = 26 units), so the prompt is drawn at head height. The disabled marker's
  `data.location` is written directly; no engine call.
- **Attachment.** `PromptSlot::Offer` takes the `refid` from `PromptAnchor::RefID()`: the marker
  in third person, the player in first person or without a marker. On a switch, every CIGAR prompt
  is withdrawn and offered again, because SkyPrompt keeps a queued prompt's reference.
- **Probe controls.** The author panel's "프롬프트 위치 탐색 (3인칭)" chooses d = 0 (the player,
  as before), 25, 40 or 60; the default is 40. The final build fixes one value and drops the
  panel item.
- **Open:** whether the forward offset covers the face when the camera looks at the character from
  the front. In-game test: `TEST-next-ingame.md` N6.

### Hardening plan (designed 2026-09-27; applied after N6 passed, see below)

The user decided to keep the marker approach if N6 shows it reads naturally. The probe then
becomes the real implementation, with these changes. Items marked *[verify]* rest on an inference
that the probe's own logs can confirm.

- **Marker life.**
  - One marker per save, reused through `ANCH`.
  - At load, the co-saved FormID is accepted only if it resolves to a reference whose base is
    `XMarker` 0x3B and which is not deleted; otherwise a new one is placed and the old ID logged.
  - Footprint: one disabled reference with no 3D and no script. If CIGAR is removed it stays as an
    inert disabled marker; this is to be stated in the README.
  - Deleting it at every save and placing it again was considered and rejected: `kSaveGame`
    arrives on the saving thread, before the write, where changing world objects is unsafe.
- **Cells, loading screens, fast travel.**
  - SkyPrompt reads a disabled reference's position from `GetPosition()`, because
    `GetCurrent3D()` is null. The marker's cell therefore does not matter.
  - The marker is persistent, so it stays in memory across cell changes *[verify: after
    interior/exterior changes and fast travel, `marker.get()` still resolves; the sample line
    shows it]*.
  - The update hook does not run during loading screens, and CIGAR's prompts are off in blocking
    menus, so nothing is drawn stale.
- **Riding, sitting, transformation, bleedout, scenes.**
  - The facing comes from the player's own `GetAngleZ()` (the horse's heading when mounted) and
    the head node from its middle-high process. When the node is missing (a transformation in
    progress), that frame is skipped.
  - In scenes and killmoves CIGAR's prompts are hidden anyway; the marker keeps following
    harmlessly.
- **Recovery and fallback.**
  - Every frame the handle is resolved. If it fails, prompts switch to the player at once (one WARN
    in the log), and a new marker is placed on the game thread, at most once every 10 s.
  - If placement fails three times in a session, the player attachment stays until the next load,
    with one notification.
- **Hook order.**
  - `write_vfunc` returns the previous function and CIGAR always calls it first. That makes the
    order with Acheron and Grapple (both hook slot 0xAD and chain) irrelevant.
  - The risk is a later mod that replaces the slot without chaining; the marker would then stop
    moving silently. Self-check: the hook counts frames. If none arrive for 5 s while the game runs
    unpaused, CIGAR logs a WARN, notifies once, and attaches prompts to the player.
- **Probe removal.**
  - Remove the panel item "프롬프트 위치 탐색 (3인칭)" and `kCandidates`.
  - `d` becomes a constant with the N6 winner, recorded as the user's choice.
  - The 5-second `sample:` line is dropped. The attach switches, marker placement and warnings
    stay in the log.
- **Unchanged:** first person attaches to the player (the user's choice).

### Adopted after N6 (2026-09-27)

N6 passed: the marker ahead of the head worked, and the user wants the prompt a little further
right. Applied (built; deployment waits while the MO2 session holds `mods\`):

- **Forward.** Fixed at 40 units (`PromptAnchor::kForward`), the user's choice from N6.
- **Right offset.** Along the camera's right, the side SkyPrompt itself uses for actors. At this
  point it was an author-panel probe offering 0 / 10 / 15 / 20 / 30; N7 settled it as a player
  setting (next section).
- **The hardening plan above, as written.**
  - Validation of the co-saved marker (base XMarker, not deleted).
  - Per-frame handle check, with an immediate fallback to the player.
  - Recovery placing at most once every 10 s, giving up after 3 failures with one notification.
  - The frame counter: 5 s without the update hook while playing gives a WARN and a notification,
    and prompts go to the player.
  - Head-node gaps skip the frame.
  - The forward candidates and the 5-second `sample:` log are gone.

N7 draft (in `TEST-next-ingame.md`):
- In third person, compare the right offsets 0 / 10 / 15 / 20 / 30 from the panel.
- Check the view from behind, from the side and from the front.
- Pick one; it becomes the fixed value.
- Also exercise fast travel, an interior/exterior door and riding a horse. `[PromptAnchor]` must
  show no `marker is gone` or `WARN` line; recovery lines would appear there.

### Finished after N7 (2026-09-27)

N7 passed in game: all five steps, with the prompts following the character after fast travel, an
interior/exterior door and riding a horse. PromptAnchor is a finished feature.

- **The right offset stays the player's choice.** The user saw no need to pick one value ("이거
  굳이 뭘 골라야 할 필요가 있나? 그냥 선택하게 두는 게 더 좋은 것 같다"). The candidates 0 / 10 / 15 /
  20 / 30 and the default 15 are the user's decision.
  - Panel: `3. 세부 설정` → 프롬프트 위치 → 오른쪽 간격 (3인칭), in every build.
  - Stored in `CIGAR.json` as `prompt.rightOffset`, snapped to the nearest step on load
    (`Settings::kPromptRightSteps`, `Settings::PromptRight`).
  - `Settings` passes it to `PromptAnchor::SetRight` at load and on every change; the log line is
    `[PromptAnchor] right offset set to N`. The author build also shows the marker status under the
    slider.
- **Forward** stays the constant 40 (`PromptAnchor::kForward`), the user's choice from N6.
- **For the release:** the README and the Nexus description must say that one disabled marker
  reference is kept per save, and name the new option.

## Module pages (2026-09-27)

The user asked for the module list to be split: CIGAR's own modules into combat and non-combat, and
the mod integrations on their own page. `Panel.cpp` decides the page of each module (`PageOf`):

- **3. 모드 연동:** the module idles without another mod, which is exactly a label with a `needs`
  entry. The page says they idle without the mod and can stay on.
- **1. 전투:** the prompt shows in combat or with a weapon drawn (`kCombatModules`).
- **2. 비전투:** everything else. A module without a label lands here under its own name, with a
  WARN in the log.
- A module whose prompts show in both goes by what it is for.

| Page | Modules |
|---|---|
| 1. 전투 | 무기 전환, 유술, 물약, 독 바르기 |
| 2. 비전투 | 탈의·착용, 퀘스트 추적, 획득 장비 착용, 책 읽기, 무기 충전, 의자에서 마시기, 투구 벗기·쓰기, 주시하기, 파티 의상, 마네킹 의상 교환, 퀘스트 행동, 앉기·눕기·기대기 |
| 3. 모드 연동 | 목욕, 납치 행동 선택, 록온, 그래플, 배출, 항복, 먹기, 처형, 용변, 마검사 모드 |

The borderline calls:
- 물약 shows in and out of combat; it is combat because health comes first.
- 투구 puts the helmet on in combat and takes it off otherwise; it is non-combat, because it exists
  to show the face.
- 무기 충전 shows only out of combat, so non-combat.
- 유술 uses Valhalla Combat when present but works without it, so it is CIGAR's own.
- 먹기 needs Survival Mode, official content but idle without it, so an integration.

The log line at registration lists every page's modules
(`control panel: combat page …; non-combat page …; integrations page …`). A name in
`kCombatModules` that matches no module gives a WARN.

상태 and 언어 moved from the old `1. 모듈` page to the top of `5. 세부 설정`.
