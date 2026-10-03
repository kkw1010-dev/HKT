# 046 · Voice answer: equip Unrelenting Force against a crowd (design, not built)

Status (2026-10-03): design for review. The user dropped Cleave (docs/045 "Decision") and chose to improve
many-against-one by offering vanilla answers, starting with: "when it is two or more against one, a prompt
that equips Fus Ro Dah". Equip only; the player shouts with their own key (the user's decision, which is an
exception to CIGAR's "a prompt performs the whole action" rule).

## The shout

`MAG_UnrelentingForceShout` 00013E07 (Skyrim.esm; in this load order won by Stormcrown.esp, which keeps the
words Fus / Ro / Dah and sets recovery to 10 / 20 / 30 s). The prompt names it from the form, so the
game's own name shows (Korean: 거침없는 힘). A mod that replaces the shout with another form is not followed.

## Gate (every second, Tick)

- Player in combat, controls free, not in a scene, mounted, swimming or a kill move.
- **Two or more hostile actors** that are in combat, alive, not bleeding out, within **1,000 units**
  (about 14 m; my starting value) and **in the player's line of sight** (`HasLineOfSight`). Held 1 s, so it
  does not flicker as enemies move.
- The player knows the shout (`HasShout`; the game adds it when its first word is unlocked).
- The voice slot does not already hold it (`selectedPower`).
- The voice is ready (`GetVoiceRecoveryTime() == 0`): recovery is shared by all shouts, so equipping it
  while the voice recovers gives nothing to do.

## Action

Single press (a combat prompt): `ActorEquipManager::EquipShout(player, shout)`, the call QuestAction already
uses (seen working in game for the Greybeards' trial). The log line names what the slot held before and
after; a slot that does not change gives a WARN.

## Putting the old shout back

Proposed: when combat ends, put back what the slot held before (a shout or a power), but only if the slot
still holds Unrelenting Force (the player did not change it in the meantime). Reason: the press was meant for
this fight; a racial power or a favourite shout the player had chosen should not be lost silently. One log
line either way. (Alternative: leave it equipped; simpler, but the player has to go to the menu.)

## Decline

Double tap hides it for this fight (situation = the combat; it can come back in the next one).

## Growing it

One module, "Voice answers", with a small table of {shout form, gate}: Unrelenting Force first; later
candidates such as Disarm (a single strong armed enemy) or Slow Time, each a vanilla shout with its own
situation, one prompt at a time in the crowd slot. Curated entries, not a user-made rule list.

## War stomp and the key slots

The war stomp (if kept after r17c) is an action for "surrounded, within 200"; this is a preparation for
"two or more within 1,000 in sight". When both hold, the stomp shows (it acts now) and the equip prompt
waits. Combat already has up to Execute, Throw, Lock On, Grapple, Potion, Poison competing for four keys.

## Risks

- Other mods that manage the voice slot (equip-set or hotkey mods) can change it back; the restore rule
  only acts on its own equip.
- `IsInCombat` flaps at the edges of a fight; the 1 s hold and the per-combat decline cover most of it.
- Shout mods that add their own Unrelenting Force form are not detected.
