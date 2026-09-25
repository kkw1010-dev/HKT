# CIGAR — session handoff (updated 2026-09-21)

## Resume here (2026-09-25, end of session)

- In-game on 2026-09-25, passed: ~ no longer runs `smp reset`; CEE's MCM keys empty; TCL on mouse 4;
  MCM Memory restore without failures.
- Built and deployed after that, **untested in game**: Fill Her Up in the prompt-only list
  (`docs/006-deflate.md`, last section). Panel checkbox, default on.
- The user's standing rule (2026-09-25, `docs/000-adding-a-module.md` 3a): a hotkey CIGAR takes over
  is removed from the mod **and** from MCM Memory's profile. Applied now to FHU (82 -> -1), Acheron's
  surrender key (37 -> 101, CIGAR's hidden F14) and PNO's urinate key (51 -> -1); `verify_deploy.py`
  checks these rows and fails on a cancelled-remap Esc.
- Passed in game later on 2026-09-25, reported by the user without logs: guard 유술 (its refusal
  rate is accepted as unresolved, the user's decision), ItemEquip (armor too), Recharge,
  ChairDrink, pass time in a chair, warm hands at a forge, prompts hidden over menus.
- **Still to test in game:** Fill Her Up prompt-only (below); Potion's 해독, 질병 치료, 수중 호흡 and
  the 체력 위급 pick; QuestAction (the Greybeards' Unrelenting Force).
- Party Sheet HUD while sitting: the `HUD mode` log lines show the vanilla HUD stayed in `All` at
  alpha 100 through three chair sits (04:00:34, 04:00:55, 04:01:04), so the vanishing is Party
  Sheet's own logic, not the game HUD. On 2026-09-25 `[HUDVisibility] Enabled` was set 1 -> 0 in
  `mods\Skyrim Party Sheet FHD Preset\SKSE\Plugins\PartySheet.ini` (backup
  `PartySheet.ini.bak_20260925_hudvisibility`, one byte differs); not yet seen in game.
- CIGAR's lighting work is abandoned; TCL runs on its own hotkey (memory `cigar-no-light-module`).
- Next, the user's plan: absorb Streamlined Fishing into CIGAR. Prep is in `docs/027-fishing.md`:
  on this modlist Fish Anywhere's loose script already overrides Streamlined Fishing's rod equip,
  and Fishing Preview's overrides SFO's animations. The module waits on the decisions listed there.
- CIGAR's design philosophy (the user's, 2026-09-24): hotkey terminator, one-button interaction,
  UX sacrosanct; no player-authored rule framework (`docs/022`); a prompt performs the whole action,
  and nothing vanilla already does in one press is duplicated.

Read this first, then `README.md`. The design rationale and the test history of
each module are in `docs/`, one file per module, and each file starts with its
status.

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
4. **`QuestTrack` starts the SI absorption pass (2026-09-21).** It listens for an objective-state
   transition to displayed, offers the untracked quest as a 15-second hold prompt, and dispatches the native
   Papyrus `Quest.SetActive(true)` method. Hold and tracking are confirmed in game. A nameless
   miscellaneous QUST now falls back to the new objective text instead of exposing its FormID;
   that label fix is confirmed in game.
   `QuestActions.enabled_track` is the only new SI switch CIGAR replaces; the rest of QuestActions
   remains enabled. See `docs/017-quest-track.md` and `SI/_ABSORPTION/_MAP.md`.
5. **`ItemEquip` continues the SI absorption pass (2026-09-21).** A newly acquired playable weapon
   or armor piece gets a 15-second hold prompt outside combat. It uses `TESContainerChangedEvent`
   and revalidates ownership and equipped state before calling `ActorEquipManager::EquipObject`.
   It replaces only `ItemUse.enabled_equip_weapon` and `enabled_equip_armor`; runtime test pending.
   See `docs/018-item-equip.md`.
6. Backlog below.

The **author/dev build** (`CIGAR`) is enabled and the release build (`CIGAR 0.2.0`) is disabled.
The dev mod folder's SI override is therefore active in the VFS; deploy sync keeps the absorbed
switches off while leaving the rest of each SI module intact.
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
    the MO2 profile `TKL - MUNG ADDON`, directly above
    `[NoDelete] 0008 StreamlinedInteractions`.
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
| `Potion` | 마시기: <물약 이름> | — (replaces SI's ItemUse potion actions) | `015` |
| `QuestTrack` | 추적하기: <퀘스트 이름> | — (replaces SI Quest Tracking) | `017` |
| `ItemEquip` | 장착하기: <장비 이름> | — (replaces SI weapon/armor equip) | `018` |
| `Rest` | 앉기, 눕기, 기대기, 손 녹이기, 시간 보내기 | — (replaces SI IdleActions) | `019`, `020` |
| `Recharge` | 충전하기: <무기 이름> | — (replaces SI weapon recharge) | `023` |
| `QuestAction` | 장착하기: <샤우트> | — (replaces SI QuestActions) | `025` |
| `ChairDrink` | 마시기: <술> | — (SI chair drink, narrowed) | `026` |

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
- **SI overlap.** `mods\CIGAR\SKSE\Plugins\StreamlinedInteractions\settings.json`
  turns off SI's Bathe, DressActions (water, bed, wardrobe), Quest Tracking, weapon/armor equip,
  and the six ItemUse potion actions, and pins SI's
  preset to Power User (2). `tools/sync_si_settings.py` enforces this on
  deploy, and SI keeps the switches off after its menu is opened.
- **Untested:** gamepad buttons (the user does not use a pad).

### Prompt input policy

- Non-combat contextual actions default to a hold, matching SI's protection against accidental
  state changes. Single press is reserved for timing-sensitive combat actions or an explicitly
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
  `sync_si_settings.py` and `verify_deploy.py`. `verify_deploy.py` fails on:
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
6. **SI IdleActions, absorbed as roleplay actions.** SI's `IdleActions.enabled` is off since
   2026-09-21 (the user's call), so every action below is gone from the game until CIGAR has it.
   The user wants each one kept, even where CIGAR has a similar module, because the point is
   roleplay rather than removing a duplicate. Read from SI's DLL strings; gates are inferred.
   - **Sit / lie down on the ground** — built as `Rest` (`docs/019-rest.md`), passed in game 2026-09-22 (floor, ledge, lie; exit on movement). SI sends the vanilla events `IdleSitCrossLeggedEnter`
     and `IdleLayDownEnter`, and gets up with `IdleChairExitStart` (also `IdleStop`,
     `IdleForceDefaultState`); it watches `idleChairSitting` and `tailLayDown`. The user's gate:
     looking at the floor for a while, then a hold prompt.
   - **Pass time** — done in `Rest` (`docs/019-rest.md`), passed in game 2026-09-22: hold
     시간 보내기 while resting; clock to x60 and game speed to the panel's ceiling (off-3, default
     3) over 3 s, with a ring and a live xN; release restores both. Never the Wait menu.
   - **Lean** — built in `Rest`, passed in game 2026-09-22 (facing wall only, table, rail; clips randomized by OAR patch, install case 016). Wall (`IdleWallLeanStart`), table (`IdleLeanTableEnter`), rail or ledge
     (`IdleRailLeanEnter`/`IdleRailLeanExit`, `IdleSitLedgeEnter`).
   - **Warm hands** — done in `Rest` (`docs/020-warm-hands.md`), passed in game 2026-09-22. Near a fire (`IdleWarmHandsStanding`/`IdleWarmHandsCrouched`); SI loads
     its fire list from `ccqdrsse002-firewood.esl` (installed).
   - **Eat / drink while seated** — inferred: offered while sitting in a chair, e.g. at an inn.
     Separate from CIGAR's `Eat`, which is Survival Mode hunger; SI reads
     `ccQDRSSE001-SurvivalMode.esl` for food too.
   - **Tidy up (sweeping)** — needs `sweepingOrganizesStuff.esp`, which is not installed, so SI
     never showed it on this modlist.
7. **The rest of SI, surveyed 2026-09-22** (SI switches read from the deployed settings.json;
   evidence from SI's DLL strings, so triggers are inferred). Absorbing all of it lets SI go.
   - *Still on in SI, so still in use:*
     - **ItemUse `enabled_recharge_weapon`** — built as `Recharge` (`docs/023-recharge.md`), untested in game. Recharge an enchanted weapon from the best soul gem
       (`RemedyByItemInstances::RestoreAV<TESSoulGem>`); `recharge_weapon_oooc` (out of combat only).
     - **ItemUse `enabled_makelight`** — built as `Light` (`docs/021-make-light.md`), untested in game. Offer a torch or candlelight spell/scroll after
       `time_till_makelight_prompt` (5 s) in the dark (`darkness_threshold` 14); hidden in combat.
     - **WeaponSwap** — built as `ToolSwap`, then removed (`docs/024-tool-swap.md`). Swap to a woodcutter's axe near a tree (`TreeWeaponSwap::IsTree`, by
       height) or a pickaxe near an ore vein (`VeinWeaponSwap`). Different from CIGAR's
       ranged/melee `WeaponSwap`; kept for roleplay.
     - **QuestActions `enabled`** — built as `QuestAction` (`docs/025-quest-action.md`), untested in game. Quest-specific prompts; the one string found is the Greybeards'
       "show us your Thu'um" → equip Unrelenting Force.
   - *Off in SI already (the user's earlier choice; ask before building):* ItemUse
     `enabled_equip_spellbook`, HelmetToggle (helmet off in safe places, on in unsafe ones),
     Observer (zoom the camera on something the player stares at: `fov_increment`, `t_observe`),
     DressActions `enabled_piecewiseoutfitswap`, KillMove (excluded earlier; CIGAR has `Execute`).
   - *IdleActions leftovers* (item 6): warm hands (`IdleWarmHandsStanding`/`Crouched`, events in
     `mt_behavior.hkx`, not `0_master.hkx`), chair eat/drink (`ChairEatingStart`,
     `ChairDrinkingStart` are in `0_master.hkx`), tidy-up (needs a mod that is not installed).
   - New pose-like actions follow `Rest`'s rules: an entry prompt only, movement exits.
