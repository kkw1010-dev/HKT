# CIGAR

An SKSE plugin that turns the things you do over and over in Skyrim into **on-screen prompts**.
A prompt appears only while the action makes sense, and one key does the whole action, so you no
longer need to remember other mods' hotkeys.

No plugin file (.esp/.esl): nothing is added to your load order, and you can remove it at any time.
Text is in English or Korean, following your game's language.

## Requirements

- [SKSE64](https://skse.silverlock.org/)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- [SkyPrompt](https://www.nexusmods.com/skyrimspecialedition/mods/148703), which draws the prompts. Without it nothing shows.
  SkyPrompt itself needs SKSE Menu Framework and ImGui Icons.
- [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) also hosts CIGAR's
  in-game settings panel. CIGAR works without the panel.

Everything else is optional; see "Other mods" below. Tested on Skyrim 1.6.1170 (Anniversary Edition)
with SkyPrompt 2.3.15; SkyPrompt 2.4.0 keeps the same API (2.0). Made for keyboard and mouse;
gamepads are not supported.

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

Prompts marked with a mod name need that mod; the rest work with Skyrim alone.

### Daily life

| Prompt | When |
|---|---|
| Bathe / Shower | Naked in water, or under a waterfall. *Bathing in Skyrim - Renewed* |
| Undress / Get Dressed | At a bed or wardrobe, and in water. What you take off is remembered and put back on |
| Eat: <food> | When you are hungry. Cheapest food first; raw meat, drinks and spoiled food are skipped. *Survival Mode* |
| Urinate / Defecate | When bladder or bowels are full. *Private Needs - Orgasm* |
| Deflate (hold) | While inflated. *Fill Her Up Baka Edition* |
| Sit / Lie Down | Standing still with the weapon sheathed, looking down at the floor. Moving gets you up slowly |
| Lean on Wall / Table / Railing | Facing a wall, a table or a railing |
| Warm Hands | In front of a campfire, brazier or hearth |
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
| Choose Action | While held in BaboDialogue's kidnap room. *BaboDialogue* |

### Combat

| Prompt | When |
|---|---|
| Lock On | In combat while no target is locked. *True Directional Movement* |
| Grapple | In combat with an enemy close by. *Grapple* |
| Ranged Weapon / Melee Weapon | When the enemy's distance does not suit the weapon in your hands (bows and crossbows only with ammo) |
| Execute | An enemy's stun is broken. *Valhalla Combat* |
| Jujutsu | A blocking humanoid enemy is close. Four throws that knock it down and break its guard, and a neck break that kills |
| Surrender (hold) | In combat below 40% health. *Acheron* |

## Other mods

CIGAR **uses these mods when they are installed and quietly skips the prompt when they are not.**
It has no masters, ships no patches and none of these mods' files, and needs no switch to detect
them.

| Prompt | Mod | Where |
|---|---|---|
| Bathe, Shower | Bathing in Skyrim - Renewed | [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/135288) |
| Lock On | True Directional Movement | [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/51614) |
| Grapple | Grapple, by Smooth | Smooth's Patreon |
| Execute | Valhalla Combat | [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/64741) |
| Surrender | Acheron | [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/108159) |
| Surrender (knows its 3-minute rule) | Yamete Kudasai | [LoversLab](https://www.loverslab.com/files/file/23123-yamete-kudasai/) |
| Eat | Survival Mode (Creation Club, free with the current game) and [Survival Mode Improved - SKSE](https://www.nexusmods.com/skyrimspecialedition/mods/78244) | |
| Eat (skips Gourmet's non-meals) | Gourmet | [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/96876) |
| Deflate | Fill Her Up Baka Edition, by BakaFactory | LoversLab / SubscribeStar |
| Urinate, Defecate | Private Needs - Orgasm | [LoversLab](https://www.loverslab.com/files/file/39023-private-needs-orgasm/) |
| Choose Action | BaboDialogue, by BakaFactory | LoversLab / SubscribeStar |

Warm Hands finds fires with the list from the Survival Mode Creation Club file
(`ccqdrsse001-survivalmode.esl`); without it that one prompt never shows.

### Settings CIGAR changes in other mods

So that one key does not do two things, CIGAR can take a mod's own hotkey over. This is the
**prompt only** switch on the Keys page, **on by default**, and switching it off gives the key back.

| Mod | While prompt only is on |
|---|---|
| Grapple | its hotkey moves to F13, a key keyboards never send |
| Acheron | the surrender key moves to F14 (through Acheron's own settings, saved with your game) |
| Valhalla Combat | the execution key moves to F15 |
| Fill Her Up Baka Edition | the deflate hotkey is cleared |
| Private Needs - Orgasm | its six hotkeys are cleared, including its menu and status keys |

One change does not depend on the switch: with True Directional Movement installed, Grapple's target
lock key is set to TDM's lock key, which Grapple presses to release the lock during a grapple.

If you change one of these keys in the other mod's menu, press **Check mod keys again** on the Keys
page once.

## Settings (SKSE Menu Framework)

The in-game menu has a **CIGAR** section with three pages.

1. **Modules**: switch each feature on or off; a feature switched off takes its prompts away at once.
   The **language** is chosen here too: Auto (your game's language), 한국어 or English.
2. **Keys**: the four prompt keys, and the prompt only switches above. A clash with another mod's key
   is shown.
3. **Options**: when prompts start (potion thresholds, hunger stage, needs level, weapon swap
   distance, jujutsu reach, bed and wardrobe reach, pass time speed) and **jujutsu damage**: the share
   of the enemy's maximum stamina (default 100%) and maximum health (default 5%) a throw takes. A throw
   always leaves at least 1 health; only the neck break kills.

Your choices are saved to `SKSE/Plugins/CIGAR.json`.

## Good to know

- **Chair drinking** knows the 29 drinks of the base game and its DLCs, plus any drink another mod
  tags as alcohol (Gourmet, Object Categorization Framework, SunHelm).
- **The helmet** comes off at once, without an animation.
- Page names in the settings panel switch language at the next launch; everything else switches at
  once.

## Known issues

- **Jujutsu does nothing until someone has died since the game was loaded.** Once anyone (any enemy)
  dies in the session, it works normally. Until then the prompt shows but the move does not play. The
  cause appears to be a kill-move state inside the game engine. Changing that value directly might fix
  it, but there is no way to check what else such a change would affect, so it is **deliberately left
  alone**. Refusals in that window raise no warning; the log gives the reason.
- **Execute can miss the first press.** Press again.

## If something goes wrong

Every launch writes `Documents/My Games/Skyrim Special Edition/SKSE/CIGAR.log`. It says why a prompt
did not show or an action did not happen, and which of the mods above were found, so please attach
it when you report a problem.

## License

CIGAR uses CommonLibSSE-NG, so it is **GPL-3.0-or-later**. The source is at
<https://github.com/kkw1010-dev/HKT>.
