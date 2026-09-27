# 037 · The Wizard Warrior: research only

Status (2026-09-27): **research before a decision; nothing built.** A Nexus user (xLenax) asked for
a combat prompt that activates The Wizard Warrior. The user is weighing whether it is worth doing:
- 3,299 endorsements and 86,737 unique downloads
- stable, since the mod has not changed since 2022
- low complexity if CIGAR only takes over the key
- listed on that mod's page as a mod using it, CIGAR would pick up visitors

## The mod

The Wizard Warrior - Spellsword Magic Combat Evolved (Nexus 14890), by dhxxqk2010, 5.0.1 of
2022-09-12. It binds spells to combat actions: attack, power attack by direction, block, bash,
bow. Four spell groups, and instant-spell keys. It is not installed in this load order.

## What is known

**Confirmed** from Nexus's file-content preview of the 5.0.1 main archive (metadata only, nothing
downloaded):
- There is **no DLL**. It is Papyrus only, and ships its `.psc` sources:
  - `QK_MainQuestScript` (27.8 kB source)
  - `QK_MCM_Script` (89.4 kB)
  - `QK_TogglePower` (189 B)
  - `QK_ToggleImportPower`, `QK_SwitchGroup`, `QK_SpellCastingScript`, `QK_ImportSpellScript`,
    `QK_PowerAttackSpellScript`, `QK_GreatswordPowetAttackSpellScript`, `QK_MagicShoutScrpit`,
    `QK_EnchWeaponSpellScript`, `QK_SpellLearnCheckScript`
  - `The Wizard Warrior.esp` (30 kB)
- It requires SKSE, SkyUI and Papyrus Extender; PapyrusUtil is optional (setup storage).

**From the author's page and changelog** (the author's words, not checked in code):
- **X** turns The Wizard Warrior on and off; it "activates the main power/buff".
- Each spell group has a hotkey. Pressing the active group's key casts Instant Spell 0; the
  quick start says "Press 1 to try the default Fast Healing Instant Spell". Instant Spells 1-4 use
  the Nullify key with a group key.
- A **Nullify** key is held to suppress spells. There is also an Import power and a Capture Mode
  power.
- The changelog says:
  - 4.01: "toggle key cannot be changed (you have to reset quest script)"
  - 4.2.5: "unused group hotkey will no longer register its event"

**Inferred, to confirm in the `.psc`:**
- The keys are Papyrus `RegisterForKey` / `OnKeyDown` in a quest script, `QK_MainQuestScript` or
  `QK_MCM_Script`.
- `QK_TogglePower` (189 bytes) is a power's effect script that calls the same toggle, so the
  toggle may also be reachable through a spell.

## How CIGAR could drive it

These are the patterns CIGAR already uses for Grapple, Private Needs and BaboDialogue:

1. **Resolve at load.**
   - A soft integration: `The Wizard Warrior.esp` present, its quest found, the script bound.
   - Absent means the module is silently off (`cigar-dll-soft-integrations`).
2. **Read whether it is on.**
   - A script property or variable of the main quest, or the main buff's magic effect on the
     player.
   - The `.psc` decides which.
3. **Toggle it.**
   - Either call the quest script's toggle function through the Papyrus VM, as CIGAR calls PNO's
     `UrinateAndDefecate`,
   - or cast the toggle power's spell on the player.
4. **Prompt-only mode.**
   - Move WW's X key to a key no keyboard sends, as done for Grapple (F13).
   - Update the MCM Memory profile at the same time, or it restores X on every new game
     (`hotkey-changes-also-update-mcm-memory`).
   - The 4.01 note says a key change needs the quest script reset, so the key must be set through
     its own re-register path. Check this first.

**A key conflict to settle first.** WW's group keys default to 1-4 ("Press 1 ..."). SkyPrompt's
prompt keys are also 1-4: `keys: [2,3,4,5]` in SkyPrompt's `settings.json`, and those are the
scan codes of keys 1-4. SkyPrompt's Left/Right arrows (203/205) cycle between prompts. With WW
installed as shipped, pressing 1 both answers a CIGAR prompt and casts WW's instant spell.
Options:
- move WW's group keys in prompt-only mode
- leave them to the user

The group keys and instant spells are hotkeys too, so they could become prompts later. They are
out of any first version.

## When to offer it (design)

The worry is the rule "a prompt performs the whole action, never a preparatory step" (the
rejected tool swap).

- **Why it can fit.** Turning WW on is not "hold the tool and then act". It changes what every
  attack does for the rest of the fight: it *is* entering spellsword combat. The accepted
  precedent is 록온 (Lock On), a combat-mode prompt that is offered when a fight starts. The
  rejected tool swap only put a pickaxe in hand; the player still had to press E on the vein.
- **Proposal:**
  - Offer 마검사 모드 once a fight starts, with a weapon drawn and WW off; one press turns it on.
  - Offer 마검사 해제 only after the fight ends, never during it.
  - Both single-press prompts, following the combat input policy.
  - A decline (double tap) hides it until the next fight (`cigar-declined-prompt-stays-hidden`).
- **Not proposed:**
  - Turning it on automatically; the prompt decides, not a trigger
    (`cigar-takes-animations-not-auto-triggers`).
  - Group switching as prompts in a first version.

## Estimated work

- About one session, similar to the Grapple integration:
  - the module and prompt-only key handling
  - verify_deploy checks for the script and property names WW depends on
  - docs and a test
- **Before it starts:**
  1. The user approves installing WW; download is by the user or with their permission.
  2. Read `QK_MainQuestScript.psc` and `QK_MCM_Script.psc` to confirm the toggle function, the
     key registration and the "is on" state.
  3. Decide the 1-4 key conflict.
