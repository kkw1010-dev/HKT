# 037 · The Wizard Warrior: 마검사 모드

Status (2026-09-27): **built, deployed and installed; in-game NOT VERIFIED.**
- The module (`src/WizardWarrior.*`) is in the deployed author build.
- The user approved the WW install in the CIGAR session. It is Installation and Modification case
  037: `##Magic`, plugin index 0x89, and verify_deploy passes WW's script-name check.
- The in-game test is `TEST-next-ingame.md` N4.

A Nexus user (xLenax) asked for a combat prompt that activates The Wizard Warrior. The user took
it on for these reasons:
- 3,299 endorsements and 86,737 unique downloads
- stable, since the mod has not changed since 2022
- CIGAR only needs to trigger its toggle
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

## The design (the user, 2026-09-27)

- **When.** Drawing a weapon while WW is off offers **마검사 모드**. Whether a fight is on does not
  matter; the player can try it by swinging at the air. One press calls
  `QK_MainQuestScript.ToggleAbility()`.
- **Hidden:**
  - while WW is on (`QK_SpellToggle` = 1)
  - while `Allow_Switch` is false (a concentration spell through block)
  - in a SexLab or OStim scene
  - after a decline (double tap), until the weapon is sheathed: the same logic as the other
    declines
- **마검사 해제 (changed by the user, 2026-09-27, after N4 passed).** Sheathing out of combat
  while WW is on offers to turn it off. It is shown while that holds (with `Allow_Switch` true and
  no scene), and a decline hides it until the next draw and sheathe. The user's reason: in play,
  WW could not be turned off by hand and stayed on, or only wore off with time, which was very
  distracting. So an off prompt does not break the principle.
  - What the logs and the source show about "cannot turn it off":
    - X does run the off path. In the N4 run, `QK_SpellToggle` went 1 -> 0 at 15:26:53 and
      15:27:58 with the weapon sheathed, where CIGAR only turns WW on.
    - `ToggleAbility()`'s off path stops the current group's glow, dispels `TWW_Power` and sets
      the global to 0.
    - It does **not** call `Enchantment_On(False)`, so a weapon enchantment spell (when enabled in
      WW's MCM) stays until its own duration ends. WW's 4.2.6 changelog says this is deliberate.
    - The only refusal is `Allow_Switch` false, during a concentration spell; the prompt is
      hidden then.
    - CIGAR therefore uses `ToggleAbility()` both ways and does not change WW's enchantment
      behaviour.
- **Keys and MCM are the player's.** CIGAR does not move, unbind or report WW's keys, and does not
  touch MCM Memory for it. No prompt-only mode. The user rejected the earlier options A, B and C
  for the 1-4 clash: "나는 모더지 출장 수리기사가 아니다" (I am a modder, not a house-call
  repairman). If WW's group keys 1-4 clash with the prompt keys while WW is on, the player
  rebinds one side.
- **Soft integration.** Without `The Wizard Warrior.esp`, the module logs once and idles.

## As written (`src/WizardWarrior.cpp`)

- **At load** it looks up `QK_QuestMain` (0x878), the global `QK_SpellToggle` (0x87E) and the bound
  `QK_MainQuestScript`. It logs `ready: ...`, or why the module is off.
- **Gate (100 ms).** Weapon drawn, not on, `Allow_Switch`, no scene, not dismissed, and not within
  2 s of an accept. The gate line logs each of these.
- **Accept.** `DispatchMethodCall2(QK_QuestMain, "QK_MainQuestScript", "ToggleAbility")`. Two
  seconds later it checks `QK_SpellToggle`: it logs `on: QK_SpellToggle = 1`, or a WARN plus one
  notification when WW did not turn on (self-reporting).
- **Prompts.** 마검사 모드 is ID 41 and 마검사 해제 is ID 42, both single press. The panel label is
  마검사 모드 / Wizard Warrior Mode.
- **Off check.** Two seconds after 마검사 해제 it checks that `QK_SpellToggle` is 0: `off: ...`,
  or a WARN plus one notification.
- **verify_deploy.** When the `The Wizard Warrior` mod is enabled, it checks that
  `QK_MainQuestScript.pex` still has `ToggleAbility`, `Allow_Switch`, `PowerToggle` and
  `KeyPowerUP`. Otherwise it only notes that the module idles.

## The install (done 2026-09-27, IaM case 037)

The placement follows the Installation and Modification rules:
- mod folder `The Wizard Warrior` inside the `##Magic` separator, next to Magic Sneak Attacks and
  Enhanced Reanimation (magic mechanics, no shared files)
- plugin `The Wizard Warrior.esp` (a full ESP, master Skyrim.esm) in the `Overhaul - Magic` group,
  right after `aap-Clairvoyance Corpses.esp` in `plugins.txt` and `loadorder.txt`
- no ESL compaction, so the local IDs (0x878, 0x87E) stay those every Nexus player has
- its requirements (SKSE, SkyUI, powerofthree's Papyrus Extender, PapyrusUtil) are installed
