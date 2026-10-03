# 046 · Voice answer: equip Unrelenting Force against a crowd (design, not built)

Status (2026-10-03): **built** as the module `VoiceAnswer` (panel: 용언 장착 / Shout Ready, combat page),
deployed in the author build, not yet seen in game. Decisions: D40 (the user): nothing is put back after
the fight, because the module is meant to offer other shouts as other situations come. The rest of this
page is the design it was built from. The user dropped Cleave (docs/045 "Decision") and chose to improve
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

Not done (the user, D40: "그대로 둬도 되지 않나?, 어차피 다양한 용언들이 뜨게 만드니깐"). The shout stays equipped.

## Decline

Double tap hides it for this fight (situation = the combat; it can come back in the next one).

## Growing it

`VoiceAnswer` holds a table of {shout, situation}; the first entry whose situation holds and which the player
knows and has not equipped is offered, one prompt at a time. Candidates (not built; situations are my
proposals, each to be decided by the user):

| Shout (Skyrim.esm) | Situation | Why it answers it |
|---|---|---|
| Unrelenting Force 00013E07 | two or more enemies in sight within 1,000 | built: pushes a crowd back |
| Disarm (진압) 00070981 | one enemy close, holding a weapon, at a higher level than the player | takes the weapon of the one dangerous foe |
| Slow Time (시간 왜곡) 00048AC9 | three or more enemies in sight, or an enemy archer and a melee enemy at once | buys time to act against many |
| Become Ethereal (에테르화) 00032920 | health low while two or more enemies are close | the vanilla "get out of this" answer |
| Ice Form (얼음 형태) 00070980 | one enemy charging in from more than 500 away | stops a single runner |

FormIDs checked against the load order on 2026-10-03 (Skyrim.esm shouts, all won by Stormcrown.esp in this game).

## Key slots

The war stomp and Cleave were dropped (2026-10-03), so this is the only crowd prompt. Combat already has up
to Execute, Throw, Lock On, Grapple, Potion and Poison competing for four keys.

## Risks

- Other mods that manage the voice slot (equip-set or hotkey mods) can change it back; the restore rule
  only acts on its own equip.
- `IsInCombat` flaps at the edges of a fight; the 1 s hold and the per-combat decline cover most of it.
- Shout mods that add their own Unrelenting Force form are not detected.
