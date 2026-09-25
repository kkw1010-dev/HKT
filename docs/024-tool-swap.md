# 024 · Tool swap (pickaxe at a vein, axe at a tree)

Status (2026-09-24): **removed the same day**, after one in-game run.

## Why it was removed (the user, 2026-09-24)

- It breaks CIGAR's philosophy: a prompt should perform the action, not hand the player a tool
  for it. Vanilla already mines a vein with one E press (the pickaxe only has to be carried).
- Trees: `Woodcutting Tweaks` (Nexus 53538, with `Trees patches for Woodcutting Tweaks`) gives
  every tree it patches a harvest item, `ANDR_TreeLog_MISC` 통나무, with the harvest sound
  `WPNSwingBlunt1Hand`. E on such a tree with a woodcutter's axe carried yields a log, and a
  chopping block turns logs into firewood. So "stand at a tree, chop, get wood" is already one E
  press on this order.
- The in-game run (the tree prompt worked, but many trees never showed) is explained by the same
  mod: `ToolSwap` skipped any TREE with a harvest item as flora, and Woodcutting Tweaks puts the log
  on every tree it patches, so only unpatched trees were offered.

The rest of this file is the record of what was built.

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
  round. A fishing swap would only have equipped the rod earlier, so none is built;
  `verify_deploy.py` fails if Streamlined Fishing is disabled.
  2026-09-25: only the Cast Line half is live here; the rod equip is overridden by Fish Anywhere
  With Water's loose script (`027-fishing.md`).

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
