# CIGAR 2.0

**C**ontextual **I**nteraction, **G**ameplay **A**cceleration & **R**hythm.

CIGAR is an ESP-less SKSE plugin (`CIGAR.dll`, one DLL for SE, AE and VR) that turns the things a
Skyrim player does over and over into **context prompts** drawn by
[SkyPrompt](https://www.nexusmods.com/skyrimspecialedition/mods/149963): a prompt appears only while
the action makes sense, and one key does the whole action. It also replaces other mods' hotkeys with
prompts.

CIGAR resolves repetitive interactions from gameplay context and exposes them as prompts. The system
absorbs the interaction complexity so the player can simply play the game.

The player-facing readme, in Korean, is [`dist/README-release.md`](dist/README-release.md); it ships
with the release.

## Principles

- **No plugin file.** Nothing is added to the load order, and no other mod becomes a master.
- **One button, whole action.** A prompt performs the action itself; nothing vanilla already does in
  one press is duplicated. Non-combat actions are hold prompts so a stray tap changes nothing.
- **Optional integrations are detected at runtime.** An interaction whose target mod is missing stays
  idle and says so in the log. No per-mod patches, no MCM toggles to enable or detect one.
- **Self-reporting.** Every module logs its gate inputs and actions to `CIGAR.log`, and silent
  blockers raise one notification.

## Modules (2.0)

All 24 modules passed the CIGAR 2.0 final in-game test on 2026-09-25 (44 of 44 checklist items).

| Module | Prompt | Needs (optional) |
|---|---|---|
| `Bathe` | 목욕하기, 샤워하기 | Bathing in Skyrim - Renewed |
| `Dress` | 탈의하기, 착용하기 (bed, wardrobe, water) | — |
| `Eat` | 먹기 | Survival Mode + Survival Mode Improved |
| `Needs` | 소변 보기, 대변 보기 | Private Needs - Orgasm |
| `Deflate` | 배출 | Fill Her Up Baka Edition |
| `Rest` | 앉기, 눕기, 기대기, 손 녹이기, 시간 보내기 | — |
| `ChairDrink` | 마시기 (seated at an inn or home) | — |
| `Observe` | 주시하기 (zoom on a person or a distant view) | — |
| `ItemEquip` | 장착하기 (newly acquired gear; armor only if better and not over an enchanted piece) | — |
| `BookRead` | 읽기 (unknown spell tomes, quest notes) | — |
| `Helmet` | 투구 벗기, 투구 쓰기 (helmet off by default, on in combat) | Open Animation Replacer clips |
| `Recharge` | 충전하기 (enchanted weapon at 25% or less) | — |
| `Poison` | 독 바르기 | — |
| `Potion` | 마시기 (health, stamina, magicka, cure poison, cure disease, water breathing) | — |
| `QuestTrack` | 추적하기 (untracked quest, new objective) | — |
| `QuestAction` | 장착하기 (the Greybeards' shout) | — |
| `PartyOutfit` | 파티 의상 입기, 원래 장비로 (Diplomatic Immunity) | — |
| `BaboKey` | 행동 선택 (kidnap events) | BaboDialogue |
| `LockOn` | 록온 | True Directional Movement |
| `Grapple` | 그래플 | Grapple |
| `WeaponSwap` | 원거리 무기, 근접 무기 | — |
| `Execute` | 처형 | Valhalla Combat |
| `Jujutsu` | 유술 (four hand-to-hand takedowns, and a lethal neck break) | — |
| `Surrender` | 항복 | Acheron (+ Yamete Kudasai) |

A prompt dismissed with SkyPrompt's double tap stays hidden until the situation changes (pass time,
helmet, chair drink, observe). Prompts hide while a menu is open. Keyboard and mouse are the target;
gamepads are not supported.

## Known issues

- **Jujutsu is refused until the session's first death.** After a load, every 유술 press is refused
  until any actor other than the player has died; from then on it works normally. Traced over tests
  24-29 in `docs/012-jujutsu.md`: not the move, the victim, its animation graph, the kill camera or
  NPC grapples. The one lead left was writing an AI-process field (`killMoveTimer`) whose effect is
  only known from reverse engineering, and it was deliberately **not touched because of that unknown
  risk**. Refusals in that window are logged as the known issue and raise no notification.
- **Execute sometimes misses its first press.** Pressing again executes.

History: the project started on 2026-09-16 as a Papyrus-based prototype, was rewritten as this SKSE
DLL on 2026-09-17, and reached CIGAR 2.0 on 2026-09-25. Each module's design and test record is in
`docs/`, one file per module.

## Layout

```text
src/main.cpp        SKSE entry, lifecycle messages, co-save, ticker (one game-thread task per 100 ms: FastTick every time, Tick once a second)
src/Module.h        module interface (OnGameLoaded / Tick / OnAccepted) and gated logging
src/Prompt.*        SkyPrompt client and one sink per prompt (SkyPrompt 2.3.15 removes by sink)
src/Util.*          strip rules, worn description, Papyrus script-property reader, synthetic key presses
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
src/Jujutsu.*       vanilla H2H kill move on a guarding humanoid; KillActorHandler hook keeps the victim alive (the neck break kills)
src/Potion.*        drinks a potion for low health, stamina, magicka, a poison, a disease or being submerged (potions found by their effects, not by form ID)
src/QuestTrack.*     offers an untracked quest when one of its objectives becomes displayed, then calls Quest.SetActive
src/ItemEquip.*      offers newly acquired gear for immediate equip (armor only if better, never over an enchanted piece)
src/BookRead.*       offers an unknown spell tome or a quest note for reading
src/Rest.*           sit, lie, lean and warm-hands idles, and pass time (clock and game speed while held)
src/ChairDrink.*     drinks alcohol with the vanilla chair-drinking idle at an inn or home
src/Observe.*        eased field-of-view zoom on a person or distant scenery
src/Helmet.*         helmet off by default with a take-off clip, on again in combat
src/Recharge.*       recharges the held enchanted weapon from the best-fitting soul gem
src/Poison.*         applies the most valuable poison to the drawn weapon (Mod Poison Dose Count perks honoured)
src/QuestAction.*    equips the shout a quest asks for (the Greybeards)
src/PartyOutfit.*    party clothes for Diplomatic Immunity, and back to the stored gear
src/Text.*          Korean/English pairs for every player-facing string; the language comes from CIGAR.json or the game's own text
src/Settings.*      per-module switches, prompt keys, prompt-only switches, eat stage, weapon swap distance and Dress reach, saved to Data/SKSE/Plugins/CIGAR.json
src/Panel.*         SKSE Menu Framework pages (CIGAR / 1. 모듈, 2. 단축키, 3. 세부 설정); the release build shows feature descriptions only
tools/make_release.py   assembles the installable folder and .7z under Downloads: --standard (CIGAR_RELEASE) or --nexus (CIGAR_NEXUS, refuses a DLL naming another mod)
dist/README-release.md  the readme that ships with the release; the player-facing one
dist/README-nexus*.md   the Nexus edition's readmes (English, Korean)
include/TDM/        True Directional Movement API, V1 part (ersh1/TrueDirectionalMovement @ 57b913a)
include/ValhallaCombat/  Valhalla Combat API, V2 part (BSD-3, D7ry/valhallaCombat)
include/SkyPrompt/  SkyPromptAPI header (MIT, QTR-Modding/SkyPromptAPI @ cb4e551)
lib/commonlibsse-ng alandtse/CommonLibVR branch ng (submodule)
tools/Build.ps1     build (VS 2026 Build Tools, Ninja, vcpkg at C:\TAKEALOOK\TOOLS\vcpkg), deploy, verify
tools/register_profile.py  enable the CIGAR mod in MO2 and disable release copies (MO2 closed)
tools/verify_deploy.py     deployment assertions (exit 1 on any failure)
```

On the author's modlist the author build is deployed to `C:\TAKEALOOK\mods\CIGAR\SKSE\Plugins\CIGAR.dll`;
`tools\Build.ps1 -Package` builds the release instead and assembles it under `Downloads\CIGAR <version>`;
`tools\Build.ps1 -Nexus` builds the base-game-only Nexus edition (`docs/035-nexus-edition.md`) as
`Downloads\CIGAR <version> Nexus`.

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

License: CommonLibSSE-NG (alandtse) is GPL-3.0-or-later, so CIGAR is distributed under
GPL-3.0-or-later with this source.

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

A HUD notification appears when SkyPrompt is missing, when prompt IDs collide, when BiS is disabled in
its MCM, or when an action silently fails (each names the log).
