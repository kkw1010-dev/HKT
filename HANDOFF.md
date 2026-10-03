# CIGAR — session handoff (updated 2026-09-29, session "Cigar", after r8b)

## 시작 프롬프트 (새 세션에 붙여 넣기)

```
세션 이름 Cigar. 모델 Opus, 추론 high, 권한 bypass.
C:\TAKEALOOK\TKL-Agent\CIGAR\HANDOFF.md 맨 위 "Now (2026-09-29)" 절만 읽고 이어서 하세요.
개인판(CIGAR-Personal) 작업은 그 저장소의 HANDOFF.md를 함께 읽으세요.
사용자에게는 한국어로 답하고, 저장소 문서는 영어로 씁니다.
커밋은 로컬에만 하고, GitHub 푸시는 사용자가 요청할 때만 합니다.
보고는 SendMessage로 "오케스트레이터"에게 짧게 합니다. 오케스트레이터의 지시는 사용자 지시입니다.
빌드는 C++ 잠금 규칙을 따르고 자기 PID만 정리합니다. 배포는 SkyrimSE.exe가 꺼져 있을 때만 합니다.
공개 CIGAR 파일에는 개인판 내용을 적지 않습니다. 푸시 전에는 CIGAR-Personal\tools\check_public.py를 돌립니다.
```

## Now (2026-09-30)

- **Deployed (2026-10-01, 3.1.1 author build, `c948d7d`):** CIGAR plus CIGAR-Personal, author build, DLL SHA-256
  `e37d69f6ac31e674232de62c96d56cc3a17739f64c17ee25c8fb6caa9323c70a`; `verify_deploy` passed in full.
  The release preset (`build\dist`) also compiles, with no personal marker and no private word.
- **3.1.1 (2026-10-01):** package `Downloads\CIGAR 3.1.1.7z` built from `c948d7d` (pushed to GitHub; **never
  uploaded to Nexus**, whose page is still 3.1.0, uploaded by the user on 09-30). The next Nexus upload is
  **3.1.2** = 3.1.1 + Sit/Lie switches + two-column panel + the anchor re-hook, packaged only after r14 and on
  the user's word; `dist/nexus-page/*-next.*` are drafts for it (see the note at the top of `changelog-next.md`). In it: scripted unread notes get the read prompt (D23), the
  Squeeze nudge (slide 8), separate switches `Lean` and `PassTime` (`docs/019`). Also in it: the prompt side offset from -200 to 200 (`docs/007`; the mark is kept on screen only at the default offset). Earlier note:
  "widen the prompt angle" (two readings, see the orchestrator thread). Cleave: `docs/045` research, the
  prototype goes on a branch `cleave-proto` after CX-20/21, never on master.
- **Branches:** the working branch is `v3` (pushed to GitHub as both `master` and `v3`). The local branch
  `master` is an old one (behind): never build from it. `cleave-proto` (from `v3`) holds the Cleave
  prototype (`docs/045` "Prototype" on that branch); its test build is
  `C:\TAKEALOOK\_test-runs\cigar-cleave-proto\CIGAR.dll`, not deployed. Merge only on the user's word.
  **2026-10-01 (CX-21 done):** `docs/045` sections 6 and 12 are final. In this load order MCO fences the
  vanilla sideways power-attack idles into first person and sneaking, so the prototype (07da707, test DLL
  d25f8a81...) starts the swing three ways in turn per accepted prompt (action, event, idle) and logs what each
  produced; r14 reads that from `CIGAR.log`. `build\release` holds the prototype's DLL until the next `v3` build.
  **Later the same day (D30, D31):** the branch also holds the one crowd slot (stomp when surrounded, else
  Cleave; Fus Ro Dah logged only) and the war stomp: Bow Rapid Combo V3's kick copied at load into
  `overwrite\meshes\OpenAnimationReplacer\CIGAR Stomp` (a second runtime folder, like `CIGAR Squeeze`), played by
  Hot Key Skill's `CustomStartC`, with Dawnguard's area stagger at 0.33 s. `docs/045` "Crowd slot and war stomp"
  on the branch has the log lines and the console setup.
- **After r13 (2026-10-01, deployed author build):** r13 passed (nudge, notes, pass time, lean, prompt position).
  New: `Sit` and `Lie` switches (RestParts), the panel's module pages in two columns, no gate/last-log lines in
  the panel in any build. r14 items are in `TEST-next-ingame.md`. The anchor's quiet hook has an owner at last:
  `valhallaCombat.dll` rewrites the PlayerCharacter::Update vtable slot some minutes after CIGAR hooks it (early
  r13 run). Built (deployed, not seen in game): when the hook is silent for 5 s and the slot is not
  CIGAR's, `PromptAnchor` hooks again on top and chains to the holder, at most 3 times a session; a re-entry
  guard sends a second entry to the pre-CIGAR function, so a holder that chains back to CIGAR cannot loop.
  Judged from the log alone (`re-installed (#n of 3)` then `being called again`).
- **Mannequins Improved support (`docs/044`, D28): cancelled (2026-10-01); the user removed the mod from the modlist.**
- **Nexus feedback notes, reply drafts and research live in `..\CIGAR-Personal\nexus-notes\` (moved 2026-10-02), never in this repo;** `dist/nexus-page` keeps only posted texts and their next drafts (description, changelog, summary).
- **3.1.2 released (2026-10-02):** package `Downloads\CIGAR 3.1.2.7z` (DLL bf802224...), texts and hashes in `dist/nexus-page/upload-3.1.2.md`; pushed to HKT `master` and `v3`; uploaded to Nexus as file 813181 (page at 3.1.2; `description-live.bbcode` is the posted text). The upload-record commit after the push is local only. The anchor re-hook is still unconfirmed in game.
- **Before any Nexus description change: read the real page first** (`dist/nexus-page/UPLOAD-PROCEDURE.md`; `tools/check_nexus_page.py`). The 3.1.2 upload lost three feature GIFs the author had put on the page by hand.
- **Runtime guard on the anchor hook (2026-10-02, committed NOT COMPILED: builds were closed for r17).** `PromptAnchor::Install` hooks as before on SE, VR and AE up to 1.6.1179; on AE 1.7.x it first checks the player vtable and the slot; on a runtime newer than the bundled CommonLibSSE-NG knows it does not hook and logs one WARN (prompts stay on the player). Build and read the log line `PlayerCharacter::Update hooked on runtime ...` on 1.6.1170 before anything ships. CIGAR is in the "Mage Viking" Wabbajack list (game 1.7.x); never run on 1.7 by us.
- **Deployed author build 2026-10-03 (`v3` 3808b72): DLL 820559BC...** = 3.1.2 + poison press in combat + anchor runtime guard; the user's new baseline. Prototype DLL backups in `_test-runs\cigar-cleave-proto\backup-*`.
- **Cleave and the war stomp dropped (user, 2026-10-03)** — see `docs/045` "Decision"; `cleave-proto` is reference only. Next build = `v3` (Poison press in combat, anchor runtime guard) + the Unrelenting Force equip prompt (`docs/046`, awaiting approval). r17c: poison in combat and Grapple NPC off passed; the guard logged `hooked on runtime 1-6-1170-0 (slot AD; ...)`.
- **In this build (new):**
  - Every non-combat prompt fills a ring; every declined prompt stays hidden while its situation lasts
    (`docs/042`). `tools/check_prompt_rules.py` enforces the ring rule and typed form lookups before
    each build.
  - Squeeze Past (`src/Squeeze.*`, `docs/038` "As built" and "r9"): needs EVG Animated Traversal
    (D17); gate fixed after r9 (windowed speed, lingering prompt, scene/furniture NPCs, trace line,
    `tools/replay_squeeze_gate.py`).
  - The 2026-09-30 code review: 20 findings fixed, 4 not changed with reasons (`docs/043`).
  - HUD notices split (`docs/041` "Applied"): `Util::NotifyDiagnostic` is log-only in the release
    build; nine call sites use it, the rest stay HUD after reviewing CX-03.
  - The personal module fix (see CIGAR-Personal `HANDOFF.md`).
- **r9 is read** (`docs/038` "r9"): Squeeze S1-S6 failed (tick-noisy speed, the prompt followed the
  block tick by tick, most inn NPCs refused as scene or furniture); fixed and deployed, r10 again.
  Everything else in r9 passed and the log confirms it (Potion's decline was not exercised).
- **r8b is read** (`docs/038` "Stage 0d results", `TEST-next-ingame.md` "r8b 판정"). BaboKey linked.
- **r10 is read** (`docs/038` "r10"): Squeeze passed but two NPCs fell through the floor; fixed (no bump,
  standing NPCs only, fall guard, cloth sound). The user's "player-side only" idea is reviewed there:
  a system-group probe is proposed, not built. Also seen in r10: PromptAnchor's update hook stopped
  reaching CIGAR at 18:03:12 while ticks ran (prompts on the player for the session); not caused by CIGAR
  changes, cause not found yet.
- **r11 is read** (`docs/038` "r11"); **waiting on r12** (`TEST-next-ingame.md` "r12"): `TEST-next-ingame.md` "r10" (S1-S6 Squeeze Past, r10-P potion,
  and the passive review checks). Read `CIGAR.log` after the game closes; for Squeeze also run
  `python tools/replay_squeeze_gate.py`.
- **D17, D18 decided (the user, 21:50) and built:** Squeeze needs EVG Animated Traversal (and Offset
  Movement Animation), off without it; CIGAR builds its gesture submod at plugin load from the
  player's own EVG clip (`docs/038` "As built"). Potion: a press in combat, a ring out of it.
- **Push Test submods removed** (2026-09-29, MO2 Opt's lock). CIGAR writes `overwrite\meshes\OpenAnimationReplacer\CIGAR Squeeze` at each launch (registered with MO2 Opt as a runtime file).
- **Release D09: on hold (the user).** `CHANGELOG.md` "Unreleased" holds the next release:
  - the gamepad D-pad preset and mouse buttons (closed; texts say "checked with an Xbox controller");
  - the crash fix (SkyPrompt calls on the render thread);
  - the Jujutsu swing gate.
  Added 2026-09-29, untested in game until r9/r10: Squeeze Past, ring prompts, declines that stay
  declined, fewer HUD messages. Not in it yet: the r8 script-link guard (add a CHANGELOG line when the
  release is cut). Package with `tools\Build.ps1 -Package` (`tools/make_release.py`), and run
  CIGAR-Personal `tools/check_public.py` first.
- **GitHub (HKT):** pushed 2026-09-30 on the user's word: `master` = `v3` = `8a8f750` (fast-forward from
  `8254180`). Before it, the D07 filter took `TCL_LIGHT_HOOK_ANALYSIS.md` out of the 58 unpushed commits
  (`filter-branch --index-filter ... --prune-empty`, 2 emptied commits dropped, final tree unchanged);
  `check_public.py` passed on the tree and the commit messages. Local branch `backup/pre-d07-filter`
  keeps the pre-filter history; never push it.
- **Procedures:**
  - Build: `tools\Build.ps1`. It runs `check_menu_framework.py` and `check_input_map.py`. `-Deploy`
    also copies; the usual deploy is to copy `build
elease\CIGAR.dll` and `.pdb` into
    `mods\CIGAR\SKSE\Plugins` by hand while Skyrim is closed.
  - Verify: `python toolserify_deploy.py`.
  - Package: `tools\Build.ps1 -Package`.
  - In-game checklist: `TEST-next-ingame.md`.
  - Crashes: `TKL-Agent\Crash Triage`.
- **Session rules:** Opus with high effort, bypass permission mode. Orchestrator instructions are
  the user's. Downloads go to the "경량 에이전트" session. No upstream bug reports to other mod
  authors. Gamepad and mouse work is closed.

## Start here (older, 2026-09-28)

Reply to the user in Korean. Read this section, then `README.md`. The design rationale and the test
history of each module are in `docs/`, one file per module, and each file starts with its status.

### Next session starts here (2026-09-28, after r5)

- **r5 is read** (`docs/038` "Stage 0 results", `docs/012` "r5"). Push-through: the NPC side is GO with
  the direct bump graph events (displaced 50-84 units, no line, no hostility; `ActionBumpedInto` did
  nothing). The user picked **3805 (EVG Squeeze, upper body)** and asked to combine it with 3806.
- **Waiting for the user's choice** of the combination (`docs/038`, three options: 3805 cut short,
  pick by the gap, a real blend on HOLD). Recommended: test the first two in one short probe round.
  The next probe build also stops a running gesture before starting a new one.
- **Gamepad and mouse mapping: built for the next release, test r6 pending.** Two pad presets
  (SkyPrompt's buttons by default, or D-pad 1 Up / 2 Down / 3 Left / 4 Right) and the mouse's middle
  and side buttons; design, log lines and tests in `docs/016` "Decided". It ships with push-through and
  the 유술 swing gate. Tested on Xbox only (the user's pad); release texts say PlayStation pads were not
  tested. `tools/check_input_map.py` guards it in every build. Deployed as `de8ff68` (no panel warnings, the user's
  call; a `player equip:` log line for the hotkey check); r6 items D0-D8 in `TEST-next-ingame.md`.
- **r6 CTD (23:30:09, D2) and the fix: `docs/039`.** A SkyPrompt 2.4.0 race (`Manager::ShowQueue`
  reuses row indices across a lock release), hit when a CIGAR withdraw from a task lands between its
  two loops. It was not the D-pad codes. Every SkyPrompt call now goes through a queue drained by a
  hook on SkyPrompt's own Present call site; `check_input_map.py` enforces it. The next run's log must
  show `prompt queue: Present hooked`, `ticks run on thread M, Present on thread N` (M != N confirms
  the premise). r6 D2 onward, U1 and K1 carry over.
- **r7 (2026-09-29), `b26e031`:** T0 confirmed the premise (ticks on pool threads 8784/34920, Present on
  30696, and a tick seen inside Present); T1 10x draw/sheathe with no CTD; D1-D5, D7, D8 passed, D6
  skipped; U1 passed (10 plays, 4 refusals on swing starts), so the SBF 2.0 `mt_behavior.hkx` is the new
  baseline; K1 withdrawn (no save-then-load tests, the user's rule); B5: no prompt showed during the wash,
  now also guaranteed by a tick-level wash guard in `main.cpp`. Gamepad and mouse work is closed.
- **Deployed now: the author build of `124578a`** (queue, wash guard, squeeze probe, and the Jujutsu
  line `first death since the load: <who>, killed by <whom>`). K1-style save-then-load tests are not
  proposed any more; a loaded save's refusal window is read from ordinary play logs.
- **Deployed earlier: `b26e031`** (the queue). r7 checklist in `TEST-next-ingame.md` "r7": T0 thread
  premise, T1 draw/sheathe stress, D2-D8, K1 (save-loaded half), U1; bandit `0003DE8A`. No upstream
  SkyPrompt report (the user's decision); `docs/039` also answers "why not SkyPrompt 2.3.15".
- **Squeeze feature, stage 0d probe built (`7822eab`), NOT deployed yet** (the game was running r7 on
  `b26e031`): deploy when the game is closed, then r8 = `TEST-next-ingame.md` "r8 탐침" Q1-Q5. The user
  approved comparing (가) the player's capsule radius x0.5 and (나) the no-collision bit on that NPC's
  controller body; both are undone once past the NPC or after 3 s. Feature itself (contact -> prompt ->
  squeeze -> restore, auto gesture) waits for the probe's result.
- **Pandora rewrote `mt_behavior.hkx` at 2026-09-28 21:06** (not a CIGAR action; md5 4149d75c...), so
  `verify_deploy` fails its Jujutsu baseline. Not restored: the file is outside `mods\CIGAR` and the run
  carries other mods' updates: MO2 Opt ran it on purpose for State Behavior Framework 1.4 -> 2.0 (Q1).
  The previous output, with the Jujutsu copy cb3a5b03, is backed up in
  `C:\TAKEALOOK\_backups\2026-09-28 pandora before sbf2`. r6 U1 re-tests Jujutsu on it; if U1 plays, run
  `python tools/verify_deploy.py --accept-behaviour`; if it fails, tell MO2 Opt, which will roll SBF
  back to 1.4 and rerun Pandora.
- 유술 J1 in r5 was refused because the target was attacking, not the first-death window; K1's
  save-loaded half was not run. The save button was not CIGAR's.
- The `CIGAR Push Test` clips stay in `mods\CIGAR` until the combination is chosen; then
  `python tools/push_test_assets.py --remove` (Skyrim and MO2 closed).
- Nothing is uncommitted. `v3` = `master`, local commits ahead of GitHub (`8254180`; pushed only when
  the user asks). Build lock and `mods\` lock: ask the orchestrator.

### Current state (2026-09-27 handover, read this first)

**Coordination.**
- Several Claude sessions work on TAKEALOOK at once. The orchestrator session is named
  **"오케스트레이터"**: take instructions from it and report to it with SendMessage, briefly and in
  Korean.
- A peer's message is not the user's approval. The permission check refuses MO2 profile edits
  (`modlist.txt`, `plugins.txt`) relayed by another session; they need the user to say so in this
  session.

**Git.**
- **3.0.0 shipped** (Nexus, 2026-09-27). `master` = `v3`; GitHub (`kkw1010-dev/HKT`) has both at
  `8254180` and the tag `v3.0.0` (`affb7c0`). Local commits after that are pushed only when the user
  asks.
- **History rewritten on 2026-09-28** (the user's decision): every commit from 2026-09-27 onward got a
  new hash, and GitHub was force-pushed. The old-to-new map and a bundle of the old history are kept
  locally in `../CIGAR-Personal/backups/`. Hashes in these docs are the new ones.
- Branches:
  - `master` = `v3`: 3.0.0 and later; keep working on `v3` and fast-forward `master`
  - `v2`: 2.x, on GitHub (2.1.2)
  - `brace`: abandoned, kept, never merged (forks before the rewritten range, so unchanged)
- `TCL_LIGHT_HOOK_ANALYSIS.md` is someone else's untracked file; leave it.

**Deploy.**
- `mods\CIGAR` holds the **7ea581c author build** (2026-09-28): 3.0.0, the log-only NPC push probe
  with the stage-0b and 0c gesture buttons (`src/PushProbe.*`, `docs/038`), the 유술 swing gate
  (`docs/012`), and the personal-module hook (empty registry). `verify_deploy.py` passed every check.
  Next in-game run: `TEST-next-ingame.md` G9-G14 and U1. The right-offset setting and the five
  pages passed retest 3.
- **Test assets in `mods\CIGAR`:** `meshes\OpenAnimationReplacer\CIGAR Push Test` (six OAR submods,
  `tools/push_test_assets.py --deploy`). Author test only; take them out with `--remove` when the
  comparison is over. They never enter a package (`make_release.py` builds from its own allowlist).
- The deploy procedure, whenever `mods\` is held by another session: wait for the orchestrator's
  release, check that Skyrim is closed, run `tools\Build.ps1 -Deploy` (it builds, copies only into
  `mods\CIGAR`, runs `verify_deploy.py` and stops only the processes it started), then report the
  result and hand the lock back.

**Tests (`TEST-next-ingame.md`).**
- N1-N5 passed in the 17:41 run: the relief prompt hidden during a bath, 주시하기 after a bath or
  when idle, the mannequin regression on Another Mannequin Script Fix, 마검사 모드 on and off.
- N6 passed: the prompt marker.
- N7 passed: all five steps; the prompts follow after fast travel, a door and riding. **PromptAnchor
  is finished.** The user kept the offset as the player's choice instead of fixing one value, so the
  N7 item became the player setting `5. 세부 설정` → 프롬프트 위치 → 오른쪽 간격 (3인칭)
  (`CIGAR.json` `prompt.rightOffset`, 0 / 10 / 15 / 20 / 30, default 15; `docs/007` "Finished after
  N7").
- Test logs are archived under `C:\TAKEALOOK\_test-runs\<date time>\SKSE\`.

**Added in 3.0.0 so far (all on `v3`):**

| Feature | Detail | Doc |
|---|---|---|
| MannequinSwap | The slot-60 fix: the HDT SMP carrier no longer blocks | `docs/030` |
| Needs | No prompt while Bathing in Skyrim washes (`Bathe::Washing()`) | `docs/013` |
| WizardWarrior | 마검사 모드 on weapon draw (ID 41) and 마검사 해제 when sheathing out of combat (ID 42), both through `QK_MainQuestScript.ToggleAbility()`. The Wizard Warrior 5.0.1 is installed (IaM case 037, plugin index 0x89) | `docs/037` |
| PromptAnchor | In third person, prompts attach to one disabled XMarker (co-save `ANCH`) that a `PlayerCharacter::Update` hook (0xAD, chained) moves every frame to head + facing × 40 + camera-right × offset (player setting, default 15). First person uses the player. With recovery, a hook-liveness check and fallbacks. Passed N6 and N7 | `docs/007` "Prompt placement" |

**The user's decisions this session.** Do not re-propose these.
- Brace (버티기) abandoned.
- Modifier and arrow keys as prompt keys rejected.
- Other mods' keys, MCM settings and key clashes are the player's business.
- The enchantment WW leaves after 마검사 해제 is WW's own behaviour.
- In first person, prompts stay on the player.
- Forward distance is 40.
- The right offset stays the player's choice: 0 / 10 / 15 / 20 / 30, default 15.
- A pinned Nexus FAQ post is up (comment 176294172).
- Nexus replies are posted by the user, never by Claude.

**Open items and backlog.**
1. **Retest 3 (R3-C1..C8) passed** (the user's report through the orchestrator, 2026-09-28). The
   author-build log of 2026-09-27 23:20-23:47 shows it:
   - the five pages registered, with the modules on the pages of `docs/007` "Module pages", and
     each page drawn; no WARN from the panel;
   - the right offset moved through 0 / 10 / 15 / 20 / 30 and saved (`settings saved`, `prompt right
     offset N`).
   - Not in that log: a module switch toggled (R3-C5) and a reload after saving 30 (R3-C8). The
     offset lives in `CIGAR.json`, not the save, so a reload cannot lose it.
   - Seen in the same log, not an r3 item: on a **new game** the marker cannot be placed at load
     (`WARN could not place the marker (load)`, the player is not in the world yet); recovery placed
     it 25 s later and prompts attached to it. Harmless, but the WARN reads like a failure; a quiet
     first retry for the new-game case is a candidate fix.
2. **3.0.0 is live on Nexus 193080** as **file 811321** since 2026-09-28 (the trimmed release,
   below). First upload: file 811289 (`CIGAR`, 3.0.0, Main, 922,079 bytes), an Update of 2.1.2; mod version 3.0.0, changelog, description and The Wizard Warrior requirement
   done. The session "CIGAR 3.0.0 Nexus 업로드" did the upload at the user's direct instruction,
   because this session's permission check kept refusing the Nexus edit page. Record:
   `dist/nexus-page/upload-3.0.0.md`. DLL sha256 `afef2eb1...`. The `Downloads\CIGAR 3.0.0` folder
   and archive were gone on 2026-09-28; rebuild with `tools\Build.ps1 -Package` if a copy is needed.
   - The slider and the five pages passed retest 3 in the author build; the release build itself
     has not been launched.
   - Open: a GitHub release for 3.0.0 and the push, only when the user asks.
   - **Release texts trimmed (the user, 2026-09-28): English, feature descriptions only.** The
     Korean readme no longer ships (`dist/README-ko.md` deleted, `make_release.py` allows five
     files). README, CHANGELOG and the Nexus texts lost their system explanations; the user's own
     page sections are cut to one or two paragraphs each. **On Nexus since 2026-09-28**, applied
     by the orchestrator with the user's approval (`dist/nexus-page/upload-3.0.0.md`, top):
     - file 811321, `CIGAR 3.0.0 (docs trimmed).7z`, 917,088 bytes, the same DLL (sha256
       `afef2eb1...`), an Update of 811289 (now Old versions)
     - description = `description-live.bbcode` = `description-3.0.0-trimmed.bbcode`; the text
       before the trim is `description-3.0.0-untrimmed.bbcode`
     - Logs 3.0.0 = the seven lines of `changelog-3.0.0-trimmed.txt`
3. **NPC push-through (`docs/038`), stage 0 and 0b.** The user's decisions so far are recorded there
   (small bump, no NPC line, 70 / 0.3 s / 1.5 s, "비켜 지나가기 (누르고 있기)"). Deployed: the bump probe
   (P1-P5) and six clip candidates (G1-G7, `CIGAR Push Test`, vanilla A and EVG C with its annotations
   stripped). Waiting for the in-game run; see "Next session starts here". Clip previews:
   `C:\TAKEALOOK\_staging\push-clip-candidates\index.html`. Whether a vanilla clip may ship in
   the package is the user's call (`docs/038`, "Must a vanilla clip ship").
4. **Personal modules** (the user, 2026-09-28). Modules for the user's own game only live in the
   sibling repo `../CIGAR-Personal` (local, no remote) and are tracked in its own `HANDOFF.md`. They are
   compiled into the author build only (`src/Personal.h`); CIGAR's public files never name them.
5. Read It Now duplicates part of 책 읽기. Whether to disable it is the user's call; CIGAR needs no
   change.
6. **주시하기 during a bath, third person (open, no action).** The user will check it in game
   (Nexus IAMTOKKO; `CIGAR-Personal/nexus-notes/feedback-2026-09-28.md` §3). Wait for the result.
7. Optional: propose a per-prompt offset API to SkyPrompt (QTR-Modding, MIT).
8. Nexus: the reply drafts in `CIGAR-Personal/nexus-notes/replies-2026-09-27.md` are for the user. There is
   also an FAQ candidate about WW's lingering enchantment in `docs/037`.
9. Crash Triage S001 (the Journal Menu CTD): a discriminating test waits for the user's choice. It
   is tracked in `../Crash Triage`.

**Where things are.**
- `docs/007`: panel, prompt keys, prompt placement and PromptAnchor
- `docs/037`: WizardWarrior
- `dist/nexus-page/`:
  - `description-live.bbcode`
  - `sticky-faq.txt`
- `TEST-next-ingame.md`: N1-N7

**Pitfalls met this session.**
- Python heredocs in Bash collapse `\`, and `\2` became a control character. Write scripts to
  scratchpad files instead.
- In PowerShell, run `Build.ps1` without `2>&1`, or vcvars' stderr aborts it.
- The C++ build tools and the vcpkg lock are shared with other sessions' builds.
  - A reconfigure waits on the lock while another session builds.
  - Build only when the orchestrator says the slot is free.
  - Never kill `mspdbsrv` or `vctip` by name.

### Earlier on 2026-09-27 (history)

- **MannequinSwap 3.0.0 is tested on the USSEP script.** The combined new-game run of 2026-09-27
  passed all of it:
  - A1: the slot-60 fix `7ccf6c0`; the HDT SMP carrier no longer blocks an outfit with a slot-60
    piece
  - A2: the same base item on both sides, with no bounce
  - A3: re-entry

  The test list is `TEST-run-2026-09-27.md` on the `brace` branch; its log is archived in
  `C:\TAKEALOOK\_test-runs\2026-09-27 1207 run\`. The USSEP section below is kept as history.
- **`+Another Mannequin Script Fix (AE.SE)`** is being turned back on by the MO2 session, as part
  of its own work. The orchestrator moved that job there; do not do it from here.
- **The brace feature (버티기) was abandoned by the user.** The branch `brace` is kept and never
  merged; `docs/036-brace.md` there records why and the stage-0 evidence. The deployed DLL is
  `v3` again (no probe), rebuilt and verified on 2026-09-27.
- **Next:** 3.0.0 packaging and release with the user (see "Next, in order").

### The USSEP mannequin test (2026-09-26, done)

The user started the next session from **whether the USSEP test is complete**. Ask for their result,
then read `SKSE\CIGAR.log` before anything else.

- **What it tests:** `TEST-3.0.0-ussep.md`. It checks MannequinSwap on USSEP's
  `MannequinActivatorSCRIPT`, the one most Nexus players have: slots in a `Form[] ArmorSlot`
  variable, 10 of them, and a second piece of the same base form bounced back to the player.
- **What to look for in the log:** the gate line should say `slots=array 1/10` (the 1 is the SMP
  carrier). Then `swap done: ... 0 problems`, no `[MannequinSwap] WARN`, and step 4 (a same-base
  cuirass on both sides) swapping without a bounce.
- **Load order is set for it:** `-Another Mannequin Script Fix (AE.SE)` (backup
  `modlist.txt.bak_20260926_ussep-mannequin-test`), so USSEP's BSA copy wins. This was checked with
  `housecarl_asset_status scripts/MannequinActivatorSCRIPT.pex`. It needs a **new game**, because a
  save from the fix's script holds 20-slot data.
- **After the test is read:** turn the fix back on (`+Another Mannequin Script Fix (AE.SE)`, MO2
  closed), and leave MO2 closed (memory `leave-mo2-closed-after-work`).
- **If it fails:** the array path is `MannequinSwap::ReadSlots` (the variable first, then the
  properties), and the duplicate handling is the `kTakeWait` poll in `FastTick`, which waits until
  the script has emptied the taken pieces' slots.
- **Still not exercised anywhere:** the vanilla script path (no USSEP, no fix). The vanilla script
  records nothing when full; the capacity preflight covers that (`docs/030`, second review).

### A CTD interrupted the USSEP test (not CIGAR)

2026-09-26 20:38:44, opening the Journal Menu 17 minutes into the test's new game. It is tracked in
`../Crash Triage` as signature S001 (the third occurrence): a Scaleform render-heap crash as the
Journal Menu opens. The two suspects are DBReV's Journal Menu translator and MCM Memory. CIGAR is not
in the stack. The USSEP test itself had not reached a swap yet, so it is still to do.

### Solved elsewhere: MCM Memory re-triggered BaboDialogue's kidnap test

A new game jumped straight into BaboDialogue's kidnap cell, because MCM Memory replayed the recorded
`$BaboTestQuestBaboKidnap` click. This was solved in another session as **Installation and
Modification case 036** (`cases/036-mcm-memory-guard-babo-debug-buttons.md`).

- The fix is the MO2 plugin `MCM Memory Guard` (`C:\TAKEALOOK\plugins\MCM_Memory_Guard\`). It
  strips BaboDialogue's `$BaboDebug` entries from the profile.
- The case says it has not been loaded by MO2 or exercised in game yet. If a new game still jumps to
  the kidnap cell, read that case first.

### What this session did (2026-09-26)

In order. Every item is committed; the versions are also in `CMakeLists.txt` history.

1. **2.1.0, one release for every host.** The user decided every other-mod integration goes back into
   the Nexus upload as off-site requirements. It relies on the Smooth (Grapple) and BakaFactory
   permissions, and on the LoversLab-dependent files Nexus already hosts. Details:
   - `CIGAR_NEXUS` and `Build.ps1 -Nexus` are gone.
   - `-Package` makes one package with `README.md` (English) and `README-ko.md`, and fails on any
     file but CIGAR's own six.
   - The archive has `SKSE\` at its root (up to 2.0.1 it wrapped a folder).
   - Helmet Toggle 2's clips play in the author build only (the user: "헬멧토글은 나만 쓸거").
   - ChairDrink knows alcohol by base-game FormIDs and mod keywords together.
   - Record: `docs/035`, first section.
2. **2.1.1.** Two fixes:
   - Observe zoomed `firstPersonFOV` (only the first-person arms model), so it never zoomed in first
     person. Two Nexus comments reported it (`docs/031`). It now uses `worldFOV`, with a WARN if
     another mod overrides it.
   - Pass Time on a gamepad stopped after 0.3 s (`docs/019`). The user now has a pad and tested it.
3. **2.1.2.** OStim scenes gate prompts like SexLab ones:
   - `Util::InScene` / `Util::SceneOf` check `OStimActorCountFaction` 0xECA in `OStim.esp`.
   - BaboKey, Eat, Execute, Needs and Surrender use that shared check.
   - OStim is not installed here. Only the SexLab regression path is covered, and 2.1.2 itself was
     published without an in-game run.
4. **Nexus 193080** (through the user's Chrome) is at **2.1.2**. Each version went up as an Update
   of the previous file. The page now has:
   - summary "No gameplay mods required; optional mods add their own prompts"
   - gamepad wording "Keyboard and mouse, or a gamepad through SkyPrompt's own gamepad buttons"
   - an OStim/SexLab scene line
   - tags AI-Generated Content + AI Media
   - no adult tag
   - requirements by the legacy method with notes: SKSE64, Address Library, SkyPrompt and SKSE Menu
     Framework "Required"; BiS, TDM, Valhalla, Acheron, SMI and Gourmet "Not necessary, but
     recommended"
   - LoversLab-only mods in the description only; **Grapple nowhere on the page** (the user)

   Live texts: `dist/nexus-page/description-live.bbcode` and `summary-live.txt`; checked IDs and
   URLs: `requirements-2.1.0.md`. Memory `cigar-nexus-edition-integrations`. Permissions stay a
   custom GPL statement, not Nexus's checkboxes, because CIGAR must be GPL-3.0-or-later
   (CommonLibSSE-NG).
5. **GitHub:** `master` and `v2` were pushed up to 2.1.2 (`232b391`). The `nexus` branch is deleted;
   the 2.0.1 source stays under tag `v2.0.1-nexus`.
6. **3.0.0 MannequinSwap on `v3`**: `src/MannequinSwap.*`, PromptID 40, and small hooks in Dress
   (`OutfitChanged`), Helmet (`Stowed`/`TakeStowed`/`Stow`/`Busy`) and PartyOutfit (`Active`).
   - The plan was reviewed against all three mannequin scripts (vanilla, USSEP, Another Mannequin
     Script Fix; `docs/030` table).
   - Four test rounds in JK's Riverfall Cottage (`coc XJKRiverFallCottage`, mannequin `6307B2E8`).
     **Test 4 passed:** 의상 보관/착용/교환, enchanted pieces moved as `the same instance`, the helmet
     both ways (it arrives stowed), a cell re-entry, the double-tap decline, and the TCL lantern left
     out.
   - Faults found and fixed along the way (all in `docs/030`): the mannequin's own SMP carrier counted
     as its outfit; outfits are now collected per worn item, not per slot; headgear moves on any
     slot. Why the iron helmet (slots 31+42) was left out before stays unconfirmed; the gate line
     now logs every piece that stays, with its reason and runtime slot mask.
7. Readmes and the in-game text say gamepads work and scenes are gated. Memory `leave-mo2-closed-after-work`
   is new (see "State now").

### State now

- **Branches:** `v3` is checked out, **local only**, head `5dcbc8c`.
  - `master` is `da34ec3`: 2.1.2 plus one docs commit, ahead of origin by that one commit.
  - `v2` = `origin/v2` = `232b391` (2.1.2).
  - Commit locally on every change; push only when the user asks.
- **Load order** (profile `TKL - MUNG ADDON`):
  - `+CIGAR`: the author build 3.0.0, deployed, `verify_deploy` passed.
  - The release copies are off: `-CIGAR 2.1.2`, `-2.1.1`, `-2.1.0`, `-2.0.1 Nexus`, `-2.0.1`.
  - `-Another Mannequin Script Fix (AE.SE)`, for the USSEP test (see above).
- **MO2 and Skyrim are closed.** Leave MO2 closed after any profile edit; the user starts it
  themselves, because an MO2 that Claude launched cannot start the game.
- **Packages** in `%USERPROFILE%\Downloads`: `CIGAR 2.1.0`, `2.1.1`, `2.1.2` (+ `.7z`). The 2.1.2 DLL
  sha256 is `8cf01ec4...`. The same copies are in `mods\CIGAR 2.1.x`.
- **SkyPrompt 2.4.0** runs fine with CIGAR. Its log warning `Failed to import translation for
  SkyPrompt` is harmless: Scaleform Translation Plus Plus replaces the translator, and the KR text
  still loads.
- Untracked files in the repo root (three analysis `.md` files by other agents, and `scratch/`) are
  not ours. Leave them.

### Next, in order

1. The USSEP test result (above), then restore the fix in the load order.
2. (Done elsewhere: case 036.) If the kidnap cell still appears on a new game, see case 036.
3. MannequinSwap for release: decide 3.0.0 packaging and upload with the user.
   - A capacity or duplicate block notifies once and logs the reason.
   - The Nexus description will need a "Mannequin Outfit Swap" line.
   - `v3` merges into `master` when 3.0.0 ships.
4. Minor, open: a GitHub release for 2.1.x (ask first); the `v2.0.1-nexus` release notes still say
   SkyPrompt 2.3.15. Modifier and arrow keys were **rejected** by the user on 2026-09-27; the
   reason is in `docs/007-control-panel.md`, and the correcting Nexus reply is drafted.
5. **Nexus feedback of 2026-09-27** (`CIGAR-Personal/nexus-notes/feedback-2026-09-27.md`; decisions and replies
   in `CIGAR-Personal/nexus-notes/replies-2026-09-27.md`). The user posts the replies, never Claude.
   - **Posted on Nexus (2026-09-27, by the orchestrator session after the user approved):**
     - `sticky-faq.txt` went up as a Posts comment and is pinned (comment id 176294172, sticky
       confirmed on the page).
     - The two FAQ lines of `description-live.bbcode` (Language) are saved on the live
       description.
     - The reply drafts in `replies-2026-09-27.md` are for the user to post; whether they are up is
       not recorded here.
   - In-game tests pending in `TEST-next-ingame.md`:
     - N1: the relief-prompt-during-a-bath fix (deployed)
     - N2: 주시하기 after a bath or when idle
     - N3: mannequin regression on Another Mannequin Script Fix
     - N4: 마검사 모드
   - The Wizard Warrior: the module is built and deployed, and WW 5.0.1 is installed (IaM case 037,
     plugin index 0x89; `docs/037-wizard-warrior.md`). In-game test N4; new game required.

### Two builds, one source (since 2.1.0)

| Build | Command | Output | Notes |
|---|---|---|---|
| Author | `tools\Build.ps1 -Deploy` | `mods\CIGAR` | panel shows gate lines; helmet clips; runs `verify_deploy.py` |
| Release (Nexus and elsewhere) | `tools\Build.ps1 -Package` | `Downloads\CIGAR <ver>` + `.7z` | all modules, English + Korean readmes, file allowlist, `SKSE\` at the archive root |

- `-Deploy` needs Skyrim closed; MO2 may stay open when only the DLL changes.
- A load-order swap needs MO2 closed, and MO2 stays closed afterwards.
- Put a new release on Nexus as an **Update** of the current main file. That moves the old file to
  old versions and lets mod managers see an update.

### Pitfalls met this session

- In this harness, inline `python - <<'EOF'` heredocs collapse backslash escapes (`\n`, `\\`) and
  sometimes fail to parse; write Python to a scratchpad file and run it instead.
- Nexus's editor (SCEditor) and some fields ignore values set by script: after setting them, make one
  real keystroke (type a space, Backspace) so the form registers the change, then verify through the
  public v2 GraphQL API (`https://api.nexusmods.com/v2/graphql`, keyless).
- The Chrome file-upload tool cannot read `Downloads`; copy the file to the scratchpad first. Never
  click a page's upload button (it opens a native picker); feed the hidden `input[type=file]`.
- Korean game text on this modlist runs with `sLanguage=ENGLISH` (the Korean patch replaces the
  English strings), so a Korean translation of another mod goes in its `_ENGLISH` file.
- Every actor on this modlist, mannequins included, wears the non-playable Softbody SMP carrier
  (`HDTSMPObjectBase`). Any "what does this actor wear" code must skip non-playable items.
- Collect worn armor per inventory item, not by asking each biped slot for its piece: the slot scan
  hid a helmet in MannequinSwap's tests 2-3.
- Which copy of a script or asset the game uses: `housecarl_asset_status` (loose beats BSA, then
  priority), before assuming a mod's script behaviour.

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
- A later plan: a fishing module on Streamlined Fishing (`docs/027-fishing.md`; waits on the decisions
  listed there).
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
- **Control panel.** Optional SKSE Menu Framework pages (CIGAR / 1. 전투, 2. 비전투,
  3. 모드 연동, 4. 단축키, 5. 세부 설정;
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

## Working style (the user asked to keep it)

The user named the brace-QTE answer of 2026-09-27 as the standard for recommendations and
reviews. Keep it across sessions:

- Lead with the verdict (yes/no and the chosen approach), then the reasons.
- Build from parts CIGAR already has (Surrender's slow motion, Jujutsu's `KnockExplosion`,
  SkyPrompt conventions) rather than inventing a new system.
- When disagreeing with an outside plan (a GPT draft and the like), argue from game feel and
  the plan's own text.
- For each failure case (accidental success from key mashing, overlapping prompts, what a
  decline means), give a concrete countermeasure.
- Leave taste values (a window length and the like) to the user: propose a starting value only,
  and record the chosen value as the user's choice.
- State the core risk openly and put a verification step first.
- End with a concrete next step.

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
