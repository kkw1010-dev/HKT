<!-- DRAFT, not for upload as is. The next Nexus upload is 3.1.2 and carries 3.1.1 with it (3.1.1 went to
     GitHub only; the Nexus page is still at 3.1.0).
     Seen in game (r14, 2026-10-02): the Sit and Lie Down switches, the two-column panel.
     NOT seen in game: the fix for prompts stuck on the player (r14 did not reproduce the problem, so the
     re-install never ran). It is worded below as an attempt, and the known issue stays.
     Not seen yet either (r15): the Rest switches drawn under the master switch.
     Delete this block before uploading. -->

# CIGAR 3.1.2

## Changed

- **Sit and Lie Down have their own switches too.** With Pass Time and Lean, the control panel now lists Sit and Lie Down, so you can keep the lean prompts and turn off sitting, lying down or both. All on by default.
- **The control panel lists the modules in two columns**, with the Sit, Lie Down, Lean and Pass Time switches under the Rest switch, which turns all of them on or off.

## Fixed

- **Prompts stuck on the player in third person (attempt).** The prompts could stop following their place beside the character when another mod took over the game function CIGAR uses to move them. CIGAR now takes its place back when that happens and keeps the other mod's part running. This case has not come up in testing since the change, so it stays under Known issues for now.

---

# CIGAR 3.1.1

## Changed

- **Pass Time and Lean have their own switches.** The control panel lists them next to Sit, Lie & Lean, so you can keep sitting and lying down while turning off the pass-time prompt or the lean prompts, or the other way round. Both are on by default.
- **A much wider range for the third-person prompt position.** The side offset under CIGAR > Options now goes from 200 to the left to 200 to the right (it was 0 to 30 to the right), so you can move the prompts out of the way, for example while observing. A far position can leave the screen; that is yours to pick. When the position is left at its default, the prompt mark is also kept on screen: if it would fall outside the view it moves to the nearest spot still visible.
- **Jujutsu is now called Throw** in the English prompts and panel. Nothing else about it changed, and your settings carry over.
- **Reading:** an unread note that carries a script (the kind that starts a quest when read) now gets the read prompt when you pick it up from a body, a bag or a chest, not only letters delivered to you.
- **Squeeze Past** eases the person aside a little as you pass.

---

# CIGAR 3.1.0 (changes since 3.0.0)

## New

- **Squeeze Past:** Hold the prompt to slip past a person blocking your way, whether they are standing or walking. It does not target enemies or someone talking to you. This needs your own copies of EVG Animated Traversal and Offset Movement Animation; without EVG, the feature stays off. CIGAR includes no animation from either mod.
- **More prompt keys:** The Keys page supports D-pad prompt keys and the mouse's middle and side buttons. Gamepad input was checked with one Xbox controller.

## Changed

- Non-combat actions now fill a hold ring (Squeeze Past starts on the press instead). A tap does nothing, and a double tap can dismiss the prompt. Combat actions remain single presses. Potions use a hold ring outside combat and a single press in combat.
- A dismissed prompt stays hidden until its situation changes. A new item, quest, book or target can bring its prompt back.
- Routine blocked-prompt diagnostics now go to `CIGAR.log` instead of the HUD. A broken integration or failed action still gives a notice when the player needs to know.

## Fixed

- Prevented a crash when a prompt changed while SkyPrompt was drawing it, and a load-time crash when another mod's script was not linked.
- Prevented an old prompt from returning on a key already used by another prompt, and fixed Pass Time staying fast after saving while it was held.
- Fixed a cut-short Throw killing its target, and hid the throw prompt when an enemy stopped guarding.
- Fixed mannequin swaps interrupted by equipment changes or by switching the module off. Also fixed several smaller gear, helmet, surrender, Observe and soul-gem cases.

## Known issues

- Third-person prompts can remain at the player's position for the rest of a session if the update hook stops reaching CIGAR. CIGAR logs the stalled hook; the underlying cause is still unresolved.
- Throw may refuse to start until the first death after loading a game. Execute can miss its first press; pressing again works.
- The Wizard Warrior's weapon enchantment remains after End Wizard Warrior until it expires, as that mod is designed to do.
