# Changelog

Player-facing changes per release. The Nexus page (mod 193080) carries the same entries.

## Unreleased

New
- **D-pad for gamepads.** The Keys page has a gamepad setting: SkyPrompt's own buttons (the default,
  unchanged) or the D-pad, with prompt 1 on Up, 2 Down, 3 Left and 4 Right. While a prompt shows, its
  direction runs the prompt instead of Favorites or a hotkey; with no prompt the D-pad works as usual.
  Tested with an Xbox controller; PlayStation controllers were not tested.
- **Mouse buttons as prompt keys.** The middle and side mouse buttons can be picked as prompt keys.
  The left and right buttons stay attack and block.

Fixed
- **A crash while prompts change.** SkyPrompt 2.4.0 could crash while drawing if a prompt was taken
  away at the same moment (for example when drawing a weapon ended the Observe prompt). CIGAR now
  sends and removes its prompts on the thread SkyPrompt draws on.
- **Jujutsu** no longer keeps its prompt up for an enemy that has dropped its guard to attack; the
  throw could not start then anyway.

## 3.0.0 (2026-09-27)

New
- **Mannequin outfit swap.** Looking at a mannequin, one hold prompt swaps what you wear with what it
  wears. Enchanted and tempered pieces move as they are, and the mannequin's helmet comes to you
  stowed.
- **Wizard Warrior Mode** (with The Wizard Warrior installed). Drawing a weapon while it is off offers
  to turn it on; sheathing out of combat while it is on offers to turn it off. Its keys and MCM stay
  with The Wizard Warrior.
- **Prompt position.** In third person, prompts now sit just ahead of your character's head, a little
  to the right, instead of beside the face. How far to the right is a setting (0, 10, 15, 20 or 30;
  default 15). First person keeps the prompts on your character. CIGAR keeps one invisible marker in
  each save for this; it is harmless if CIGAR is removed.

Changed
- **Settings panel pages.** The module switches are split into Combat, Non-combat and Mod
  integrations. Keys is now page 4 and Options page 5; the SkyPrompt status and the language moved to
  the top of Options.

Fixed
- Relief prompts no longer show while Bathing in Skyrim is washing you.

Known issues
- A weapon enchantment stays after End Wizard Warrior until it runs out. That is The Wizard Warrior's
  own design.

## 2.1.2 (2026-09-26)

- Combat, gear and needs prompts also stay away while an OStim scene plays the player, as they already
  did for SexLab.

## 2.1.1 (2026-09-26)

- Observe now zooms in first person too.
- Pass Time held on a gamepad no longer stops after a fraction of a second.

## 2.1.0 (2026-09-26)

- The prompts for other mods are back in the Nexus download; each works only when its mod is
  installed.
- The archive installs directly with any mod manager.
- Chair drinking also recognises drinks that other mods mark as alcohol.

## 2.0.1 (2026-09-26)

- First Nexus release.
