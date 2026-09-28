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
with SkyPrompt 2.4.0 and 2.3.15. Prompt keys can be keyboard keys or the mouse's middle and side
buttons. A gamepad uses SkyPrompt's own gamepad buttons, or the D-pad if you pick that on the Keys page.
Gamepad support was tested with an Xbox controller; PlayStation controllers were not tested.

Prompt text follows the language CIGAR uses; item, quest and character names come from your game,
so a translated game shows its own names inside English prompts.

## Installation

Install the archive with Mod Organizer 2 or Vortex. No new game is needed.

## Using the prompts

- A prompt stays up while its situation lasts and goes away on its own when it ends.
- Up to four prompts show at once, each on its own key. The default keys are 1, 2, 3 and 4.
- Most actions outside combat are **hold** prompts, so a stray tap changes nothing.
- **Double-tap** a prompt to dismiss it: pass time, helmet, chair drinking, observe and Wizard Warrior
  then stay away until the situation changes.
- In third person, prompts sit just ahead of your character's head, a little to the right (the
  distance to the right is a setting). In first person they stay on your character.
- Prompts hide while a menu is open (inventory, map, dialogue and so on).

## Prompts

Prompts marked with a mod name need that mod; the rest work with Skyrim alone.

### Daily life

| Prompt | When |
|---|---|
| Bathe / Shower | Naked in water, or under a waterfall. *Bathing in Skyrim - Renewed* |
| Undress / Get Dressed | At a bed or wardrobe, and in water. What you take off is remembered and put back on |
| Eat: <food> | When you are hungry. Cheapest food first; raw meat, drinks and spoiled food are skipped. *Survival Mode* |
| Urinate / Defecate | When bladder or bowels are full; not while you are bathing. *Private Needs - Orgasm* |
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
| Swap Outfits (hold) | Looking at a mannequin: what you wear and what it wears change places. Enchanted and tempered pieces move as they are, and the mannequin's helmet comes to you stowed. With one side empty the prompt is Store Outfit or Wear Mannequin's Outfit |

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
| Wizard Warrior Mode / End Wizard Warrior | Drawing a weapon while it is off; sheathing out of combat while it is on. *The Wizard Warrior* |

## Other mods

CIGAR **uses these mods when they are installed and quietly skips the prompt when they are not.**
It includes none of their files.

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
| Wizard Warrior Mode, End Wizard Warrior | The Wizard Warrior - Spellsword Magic Combat Evolved | [Nexus](https://www.nexusmods.com/skyrimspecialedition/mods/14890) |

Warm Hands needs the Survival Mode Creation Club file.

### Settings CIGAR changes in other mods

So that one key does not do two things, CIGAR can take a mod's own hotkey over. This is the
**prompt only** switch on the Keys page, **on by default**, and switching it off gives the key back.

| Mod | While prompt only is on |
|---|---|
| Grapple | its hotkey moves to F13 |
| Acheron | the surrender key moves to F14 |
| Valhalla Combat | the execution key moves to F15 |
| Fill Her Up Baka Edition | the deflate hotkey is cleared |
| Private Needs - Orgasm | its six hotkeys are cleared, including its menu and status keys |

With True Directional Movement installed, Grapple's target lock key is set to TDM's lock key.

If you change one of these keys in the other mod's menu, press **Check mod keys again** on the Keys
page once.

## Settings (SKSE Menu Framework)

The in-game menu has a **CIGAR** section with five pages.

1. **Combat**, 2. **Non-combat** and 3. **Mod integrations**: switch each feature on or off; a feature
   switched off takes its prompts away at once. Mod integrations work only with their mod installed
   and wait quietly without it.
4. **Keys**: the four prompt keys (keyboard, or the mouse's middle and side buttons), the **gamepad
   buttons**, and the prompt only switches above. A clash with another mod's key is shown. The gamepad buttons are SkyPrompt's own (A, B, X, Y by default, or what you set
   in SkyPrompt's Controls) or the **D-pad**: 1 Up, 2 Down, 3 Left, 4 Right. While a prompt shows, its
   D-pad direction runs the prompt instead of Favorites or a hotkey; with no prompt the D-pad works as
   usual.
5. **Options**: whether SkyPrompt is connected, the **language** (Auto, your game's language, 한국어
   or English), the **prompt position** (how far to the right prompts sit in third person: 0, 10, 15,
   20 or 30, default 15), when prompts start (potion thresholds, hunger stage, needs level, weapon swap
   distance, jujutsu reach, bed and wardrobe reach, pass time speed) and **jujutsu damage**: the share
   of the enemy's maximum stamina (default 100%) and maximum health (default 5%) a throw takes. A throw
   always leaves at least 1 health; only the neck break kills.

Your choices are saved to `SKSE/Plugins/CIGAR.json`.

## Good to know

- **Chair drinking** knows the 29 drinks of the base game and its DLCs, plus any drink another mod
  tags as alcohol (Gourmet, Object Categorization Framework, SunHelm).
- **The helmet** comes off at once, without an animation.
- **Prompt position:** CIGAR keeps one invisible marker in each save to place prompts. It is
  harmless if CIGAR is removed.
- **Mannequins:** tested with the Unofficial Patch and Another Mannequin Script Fix. If a mannequin
  cannot take an outfit, nothing moves and a notification says why.
- **Scenes:** while a SexLab or OStim scene plays the player, combat, gear and needs prompts stay
  away (detected at runtime; neither framework is needed).
- Page names in the settings panel switch language at the next launch; everything else switches at
  once.

## Known issues

- **Jujutsu does nothing until someone has died since the game was loaded.** After the first death it
  works normally.
- **Execute can miss the first press.** Press again.
- **A weapon enchantment stays after End Wizard Warrior** until it runs out. That is The Wizard
  Warrior's own design, and CIGAR leaves it alone.

## If something goes wrong

Every launch writes `Documents/My Games/Skyrim Special Edition/SKSE/CIGAR.log`. It says why a prompt
did not show or an action did not happen, and which of the mods above were found, so please attach
it when you report a problem.

## License

CIGAR uses CommonLibSSE-NG, so it is **GPL-3.0-or-later**. The source is at
<https://github.com/kkw1010-dev/HKT>.
