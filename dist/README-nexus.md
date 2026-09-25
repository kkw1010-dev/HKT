# CIGAR

An SKSE plugin that turns the things you do over and over in Skyrim into **on-screen prompts**.
A prompt appears only while the action makes sense, and one key does the whole action.

No plugin file (.esp/.esl): nothing is added to your load order, and you can remove it at any time.

This is the **base-game edition**: every prompt works with Skyrim alone, and CIGAR reads, changes or
depends on no other mod. Text is in English or Korean, following your game's language.

## Requirements

- [SKSE64](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- [SkyPrompt](https://www.nexusmods.com/skyrimspecialedition/mods/148703), which draws the prompts. Without it nothing shows.
  SkyPrompt itself needs SKSE Menu Framework and ImGui Icons.
- [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) also hosts CIGAR's
  in-game settings panel. CIGAR works without the panel.

Tested on Skyrim 1.6.1170 (Anniversary Edition) with SkyPrompt 2.3.15; SkyPrompt 2.4.0 keeps the
same API (2.0). Made for keyboard and mouse; gamepads are not supported.

Prompt text follows the language CIGAR uses; item, quest and character names come from your game,
so a translated game shows its own names inside English prompts.

## Installation

Install the archive with Mod Organizer 2 or Vortex. No new game is needed.

## Using the prompts

- A prompt stays up while its situation lasts and goes away on its own when it ends.
- Up to four prompts show at once, each on its own key. The default keys are 1, 2, 3 and 4.
- Most actions outside combat are **hold** prompts, so a stray tap changes nothing.
- **Double-tap** a prompt to dismiss it: pass time, helmet, chair drinking and observe then stay away
  until the situation changes.
- Prompts hide while a menu is open (inventory, map, dialogue and so on).

## Prompts

### Daily life

| Prompt | When |
|---|---|
| Undress / Get Dressed | At a bed or wardrobe, and in water. What you take off is remembered and put back on |
| Sit / Lie Down | Standing still with the weapon sheathed, looking down at the floor. Moving gets you up slowly |
| Lean on Wall / Table / Railing | Facing a wall, a table or a railing |
| Warm Hands | In front of a campfire, brazier or hearth (see below) |
| Pass Time (hold) | While sitting, lying or seated in a chair. Time flies while you hold it |
| Drink (hold): <drink> | Seated in a chair at an inn or a home, with alcohol in your pack |
| Observe (hold) | Standing still for 5 s with the weapon sheathed, looking at a person or a distant view. Zooms in |

### Gear and items

| Prompt | When |
|---|---|
| Equip (hold): <item> | For 15 s after you get a new weapon or piece of armor. Armor only when it beats what you wear, and never over an enchanted piece |
| Read (hold): <book> | For 15 s after you get a spell tome you do not know yet, or a quest note or book |
| Take Off Helmet (hold) / Put On Helmet | The helmet stays off by default so your face shows. Outside dungeons you get the take-off prompt while wearing one, and in combat the prompt to put it back on |
| Recharge (hold): <weapon> | Out of combat, when the enchanted weapon in your hand is at 25% charge or less. The best-fitting soul gem is used |
| Apply Poison (hold): <poison> | Weapon drawn, no poison on it, and a poison in your pack |
| Drink: <potion> | Low health, stamina or magicka, poisoned, diseased, or under water |

### Quests

| Prompt | When |
|---|---|
| Track (hold): <quest> | For 15 s after an untracked quest gets a new objective |
| Equip (hold): <shout> | When the Greybeards ask to see a shout |
| Wear Party Clothes / Back to Own Gear | During the Thalmor embassy party |

### Combat

| Prompt | When |
|---|---|
| Ranged Weapon / Melee Weapon | When the enemy's distance does not suit the weapon in your hands (bows and crossbows only with ammo) |
| Jujutsu | A blocking humanoid enemy is close. Four throws that knock it down and break its guard, and a neck break that kills |

## Settings (SKSE Menu Framework)

The in-game menu has a **CIGAR** section with three pages.

1. **Modules**: switch each feature on or off; a feature switched off takes its prompts away at once.
   The **language** is chosen here too: Auto (your game's language), 한국어 or English.
2. **Keys**: the four prompt keys.
3. **Options**: when prompts start (potion thresholds, weapon swap distance, jujutsu reach, bed and
   wardrobe reach, pass time speed) and **jujutsu damage**: the share of the enemy's maximum stamina
   (default 100%) and maximum health (default 5%) a throw takes. A throw always leaves at least 1
   health; only the neck break kills.

Your choices are saved to `SKSE/Plugins/CIGAR.json`.

## Good to know

- **Warm Hands** finds fires with the list from the Survival Mode Creation Club file
  (`ccqdrsse001-survivalmode.esl`), which comes free with the current game. Without it that one prompt
  never shows; everything else works.
- **Chair drinking** knows the 29 drinks of the base game and its DLCs. Drinks added by other mods
  are not offered.
- **The helmet** comes off at once, without an animation.
- Page names in the settings panel switch language at the next launch; everything else switches at
  once.

## Known issues

- **Jujutsu does nothing until someone has died since the game was loaded.** Once anyone (any enemy)
  dies in the session, it works normally. Until then the prompt shows but the move does not play. The
  cause appears to be a kill-move state inside the game engine. Changing that value directly might fix
  it, but there is no way to check what else such a change would affect, so it is **deliberately left
  alone**. Refusals in that window raise no warning; the log gives the reason.

## If something goes wrong

Every launch writes `Documents/My Games/Skyrim Special Edition/SKSE/CIGAR.log`. It says why a prompt
did not show or an action did not happen, so please attach it when you report a problem.

## License

CIGAR uses CommonLibSSE-NG, so it is **GPL-3.0-or-later**. The source is at
<https://github.com/kkw1010-dev/HKT>.
