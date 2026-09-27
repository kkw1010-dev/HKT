# Nexus upload: CIGAR 3.0.0 (uploaded 2026-09-27, file replaced 2026-09-28)

## Current state (2026-09-28): the trimmed release

The user decided the release is English with feature descriptions only. The orchestrator session
applied it on Nexus with the user's approval:

- File: `CIGAR 3.0.0 (docs trimmed).7z` uploaded as an Update of 811289, now **file 811321**
  (`CIGAR`, 3.0.0, Main, primary, 917,088 bytes); 811289 is Old versions. Same DLL (sha256
  `afef2eb1...`), no `README-ko.md`. Mod version stays 3.0.0.
- Description: `description-3.0.0-trimmed.bbcode`, now also `description-live.bbcode` (checked
  through the public GraphQL, sha256 `ba8b73bf...` after restoring entities and `<br />`). The text
  before the trim is kept as `description-3.0.0-untrimmed.bbcode`.
- Logs, 3.0.0: the seven lines of `changelog-3.0.0-trimmed.txt` (replaced whole with Edit changelog,
  because the upload merged them with the old eight).
- File page: https://www.nexusmods.com/skyrimspecialedition/mods/193080?tab=files&file_id=811321

The rest of this file records the first upload of 2026-09-27.

Uploaded on 2026-09-27 as Nexus file 811289 (`CIGAR`, 3.0.0, Main, primary, 922,079 bytes), an
Update of 2.1.2 (now Old versions). Mod version is 3.0.0, the file changelog carries the eight
lines below, the description is `description-live.bbcode` (identical once Nexus's `<br />` are
stripped) and The Wizard Warrior (14890) is a legacy requirement. All checked through the public v2
GraphQL after saving. File page:
https://www.nexusmods.com/skyrimspecialedition/mods/193080?tab=files&file_id=811289

An earlier attempt was stopped before the edit page opened (permission check: "Create Public
Surface"); the texts below are what was entered.

## The file

- Archive: `%USERPROFILE%\Downloads\CIGAR 3.0.0.7z`, 922,079 bytes
  - `SKSE\Plugins\CIGAR.dll`: 4,641,280 bytes, sha256 `afef2eb1d5efbfe7a65b6cf9a688e3392d50f54f751e881f9e204a8810588440`
  - `SKSE\Plugins\CIGAR.json`, `README.md`, `README-ko.md`, `LICENSE.txt`, `THIRD-PARTY-NOTICES.txt`
  - `make_release.py` checked: only these six files, `SKSE` at the archive root, no user-profile
    path in any file
- Nexus 193080, Files → add as an **Update** of the 2.1.2 main file (moves 2.1.2 to old versions)
- Name: `CIGAR`; version: `3.0.0`; category: Main Files
- Mod version (page header): `3.0.0`

File description:

```
3.0.0: mannequin outfit swap, Wizard Warrior Mode, prompts ahead of the character in third person (adjustable), settings pages split into Combat / Non-combat / Mod integrations. No new game needed. See the Logs tab for the full changelog.
```

## Changelog entry (Logs tab, version 3.0.0), one line per change

```
New: Mannequin outfit swap. Looking at a mannequin, one hold prompt swaps what you wear with what it wears; enchanted and tempered pieces move as they are, and its helmet comes to you stowed.
New: Wizard Warrior Mode (with The Wizard Warrior). Drawing a weapon offers to turn it on; sheathing out of combat offers to turn it off.
New: In third person, prompts sit just ahead of your character's head, a little to the right; the distance to the right is a setting (default 15). First person is unchanged.
Changed: The settings panel's module switches are split into Combat, Non-combat and Mod integrations; Keys is page 4 and Options page 5, with status and language at the top of Options.
Fixed: Relief prompts no longer show while Bathing in Skyrim is washing you.
Note: CIGAR now keeps one invisible, disabled marker in each save for prompt placement; it does nothing if CIGAR is removed.
Known issue: A weapon enchantment stays after End Wizard Warrior until it runs out; that is The Wizard Warrior's own design.
Tested with the Unofficial Patch's mannequin script and Another Mannequin Script Fix; the unpatched base-game script is supported but untested.
```

## Description and requirements

- Description: paste `description-live.bbcode` (updated for 3.0.0).
- Requirements (legacy method, like the others): add The Wizard Warrior - Spellsword Magic Combat
  Evolved (Nexus 14890), note "Not necessary, but recommended" (Wizard Warrior Mode prompts).
- Summary: unchanged (`summary-live.txt`).
