# 024 · Tool swap (pickaxe at a vein, axe at a tree)

Status (2026-09-24): built as module `ToolSwap` and deployed; not yet tested in game. SI's
`WeaponSwap.enabled` is now in the replaced list. The module name keeps clear of CIGAR's own
`WeaponSwap` (ranged/melee in combat, `docs/010`).

## What SI does (settings, MCM text and DLL strings only)

- `WeaponSwap.enabled` (on): "If enabled, the mod will prompt to swap weapons to useful tools like a
  pickaxe or a woodcutter's axe. Note that the prompts will be shown when the player is in a location
  where these tools are useful, e.g. near a tree or an ore vein."
- Scenes `TreeWeaponSwap` (`IsTree`, logging a height), `VeinWeaponSwap`, `FishingWeaponSwap`, all
  through `ItemSwapper<WeaponSwapper>`; references gathered with `ForEachReferenceInRange`.
- Prompt text `SwapWeapon` = "무기 교체".
- Unknown: the ranges, the height, whether SI gives the old weapon back.

## On this load order

- Ore veins are activators running `MineOreScript`. The winning copy is loose in
  `Faster Mining Plus SE + Mining Makes Noise + USSEP Scripts Merged`. Its `OnHit` counts strikes
  from a weapon in the vein's `mineOreToolsList` (`proccessAttackStrikes`), so a pickaxe in hand
  mines by striking; `ResourceCountCurrent` 0 is a depleted vein. Tool lists: `MineOreToolsList`
  (`10ACC4:Skyrim.esm`: pickaxe, Rocksplinter, Notched Pickaxe) and Dragonborn's Stalhrim list.
- Trees: `woodChoppingAxes` (`10ACCC:Skyrim.esm`: Axe01 woodcutter's axe, Poacher's Axe). Vanilla
  has no tree chopping by striking, so the axe is roleplay.
- TREE records cover flora too. In Skyrim.esm, harvestable flora has an ingredient, and shrubs,
  ferns and kelp top out below 400 units; pines, aspens and Reach trees stand 676-2501.
- `Pickaxe MCO` gives the pickaxe its own attack animations.
- Fishing: `Streamlined Fishing` (parapets, Nexus 80683) equips a fishing rod when the Fishing
  Supplies are activated (a menu when several rods are carried), and offers Cast Line after each
  round. SI's fishing swap would only have equipped the rod earlier, so it is not rebuilt;
  `verify_deploy.py` fails if Streamlined Fishing is disabled.

## As built

- `src/ToolSwap.cpp`, once a second, within 600 units:
  - a vein: an activator with `MineOreScript` attached, not depleted, its bounds within 200 units
    and within 60 deg of facing (bounds, because a vein's origin sits inside the rock);
  - a tree: a TREE with no ingredient and a top at least 450 units up (scaled), its trunk (origin)
    within 250 units and within 45 deg (not the bounds, which are the canopy).
  - A vein wins over a tree beside it.
- 곡괭이 들기 / 도끼 들기 (길게): <도구> shows out of combat, not mounted or seated, with movement
  controls on, when the player carries a tool from that target's list and the right hand does not
  already hold one. Pick: favourites first, then the highest damage.
- Accept remembers the right hand (weapon, spell or nothing; a second swap keeps the first one) and
  equips the tool in the right hand. The left hand is left alone.
- 무기 되돌리기: <이전 무기> shows once the player has faced no vein or tree for 2 s, or at once in
  combat. It is a **single press**, the documented exception to the hold rule, because it is
  offered in combat. Accept equips what was remembered, or puts the tool away when the hand was
  empty or the weapon is gone.
- If the player changes the right hand themselves, the remembered weapon is forgotten.
- Self-report: 1.5 s after either action the log compares the right hand with what was equipped,
  and a notification fires on a mismatch. The gate line lists veins, depleted veins and trees in
  range, so a missing prompt is explained.

## Open

- In-game: the reach and cone numbers (CIGAR's choices), the tree height against modded trees,
  and whether striking a vein with the pickaxe yields ore on this order.
