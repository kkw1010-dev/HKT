# CIGAR — session handoff (updated 2026-09-26)

## Start here

Reply to the user in Korean. Read this section, then `README.md`. The design rationale and the test
history of each module are in `docs/`, one file per module, and each file starts with its status.

### State

- **CIGAR 2.0.1 is released locally** (2026-09-26): `%USERPROFILE%\Downloads\CIGAR 2.0.1` and
  `CIGAR 2.0.1.7z` (README in Korean with the known issues). 2.0.1 is 2.0.0 made standalone (below);
  gameplay is unchanged. The 2.0.0 package, archive and `mods\CIGAR 2.0.0` copy were deleted, since
  they carried the old coupling. The user tested 2.0.0 in game with no problems (2026-09-25); 2.0.1 was
  not played. GitHub (`kkw1010-dev/HKT`, public): `master` and `v2` both hold the 2.0.1 source; there
  is no GitHub release page (the user's choice).
- **Nexus edition built, untested in game** (2026-09-26, `docs/035-nexus-edition.md`):
  `Downloads\CIGAR 2.0.1 Nexus` + `.7z` (15 base-game modules, no other-mod references,
  English/Korean). All builds now have Korean/English text (`src/Text.*`, auto from the game's names)
  and adjustable 유술 stamina/health damage. The standard `CIGAR 2.0.1` package was rebuilt with these
  too. Both are copied into `mods\CIGAR 2.0.1` and `mods\CIGAR 2.0.1 Nexus`; MO2 was open, so the
  Nexus folder has no `modlist.txt` row yet (MO2 lists it disabled on refresh). To test the Nexus DLL:
  enable `CIGAR 2.0.1 Nexus`, disable `CIGAR`, and read the load line in `CIGAR.log`.
- **CIGAR 0.2.0 is gone** (2026-09-26, the user's request): the mod folder and its `modlist.txt` row
  are removed, and so is the `release/0.2.0` branch, locally and on GitHub.
- **The Nexus build is in the load order for its first test** (2026-09-26): `+CIGAR 2.0.1 Nexus`,
  `-CIGAR`, `-CIGAR 2.0.1` (backup `modlist.txt.bak_20260926_nexus-test`). `Build.ps1 -Deploy` fails
  `verify_deploy.py` until `+CIGAR` / `-CIGAR 2.0.1 Nexus` are set back with MO2 closed.
- **The author build is in the load order**: MO2 profile `TKL - MUNG ADDON` has `+CIGAR` and
  `-CIGAR 2.0.1`, so `tools\Build.ps1 -Deploy` works. The release copy is `mods\CIGAR 2.0.1`.
- **CIGAR is a standalone system (2026-09-26).** It reads, writes and checks nothing of any other
  interaction mod: no settings reader, no deploy sync, no profile anchor, no duplicate-module warning.
  Do not add such coupling back; overlapping prompts from another mod are that modlist's problem.
- Grapple's NPC grapples and CIGAR's Grapple module are on.
- Untracked files in the repo root (three analysis `.md` files by other agents, and `scratch/`) are
  not ours; they were never committed. Leave them.

### Next: CIGAR 3.0, first feature MannequinSwap

- Plan: `docs/030-mannequin-swap.md` (facts read from the load order: `MannequinActivatorSCRIPT` by
  Another Mannequin Script Fix, 20 base-form slots, the `MannequinActivateTrig` activate parent;
  PromptID 40; one build with self-verifying logs, one test session).
- **Branch `v3`** holds all 3.0 code; do not build 3.0 on `master` or `v2`.
- **Decisions made** (the user, 2026-09-25; `docs/030`, "The user's decisions"): the stowed helmet
  goes with the outfit and the mannequin's helmet arrives stowed; `Dress` remembers the post-swap
  outfit; the Almsivi CC mannequins are included. Nothing is left to ask; the next step is the build.
- New modules follow `docs/000-adding-a-module.md` and the user's rules in memory, including
  `cigar-declined-prompt-stays-hidden` (a double-tap decline hides the prompt until the situation
  changes; hold-and-keep prompts must read the double tap themselves).

### Standing facts worth keeping in view

- The user's rule (2026-09-25, `docs/000-adding-a-module.md` section 3): a hotkey CIGAR takes over
  is removed from the mod **and** from MCM Memory's profile. Applied to FHU (82 -> -1), Acheron's
  surrender key (37 -> 101, CIGAR's hidden F14) and PNO's urinate key (51 -> -1); `verify_deploy.py`
  checks these rows and fails on a cancelled-remap Esc.
- **Deferred by the user:** QuestAction's in-game test (`startquest MQ105` did not start the quest;
  `docs/025`).
- Party Sheet's HUD vanishing while seated is Party Sheet's own gate on disabled fighting controls
  (the game's sit turns `fighting` off); nothing in CIGAR causes it. **Out of CIGAR's scope** (the
  user, 2026-09-25): Installation and Modification case 032.
- CIGAR's lighting work is abandoned; TCL runs on its own hotkey (memory `cigar-no-light-module`).
- The user's plan after that: a fishing module on Streamlined Fishing. Prep is in
  `docs/027-fishing.md`: on this modlist Fish Anywhere's loose script already overrides Streamlined
  Fishing's rod equip, and Fishing Preview's overrides SFO's animations. The module waits on the
  decisions listed there.
- CIGAR's design philosophy (the user's, 2026-09-24): hotkey terminator, one-button interaction,
  UX sacrosanct; no player-authored rule framework (`docs/022`); a prompt performs the whole action,
  and nothing vanilla already does in one press is duplicated.

## Open items, in priority order

1. **Jujutsu / 유술 is guard-break only again.** On 2026-09-20 the user dropped the perfect-parry
   feature ("패리 유술은 유기한다") and asked for a rollback to the most stable guard version. `src/Jujutsu.cpp`
   and `src/Jujutsu.h` are back to the code test 12 ran (20 of 20 presses played), with one change kept:
   the Valhalla stun share is the panel's value, default 15% (the user's choice). Removed with the
   feature: parry detection, the slow motion, the victim's recoil/stagger reset, the player's block
   suppression, the idle rotation and the forced idle state. `docs/012-jujutsu.md` keeps the full test
   history (tests 9-19) so none of it has to be re-derived.
   - Why it was dropped: after a parry the engine refused the paired idle 29 times out of 30, and no
     state told the plays from the refusals (block, recoil, time multiplier, idle and distance were all
     ruled out with per-try logs). The last build also broke NPC combat AI (they moved but stopped
     attacking), most likely from `IdleForceDefaultState` / `recoilStop` sent to the victim.
   - Passed in game on 2026-09-25 (the user); the remaining refusals are accepted as unresolved.
2. **Test environment, settled 2026-09-20** (the reasoning is in `docs/012-jujutsu.md`):
   - NPC grapples are **on again**: `mods\Grapple\SKSE\Plugins\FH_Grapple_Plugin.ini`
     `bEnableNPCGrapple = true`, byte-identical to
     `build\FH_Grapple_Plugin.ini.bak_20260919_npcgrapple`.
   - `mt_behavior.hkx` **stays the pre-PNO copy** (md5 `cb3a5b03...`), and **Pandora must not be
     run.** Private Needs needs no run: the 15:40 output already covered its FNIS list, PNO's
     animations do not pass through `mt_behavior.hkx` (neither copy holds one `Private`/`Needs`
     string; its list resolves through `0_Master.hkx` into the `FNIS_Private_Needs_Behavior.hkx`
     the mod ships), and the two copies of `mt_behavior.hkx` hold the *same multiset of strings*
     in a different order. A run would rewrite that order, which is what tests 9-12 tie to the
     유술 refusals.
   - **Guard.** `tools/behaviour_baseline.json` records that file's md5 and `verify_deploy.py`
     now fails the build when it changes, naming the spare to copy back
     (`build\mt_behavior.jujutsu-good.hkx`; the 15:40 output is still
     `build\mt_behavior.pandora-20260919-1540.hkx`). After a future Pandora run and a fresh 유술
     test, record the new file with `python tools/verify_deploy.py --accept-behaviour`.
3. **`Potion` is confirmed in game (2026-09-20).** Health and stamina prompts were offered, drunk
   and cleared; the order moved on to the next need by itself. **Untested:** 해독, 질병 치료,
   수중 호흡, and the 체력 위급 threshold picking the strongest bottle. Test 1 the same day
   found the module showing nothing because the filter required `IsMedicine()`, a flag set on 27
   ALCH records in this whole order and on none of the healing potions; see `docs/015-potion.md`.
4. **`QuestTrack` (2026-09-21).** It listens for an objective-state
   transition to displayed, offers the untracked quest as a 15-second hold prompt, and dispatches the native
   Papyrus `Quest.SetActive(true)` method. Hold and tracking are confirmed in game. A nameless
   miscellaneous QUST now falls back to the new objective text instead of exposing its FormID;
   that label fix is confirmed in game. See `docs/017-quest-track.md`.
5. **`ItemEquip` (2026-09-21).** A newly acquired playable weapon
   or armor piece gets a 15-second hold prompt outside combat. It uses `TESContainerChangedEvent`
   and revalidates ownership and equipped state before calling `ActorEquipManager::EquipObject`.
   Passed in game on 2026-09-25. See `docs/018-item-equip.md`.
6. Backlog below.

`Auto Input Switch` is the only gamepad mod enabled; see `docs/016-gamepad.md`.

Confirmed in game on 2026-09-20 (`SKSE\CIGAR.log`, 20:22-20:46), release build:
- 20 prompts offered, 11 accepted across 록온, 그래플, 유술, 소변 보기, no WARN in the run;
- the player-facing panel (no `조건:` / `최근:` line), by eye — the panel logs no contents;
- gamepad prompts, by eye — CIGAR logs only the keyboard key, so a pad accept and a keyboard
  accept are the same line. SkyPrompt supplies the pad button from its own settings.json.
CIGAR targets keyboard and mouse; the pad is not an audience (the user, 2026-09-20). Pad gaps
are known limits, not open items, and no gamepad feature belongs in the mod. See
`docs/016-gamepad.md`.

Confirmed in game on 2026-09-20 (`SKSE\CIGAR.log`, 18:24-18:32):
- the LockOn / Grapple split: both modules resolved on their own, 록온 and 그래플 were offered at the
  same time on different keys, both presses landed, and a grapple accepted while locked was followed
  by the re-lock (`re-lock after grapple ... pressed=true`, then `after re-lock press: locked=true`)
  while 록온 stayed off the screen for the whole wait. A second grapple ended with
  `re-lock dropped (not needed)`, the lock having come back by itself.
- **Not** guard 유술: 20 presses, 9 refused. That is well below test 12's 20 of 20, so the rollback
  did not restore the old rate; the refused tries show the victim moving and no longer blocking, the
  same pattern as tests 13-18.

Confirmed in game on 2026-09-19:
- the Execute prompt with actor names;
- the WeaponSwap return to the previous loadout, and the arrows re-equipped with the bow;
- Grapple after a new game;
- unique prompt IDs in the kidnap room;
- the three panel pages;
- the F13/F14/F15 prompt-only keys, with G and K doing nothing.
Execute sometimes misses the first press; the user accepts this. Test 7 suggests the same cause as 유술's
refusals (the stun-breaking swing still in progress), so `attackStop` could help there too. Not done yet.

## Where things stand

- **The plugin.** `CIGAR.dll` is an ESP-less SKSE plugin (CommonLibSSE-NG
  alandtse `ng`, one DLL for SE, AE and VR).
  - It is deployed to `C:\TAKEALOOK\mods\CIGAR\SKSE\Plugins\` and enabled in
    the MO2 profile `TKL - MUNG ADDON`.
  - The deployed DLL equals the build of `HEAD`, which `verify_deploy.py`
    checks.
- **The repo.** `C:\TAKEALOOK\TKL-Agent\CIGAR`, published on 2026-09-20 to
  `origin` = <https://github.com/kkw1010-dev/HKT> (**public**).
  - **Commit locally on every change; push only when the user asks.** That is
    the user's standing rule (2026-09-20), and having a remote does not soften
    it: commit as work lands, without being asked, and then stop. Never run
    `git push` on your own initiative, not even as the last step of a finished
    task — say the commit is local and unpushed instead.
  - Branch: `master`, tracking `origin/master`. The earlier `feat/menu-panel`
    worktree (`..\CIGAR-menu`) was merged and removed.
  - Only CIGAR is on that remote. The other 15 repos under `TKL-Agent` are
    still local only, by the user's choice on 2026-09-20. Do not give one a
    remote without being asked.
  - `lib/commonlibsse-ng` is a submodule, so only its gitlink is pushed; a
    fresh clone needs `git submodule update --init --recursive`.
  - CommonLibSSE-NG is GPL-3.0-or-later, so publishing this source is the
    licence-consistent direction; see the Pitfalls note.
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
| `LockOn` | 록온 | True Directional Movement | `005` |
| `Grapple` | 그래플 | Grapple (Patreon; usually absent) | `014` |
| `Deflate` | 배출 (길게 누르기) | Fill Her Up Baka Edition | `006` |
| `Surrender` | 항복 (길게 누르기) | Acheron (+ Yamete Kudasai consequences) | `008` |
| `Eat` | 먹기: <음식 이름> | Survival Mode + SMI (Starfrost, Gourmet) | `009` |
| `WeaponSwap` | 원거리 무기, 근접 무기 | — (TDM lock target when present) | `010` |
| `Execute` | 처형 | Valhalla Combat | `011` |
| `Jujutsu` | 유술 | — (Valhalla optional) | `012` |
| `Needs` | 소변 보기, 대변 보기 | Private Needs - Orgasm | `013` |
| `Potion` | 마시기: <물약 이름> | — | `015` |
| `QuestTrack` | 추적하기: <퀘스트 이름> | — | `017` |
| `ItemEquip` | 장착하기: <장비 이름> | — | `018` |
| `BookRead` | 읽기: <책 이름> | — (split from `ItemEquip`) | `034` |
| `Rest` | 앉기, 눕기, 기대기, 손 녹이기, 시간 보내기 | — | `019`, `020` |
| `Recharge` | 충전하기: <무기 이름> | — | `023` |
| `QuestAction` | 장착하기: <샤우트> | — | `025` |
| `ChairDrink` | 마시기: <술> | — | `026` |
| `Helmet` | 투구 벗기, 투구 쓰기 | — (clips from `CIGAR - Helmet Motions`) | `028` |
| `Poison` | 독 바르기: <독> | — | `033` |
| `Observe` | 주시하기: <대상> | — | `031` |
| `PartyOutfit` | 파티 의상 입기, 원래 장비로 | — (MQ201) | `032` |

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
  - It works by pressing TDM's key through `BSInputDeviceManager`
    (`Util::PressKey`), because TDM has no API to set the lock.
  - The prompt stays down while Grapple is taking the lock again.
- **Grapple** (split out of `LockOn` on 2026-09-20, because Grapple is a
  Patreon mod that most setups will not have while nearly all have TDM).
  - 그래플 shows in combat while locked, or while a hostile is within 350
    units; it presses Grapple's own hotkey the same way.
  - A grapple started while locked is followed by an automatic re-lock. For
    its whole wait the module raises `TDMLock::SetBusy(true)`, so the two
    modules never press TDM's key in the same frame.
  - At load, Grapple's `TargetLockKey` is synced to TDM's key (258) when TDM
    is present. Without TDM the module still offers the prompt on a hostile
    in reach.
  - Prompt-only mode (default on) moves Grapple's `Hotkey` to F13, so G is
    free. When it is off, an unset `Hotkey` (every new game) is restored from
    the remembered key or `FH_Grapple_Plugin.ini`. Keys are read only at load
    and on the panel's key check button; nothing polls.
  - `src/TDMLock.*` holds what the two modules share: TDM's API pointer, its
    lock key and that busy flag. Each module resolves it on every game load,
    so either can be switched off on its own.
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
- **Untested:** gamepad buttons (the user does not use a pad).

### Prompt input policy

- Non-combat contextual actions default to a hold, as protection against accidental state
  changes. Single press is reserved for timing-sensitive combat actions or an explicitly
  documented exception.
- `PromptSlot::SetPromptType(kHold)` selects the hold-to-accept interaction. Do not also call
  `SetHoldMode(true)` unless the module needs live key-down/key-up callbacks during the hold.

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
- **Commit every change locally as it lands; push to GitHub only on the user's
  explicit command.** See the repo note above.
- Player-facing Korean is short and administrative. Repo docs are in English.
  Reply to the user in Korean.

## How to work on it

```powershell
powershell -ExecutionPolicy Bypass -File C:\TAKEALOOK\TKL-Agent\CIGAR\tools\Build.ps1 -Deploy
```

- **Two build variants.** The default is the author build: the control panel shows each
  module's live gate string and last log line. `-Package` builds the `dist` preset instead
  (`CIGAR_RELEASE`), whose panel shows what each module does for the player and no
  diagnostics, and then assembles `%USERPROFILE%\Downloads\CIGAR <version>\` through
  `tools/make_release.py`. `-Deploy` and `-Package` cannot be combined: the mod folder keeps
  the author build. `make_release.py` refuses a DLL that still carries the `조건: %s` string,
  so an author build cannot ship under a release name.

- **Build.** VS 2026 Build Tools, Ninja, and vcpkg at
  `C:\TAKEALOOK\TOOLS\vcpkg`. An incremental build takes under a minute.
  Build.ps1 runs `check_menu_framework.py` and, with `-Deploy`,
  `verify_deploy.py`. `verify_deploy.py` fails on:
  - a stale DLL;
  - a missing dependency;
  - renamed script or property names in BaboDialogue, FHU, Grapple or
    Acheron;
  - an unpressable TDM or Acheron key;
  - a behaviour file in `tools/behaviour_baseline.json` whose md5 changed
    (a Pandora run), which is otherwise silent in game. `--accept-behaviour`
    records the current file once the module has been re-tested.
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
- **Perk entry points.** `BGSEntryPoint::HandleEntryPoint(ep, owner, ...)` takes one form per
  condition tab after the owner, then the result pointer. Read the tab count from a PERK that uses the
  entry point (`PerkConditionTabCount`); passing only the result pointer crashed the game
  (`kModPoisonDoseCount` has 3 tabs: owner, weapon, poison).
- **IED configs.** One form from a plugin that is not loaded makes IED reject the whole config file,
  with nothing on screen. `verify_deploy.py` checks the default config and the overwrite exports.
- **Decline on hold-and-keep prompts.** SkyPrompt's `kHoldAndKeep` sends only down / up, never a
  decline, for a double tap; a module that needs one reads two short taps itself (`Observe`).
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
5. **Private Needs.** Done as `Needs` on Private Needs - Orgasm (installed 2026-09-19); see item 2
   above.
6. **Tidy up (sweeping).** Needs `sweepingOrganizesStuff.esp`, which is not installed.
