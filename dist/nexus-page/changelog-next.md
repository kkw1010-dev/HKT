# CIGAR 3.1.1

## Changed

- **Pass Time and Lean have their own switches.** The control panel lists them next to Sit, Lie & Lean, so you can keep sitting and lying down while turning off the pass-time prompt or the lean prompts, or the other way round. Both are on by default.
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
- Fixed a cut-short Jujutsu throw killing its target, and hid the throw prompt when an enemy stopped guarding.
- Fixed mannequin swaps interrupted by equipment changes or by switching the module off. Also fixed several smaller gear, helmet, surrender, Observe and soul-gem cases.

## Known issues

- Third-person prompts can remain at the player's position for the rest of a session if the update hook stops reaching CIGAR. CIGAR logs the stalled hook; the underlying cause is still unresolved.
- Jujutsu may refuse to start until the first death after loading a game. Execute can miss its first press; pressing again works.
- The Wizard Warrior's weapon enchantment remains after End Wizard Warrior until it expires, as that mod is designed to do.
