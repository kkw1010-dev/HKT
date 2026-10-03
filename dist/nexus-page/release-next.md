# Next CIGAR release: candidate contents (2026-10-04, not packaged, not uploaded)

Proposed version: **3.2.0** (a new module, so a minor bump; 3.1.2 is the current Nexus and GitHub version).
Base: `v3` after 1792aa1 (the last push). The release texts are drafted in `CHANGELOG.md` "Unreleased".

| Item | Commits | Seen in game | Notes |
|---|---|---|---|
| Shout Ready (VoiceAnswer): equip Unrelenting Force against two or more enemies | 8e03ccd, e516934, 52da765 | **r23 passed** (SHOW / EQUIP / GONE); log 2026-10-03 23:17: hostile 2, enemies 2, offered, equipped (slot was Slow Time) | New combat-page switch 용언 장착 / Shout Ready, on by default. Nothing is put back after the fight (D40) |
| Apply Poison: single press in combat, ring outside | 6381d02 | r17c passed | Rule check covers it |
| Anchor hook runtime guard (Skyrim 1.7 / unknown runtimes) | 6206275 | r17c on 1.6.1170: `hooked on runtime 1-6-1170-0 (slot AD; ...)`; **never run on 1.7** | Keeps the hook as before on SE / AE up to 1.6.1179 / VR (D42) |

Not in it: Cleave and the war stomp (dropped, branch `cleave-proto` only); the Grapple INI change (the user's
own game, not CIGAR); the MannequinSwap "change for the night" idea (not started).

Before packaging: release build (`Build.ps1 -Package`, which also runs the Nexus page check), `check_public.py
--tree`, read the live Nexus description first (`UPLOAD-PROCEDURE.md`), version 3.2.0 in `CMakeLists.txt`,
the description's feature list gains one line for Shout Ready (under Combat), and the panel's new switch is
mentioned. Upload only on the user's word.
