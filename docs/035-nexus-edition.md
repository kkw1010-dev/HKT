# 035 · Nexus edition (base game only), English text, 유술 damage

Status (2026-09-26): **superseded by 2.1.0, one release for every host.** The base-game-only build
below shipped as `CIGAR 2.0.1 Nexus` (Nexus mod 193080, tag `v2.0.1-nexus`) and passed the user's
in-game test. The rest of this file is its record; the English text and the 유술 damage settings it
introduced stay.

## 2.1.0: integrations return, one package (2026-09-26, later the same day)

The user's decision: other-mod integrations go back into the Nexus upload, as requirements.
- Why it is allowed. CIGAR ships no file of another mod and touches them only at runtime (reads
  forms, settings and script properties, calls their functions, presses their keys). Two authors
  gave direct permission: Smooth (Grapple: "setting my mod as a requirement needs no permission") and
  BakaFactory (hotkey and data scanning, given for BaboPrism). A concern that Nexus would reject links
  to LoversLab adult mods did not hold: Nexus hosts 613 SexLab-named mods and files with off-site
  LoversLab / SubscribeStar requirements, among them BaboDialogue PT-BR (79571), Yamete Kudasai 2.2.3
  PT-BR (190878) and a Fill Her Up translation uploaded by BakaFactory (148200).
- **The Helmet Toggle 2 clips stay the author's** ("헬멧토글은 나만 쓸거"). Only the author build
  (no `CIGAR_RELEASE`) plays them; every release takes the helmet off at once. They were never in a
  package.
- `CIGAR_NEXUS` is gone. The `nexus` preset, `Build.ps1 -Nexus` and `make_release.py --nexus` with its
  other-mod string list are removed; with every integration compiled in, the two release DLLs would
  have been the same. `tools\Build.ps1 -Package` makes `Downloads\CIGAR <version>` with the English
  `README.md` and the Korean `README-ko.md`.
- The hard line is now **other mods' files**, not their names: `make_release.py` fails when the
  package holds anything but `CIGAR.dll`, `CIGAR.json`, the two readmes, `LICENSE.txt` and
  `THIRD-PARTY-NOTICES.txt`.
- `ChairDrink` knows alcohol by both routes: the 29 base-game FormIDs from the Nexus edition and the
  other mods' keywords, so a game without Gourmet or OCF still gets the prompt.
- The readmes list every optional mod with where to get it, and every setting CIGAR changes in
  another mod (Grapple F13, Acheron F14, Valhalla F15, FHU and PNO keys cleared, Grapple's lock key
  set to TDM's).
- Version 2.1.0.

## The user's decisions (2026-09-26)

- A Nexus upload with every other-mod integration taken out.
- SexLab excluded as well (the scene gates).
- The helmet comes off without a motion: the take-off clips are Helmet Toggle 2's and cannot be
  redistributed.
- One DLL for both languages, chosen from the game's language, with a switch in the panel.
- Version 2.0.1, the same as the local release.
- In the Nexus edition, 유술's stamina and health damage are adjustable. They are adjustable in every
  edition, with defaults equal to the old fixed values, so nothing changes for the author build.

## How the edition is built

- CMake option `CIGAR_NEXUS` (preset `nexus`, `build\nexus`; implies `CIGAR_RELEASE`).
  `tools\Build.ps1 -Nexus` builds it and runs `make_release.py --nexus`, which writes
  `Downloads\CIGAR <version> Nexus` and its `.7z`.
- Not compiled: `Bathe`, `BaboKey`, `LockOn`, `Grapple`, `TDMLock`, `Deflate`, `Surrender`, `Eat`,
  `Execute`, `Needs` (the CMake source filter). `main.cpp` registers the other 15 modules.
- Guarded with `#ifdef CIGAR_NEXUS` in the kept modules:
  - scene gates go through `Util::InScene`, which is always false in the Nexus edition (it was a
    SexLab faction lookup in each of `ItemEquip`, `Poison`, `Potion`, `WeaponSwap` and `Jujutsu`);
  - `Util::IsStrippable` checks no other mod's no-strip keywords (SexLab, OStim, Devious Devices);
    non-playable items and the body slots still stay on;
  - `ItemEquip` has no TCL lantern or Fill Her Up armor filter;
  - `WeaponSwap` takes the nearest hostile in combat, never TDM's lock target;
  - `Jujutsu` plays the vanilla kill-move idles only and has no Valhalla stun or execution check;
    the TK Dodge and Private Needs graph variables are not read for its diagnostics;
  - `Helmet` skips the clip and does not look for Helmet Toggle 2;
  - `ChairDrink` knows alcohol by the 29 base-game drink FormIDs (Skyrim, HearthFires, Dragonborn;
    read with houseCARL) instead of other mods' keywords;
  - `Settings` has no eat, needs or prompt-only keys, and the panel no prompt-only page section.
- Kept on purpose: `Rest`'s warm hands reads the fire list of `ccqdrsse001-survivalmode.esl`, the
  Survival Mode Creation Club file that ships free with the current game (without it only warm hands
  is off). `ItemEquip` still skips the Creation Club fishing rod. SkyPrompt is the requirement and
  SKSE Menu Framework the optional panel.
- **Self-check.** `make_release.py --nexus` refuses a DLL that still contains any of 30 other-mod
  strings (plugin names, keyword editor IDs, graph variables) or lacks the edition marker. On
  2026-09-26 the author DLL showed 27 of them and the Nexus DLL none. The load line in `CIGAR.log`
  names the edition: `loaded (Nexus edition, base game only; runtime ...)`.

## Language

- `src/Text.*`: every player-facing string is a pair at its call site, `Text::L("탈의하기", "Get
  Dressed")` or `Text::F(ko, en, args...)`. Log lines stay English.
- `CIGAR.json` `"language"`: `"auto"` (default), `"ko"` or `"en"`. Auto is resolved at kDataLoaded
  from the game's own names of three base-game forms (gold, the iron sword, the lockpick): Hangul in
  any of them means Korean. The Korean patch on this modlist keeps `sLanguage=ENGLISH` and replaces the
  English strings, so the INI setting could not tell. Logged as `language: Korean (auto from the
  game's names ...)`.
- The panel's first page has the language choice; changing it re-resolves and takes every prompt
  down so each comes back in the new language. The three page titles are registered once, so they
  follow at the next launch. The panel is now registered at kDataLoaded (it was kPostLoad) so the
  titles can be in the resolved language.
- English wording is Claude's (the user is not an English speaker).

## 유술 damage

`Settings::JujutsuTuning` gained `staminaDamage` (share of the victim's maximum stamina, default 1.0)
and `healthDamage` (share of its maximum health, default 0.05), both 0-100% sliders on page 3.
Without Valhalla the stamina drain is `min(max * share, current)`; with Valhalla the stun share applies
instead, as before. Health never drops below 1: only the neck break kills. The payoff log line shows
both shares.

## To test in game

Not exercised yet. The first launch of the Nexus DLL should show in `CIGAR.log`: the edition in the
load line, the language line, 15 `settings:` module lines, `ready: alcohol [ 29 of 29 base-game
drinks ]`, and no line naming another mod.

## Test 1 (2026-09-26, `CIGAR.log` 06:55-07:09, one launch)

The user reported all nine test items passed. The log of the last launch shows:

- `CIGAR 2-0-1-0 loaded (Nexus edition, base game only; ...)`, 15 modules, no warning or error line.
- `language: Korean (auto from the game's names '골드' '철 검' '락픽')`; the panel registered at
  kDataLoaded with Korean page titles; switching to English, Korean and back to auto took effect at
  once (prompts re-offered in the new language, e.g. `Drink (hold): 술 - 맥주`).
- ChairDrink: `29 of 29 base-game drinks`; ale offered, drunk seated at the Sleeping Giant Inn (3 -> 2).
- Helmet: taken off without a clip (`clips=false`); the next gate shows `worn=-`. The line's
  `unequip=false` is `UnequipObject`'s return for a queued unequip, not a failure.
- Poison applied (`poisoned after=true`), spell tome read and learned, a health potion drunk,
  ItemEquip accepted, Observe and Rest offered.
- Not in this log (the log is recreated each launch, so they may be from an earlier launch): 유술 and
  its damage settings (`CIGAR.json` still holds the defaults 1.00 / 0.05), 투구 쓰기 in combat, the page
  titles after a relaunch in English. `CIGAR.json` was left on `"language": "en"`.
- Other mods named in the log: only the field names `tdm=false` (WeaponSwap) and `valhalla=false`
  (Jujutsu) in two ready lines.

After test 1 (the user's go-ahead): the Nexus edition's ready lines drop those two fields, `tdm=` and
`valhalla=` joined the forbidden strings (30), and Helmet's line now reads `unequip queued (call
returned ...)` in every edition. Both packages were rebuilt; the Nexus upload is
`Downloads\CIGAR 2.0.1 Nexus.7z` (DLL sha256 3950258b...).

## Nexus page and GitHub release (2026-09-26)

- **Nexus:** mod 193080, "Cigar - One-Button Interaction". The user asked for a thorough review of
  the published page; Claude found and fixed, with the user's authorization:
  - requirements grouped SKSE and SkyPrompt as "pick one of these"; now SkyPrompt AND Address Library
    AND SKSE (Steam or GOG);
  - every readme and the release notes linked SkyPrompt to mod 149963 (Slower Swimming); SkyPrompt is
    148703;
  - permissions ("no re-upload", "ask to modify") contradicted GPL-3.0; now a custom GPL statement
    with the source link and credits for the libraries in the DLL;
  - the description promised "Followers", "supported external mods" and runtime optional
    integrations this edition does not have, and never said gamepads are unsupported; the corrected
    text keeps the user's own sections and wording elsewhere. Original and new texts are kept in
    `dist/nexus-page/`;
  - the file had no description or changelog.
- The package gained `LICENSE.txt` and a generated `THIRD-PARTY-NOTICES.txt`; `CIGAR.dll` is
  unchanged (sha256 3950258b...). On Nexus the new archive updated the file and the old one is
  archived (7 downloads had happened before the fix).
- **SkyPrompt 2.4.0** (what players download now) keeps API 2.0: the same structs, event numbers and
  exports CIGAR calls (checked against its DLL exports and the upstream header). CIGAR was run only
  with 2.3.15.
- **GitHub:** release `v2.0.1-nexus` notes corrected; asset `CIGAR-2.0.1-Nexus.7z` added; the old
  asset is renamed `superseded-do-not-use-CIGAR.2.0.1.Nexus.7z` (deleting it is left to the user).
