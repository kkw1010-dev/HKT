# CIGAR 3.1.2 (changes since 3.1.0)

3.1.1 was published on GitHub only; its changes are included here.

## Changed

- **Rest has a master switch and four switches of its own.** In the settings panel, Rest turns sitting, lying down, leaning and passing time on or off together. Beneath it, Sit, Lie Down, Lean and Pass Time each have a switch, so you can, for example, keep the lean prompts and turn off sitting and lying down. All are on by default.
- **The settings panel uses two columns**, so more modules fit on a page.
- **A much wider range for the third-person prompt position.** The side offset under CIGAR > Options now goes from 200 to the left to 200 to the right (it was 0 to 30 to the right), so you can move the prompts out of the way. Large offsets can put the prompts off screen, so pick a value that keeps them visible. At the default position the prompts are also kept on screen: if their spot would fall outside the view, they move to the nearest spot still visible.
- **Reading:** unread scripted notes, including notes that start quests, now get the Read prompt when you take them from a body, bag or chest; previously only delivered letters did.
- **Squeeze Past** now gently nudges the blocking person aside as you pass.
- **Jujutsu is now called Throw** in the English prompts and panel. Nothing else about it changed, and your settings carry over.

## Fixed

- **Attempted fix for third-person prompts sticking to the player (not yet confirmed in game).** The prompts could stop following their place near the character when another mod took over the update hook CIGAR uses to move them. CIGAR now attempts to restore its hook while preserving the other mod's hook. The problem has not come up in testing since the change, so it stays under Known issues.

## Known issues

- Third-person prompts may still stick to the player's position for the rest of a session (see the attempted fix above). CIGAR logs a stalled update hook in `CIGAR.log`.
- Throw may refuse to start until the first death after loading a game. Execute can miss its first press; pressing again works.
- The Wizard Warrior's weapon enchantment remains after End Wizard Warrior until it expires, as that mod is designed to do.

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
- Fixed a cut-short throw killing its target, and hid its prompt when an enemy stopped guarding.
- Fixed mannequin swaps interrupted by equipment changes or by switching the module off. Also fixed several smaller gear, helmet, surrender, Observe and soul-gem cases.

## Known issues

- Third-person prompts can remain at the player's position for the rest of a session if the update hook stops reaching CIGAR. CIGAR logs the stalled hook; the underlying cause is still unresolved.
- Throw may refuse to start until the first death after loading a game. Execute can miss its first press; pressing again works.
- The Wizard Warrior's weapon enchantment remains after End Wizard Warrior until it expires, as that mod is designed to do.
