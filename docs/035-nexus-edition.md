# 035 · Nexus edition (base game only), English text, 유술 damage

Status (2026-09-26): built and packaged as `CIGAR 2.0.1 Nexus`; **not yet run in game.**

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
- **Self-check.** `make_release.py --nexus` refuses a DLL that still contains any of 28 other-mod
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
