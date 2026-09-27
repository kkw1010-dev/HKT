# Changelog

Player-facing changes per release. The Nexus page (mod 193080) carries the same entries.

## 3.0.0 (2026-09-27)

New
- **Mannequin outfit swap.** Looking at a mannequin, one hold prompt swaps what you wear with what it
  wears (Store Outfit or Wear Mannequin's Outfit when one side is empty). Enchanted and tempered pieces
  move as they are, and the mannequin's helmet comes to you stowed. A mannequin that cannot take the
  outfit (too few free slots, or a second copy of the same armor) is refused with a notification, and
  a swap that fails is undone. Tested with the Unofficial Skyrim Special Edition Patch's mannequin
  script and with Another Mannequin Script Fix; the unpatched base-game script is supported but was
  not tested.
- **Wizard Warrior Mode** (with The Wizard Warrior installed). Drawing a weapon while it is off offers
  to turn it on; sheathing out of combat while it is on offers to turn it off. Its keys and MCM stay
  with The Wizard Warrior.
- **Prompt position.** In third person, prompts now sit just ahead of your character's head, a little
  to the right, instead of beside the face. How far to the right is a setting (0, 10, 15, 20 or 30;
  default 15). First person keeps the prompts on your character. For this, CIGAR keeps one invisible,
  disabled marker in each save; it does nothing if CIGAR is removed.

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

- Observe zooms in first person too: it now changes the world field of view, not the first-person arms
  view. A warning is logged if another mod overrides that value.
- Pass Time held on a gamepad no longer stops after a fraction of a second.

## 2.1.0 (2026-09-26)

- One release for every host: the prompts for other mods are back in the Nexus download. Each one
  works only when its mod is installed, and the package holds none of those mods' files.
- The archive has `SKSE` at its root, and the download carries an English and a Korean readme.
- Chair drinking recognises alcohol by the base game's drinks and by other mods' alcohol keywords.

## 2.0.1 (2026-09-26)

- First Nexus release.
