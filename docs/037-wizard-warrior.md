# 037 · The Wizard Warrior: research only

Status (2026-09-27): **research done, nothing built.** A Nexus user (xLenax) asked for a combat
prompt that activates The Wizard Warrior. The user is weighing whether it is worth doing:
- 3,299 endorsements and 86,737 unique downloads
- stable, since the mod has not changed since 2022
- low complexity if CIGAR only takes over the key
- listed on that mod's page as a mod using it, CIGAR would pick up visitors

## The mod

The Wizard Warrior - Spellsword Magic Combat Evolved (Nexus 14890), by dhxxqk2010, 5.0.1 of
2022-09-12. It binds spells to combat actions: attack, power attack by direction, block, bash,
bow. Four spell groups, and instant-spell keys. It is not installed in this load order.

## Confirmed from the files (5.0.1 archive, read in a temporary folder, not installed)

The user downloaded `The Wizard Warrior-14890-5-0-1-1662986657.7z`. It was unpacked to the
scratchpad and read only. The sources ship with the mod (`Source/Scripts/*.psc`); the ESP was
parsed for record IDs and property values.

- **No DLL.** The mod is Papyrus only: `The Wizard Warrior.esp` (master `Skyrim.esm`) plus
  `QK_*` scripts.
- **Forms** (local IDs in `The Wizard Warrior.esp`):

  | Form | ID | What it is |
  |---|---|---|
  | Quest `QK_QuestMain` | 0x878 | `QK_MainQuestScript`; all the state and functions |
  | Quest `QK_MCM` | 0x885 | `QK_MCM_Script` (SkyUI MCM) |
  | Global `QK_SpellToggle` | 0x87E | the script's `PowerToggle` property: **1 while on, 0 while off** |
  | Spell `QK_SpellCastingPower` | 0x87C | the script's `TWW_Power`; its effect `QK_SpellCastingEffect` (0x87D) runs `QK_SpellCastingScript` |
  | Spell `QK_TogglePower` | 0x8A8 | a power whose effect (`QK_Toggle` 0x8A9, `QK_TogglePower` script) calls `ToggleAbility()` |
  | Spells `QK_Group1Power`..`QK_Group4Power` | 0x8AE-0x8B1 | powers that switch the spell group (`QK_SwitchGroup`) |

- **Default keys** (`QK_MainQuestScript` properties in the ESP):
  - `KeyPowerUP` = 45 (**X**)
  - `GroupKey_0..3` = 2, 3, 4, 5 (**keys 1-4**)
  - `KeyHold` = 56 (**Left Alt**)
- **How the toggle key works.** `OnInit` calls `RegisterForKey(KeyPowerUP)`. The quest's `OnKeyDown`
  does not look at the key code: when `Allow_Switch` is true it calls `ToggleAbility()`; otherwise
  it re-casts a concentration spell. `ToggleAbility()`:
  - turning on: plays a sound, sets the glow, casts `TWW_Power` on the player, sets
    `PowerToggle` to 1 and `TWW_Ability` to true
  - turning off: the reverse (dispels `TWW_Power`, `PowerToggle` 0)
- **How the group keys work.** They are *not* registered by the quest.
  `QK_SpellCastingScript.OnEffectStart` (the effect of `TWW_Power`) calls `RegisterForKey` for
  `GroupKey_0..3` and reads `KeyHold`. So **the group keys are live only while The Wizard Warrior
  is on**; dispelling the power ends the registrations. Its `OnKeyDown`:
  - without Left Alt: `InstantSpell_SwitchGounp(key)` (the active group's key casts Instant
    Spell 0, another group's key switches group)
  - with Left Alt held: `InstantSpellPlus(key)`
- **Changing keys in the MCM.**
  - The toggle's keymap option calls `REG_KeyToggle(newKey)`, which unregisters the old key and
    registers the new one.
  - The group and hold options only set the properties; they take effect the next time
    `TWW_Power` starts.
  - A "release hotkeys" button sets every key to -1, but **does not unregister the toggle key**.
    Because `OnKeyDown` ignores the key code, X keeps toggling after a release (a bug in WW).
  - With PapyrusUtil, the MCM can save and restore the key list (`StorageUtil` "KeyMap").
- **`Allow_Switch`** is false while a concentration spell is being cast through block; X then
  re-casts it instead of toggling.

## How CIGAR would drive it (confirmed entry points)

1. **Resolve at load.** `The Wizard Warrior.esp` present, quest 0x878 found, `QK_MainQuestScript`
   bound. Absent means the module is silently off (`cigar-dll-soft-integrations`).
2. **Read the state** from the global `QK_SpellToggle` (0x87E), which is 1 while on. Also read
   the script variable `Allow_Switch`, and offer nothing while it is false.
3. **Toggle** by calling `QK_MainQuestScript.ToggleAbility()` through the Papyrus VM, as CIGAR
   calls PNO's `UrinateAndDefecate`. This is the same function X and the toggle power run.
4. **Prompt-only mode:**
   - Call `REG_KeyToggle(<hidden key>)`, which properly unregisters X; setting the property
     alone does not. Remember X in `CIGAR.json` for switching back.
   - Update the MCM Memory profile too (`hotkey-changes-also-update-mcm-memory`).
   - Check the key again at load and from the panel's key check (`prefer-on-demand-checks-over-polling`).

## The 1-4 key conflict

WW's group keys are scan codes 2-5, keys 1-4. SkyPrompt's prompt keys are the same keys
(`keys: [2,3,4,5]` in SkyPrompt's `settings.json`, CIGAR's defaults too). The conflict exists
**only while WW is on**, which is when CIGAR's combat prompts (Jujutsu, Execute, Surrender, Weapon
Swap, Lock On) come up. Pressing 1 then both answers the prompt and switches WW's group or casts
its instant spell.

Options:

- **A. Detect and tell (recommended).**
  - CIGAR takes over only X.
  - At load and from the panel's key check, it compares WW's `GroupKey_0..3` with the prompt keys.
  - On an overlap it notifies once and logs which keys clash; the player rebinds one side (WW's
    MCM, or CIGAR's prompt keys in the panel).
  - Least invasive: the group keys are not keys CIGAR replaces with a prompt, so moving them is
    outside CIGAR's mandate. It is also self-reporting (`make-failures-self-reporting-not-user-retested`).
- **B. Move WW's group keys** in prompt-only mode, for example to 5-8.
  - Automatic, but CIGAR would choose combat keys for another mod.
  - It would also have to update MCM Memory and handle WW's own key saving.
- **C. Unbind WW's group keys** in prompt-only mode.
  - Group switching stays available through WW's own powers (`QK_Group1Power`..`4`, on the
    favorites or shout key).
  - The "press the active group's key to cast Instant Spell 0" quick cast is lost.

## When to offer it (design)

- **Turning it on is the action, not a preparation.** It changes what every attack does for the
  rest of the fight. The accepted precedent is 록온 (Lock On), a combat-mode prompt offered when a
  fight starts. The rejected tool swap only put a pickaxe in hand; the player still had to act.
- **Proposal:**
  - Offer 마검사 모드 when a fight starts, with a weapon drawn, WW off and `Allow_Switch` true.
    One press turns it on (single press, the combat input policy).
  - Offer 마검사 해제 only after the fight ends, with WW on. In prompt-only mode X is gone, so
    this is the way to turn it off, besides WW's own toggle power.
  - A decline hides it until the next fight, or until the next time out of combat
    (`cigar-declined-prompt-stays-hidden`).
- **Not proposed:**
  - Turning it on automatically (`cigar-takes-animations-not-auto-triggers`).
  - Group switching as prompts; a group is the player's preference, not something the situation
    decides.

## Decisions needed before building

1. The 1-4 conflict: A, B or C (A recommended).
2. Whether 마검사 해제 is offered after a fight, as proposed, or WW simply stays on until the
   player uses WW's toggle power.
3. Prompt-only on by default, as for the other integrations.
4. Installing WW into the modlist to build and test (MO2 closed; the archive is in Downloads).

Estimated work once decided: about one session. It covers:
- the module and the prompt-only key handling
- the conflict check
- verify_deploy checks for `QK_QuestMain`, `QK_MainQuestScript`, `ToggleAbility`,
  `REG_KeyToggle`, `Allow_Switch` and `QK_SpellToggle`
- docs and an in-game test
