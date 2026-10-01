# Nexus upload: CIGAR 3.1.2 (prepared 2026-10-02)

Nexus mod 193080 (https://www.nexusmods.com/skyrimspecialedition/mods/193080). The page was at 3.1.0;
3.1.1 went to GitHub only, so this upload carries both.

- Archive: `%USERPROFILE%\Downloads\CIGAR 3.1.2.7z`, 983,169 bytes,
  sha256 `05a28c4a8780921bd6233449147f7b47dbc2740f911eb575e5ff590e99678ad3`
  - `SKSE\Plugins\CIGAR.dll`: 5,050,880 bytes, sha256 `bf802224fbfd06b86bbd0480f2fbfac20a73e5190869a830b3414c753b1683f6`
  - `SKSE\Plugins\CIGAR.json`, `README.md`, `LICENSE.txt`, `THIRD-PARTY-NOTICES.txt`
  - `make_release.py` passed: only these five files, no personal module, no private word, no user path.
- Add as an **Update** of the 3.1.0 main file. Name `CIGAR`, version `3.1.2`, Main Files; mod version `3.1.2`.
- File description: `3.1.2: Rest master switch with separate Sit, Lie Down, Lean and Pass Time switches, two-column settings panel, wider third-person prompt offset, Read prompt for scripted notes, Throw (was Jujutsu). No new game needed.`
- Logs, version 3.1.2: the lines of `changelog-3.1.2.txt`, one per change.
- Description: `description-next.bbcode` (becomes `description-live.bbcode` once posted).

What was and was not run in the game: every change was seen in the author build (r13 to r16), except the
attempted fix for prompts sticking to the player, which never triggered. The release binary itself (the
`dist` preset, built from the same source) has not been run in the game.
