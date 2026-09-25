# 025 · Quest action (equip the shout the Greybeards ask for)

Status (2026-09-24): built as module `QuestAction` and deployed; not yet tested in game. SI's
`QuestActions.enabled` is now in the replaced list (its tracking half was already `QuestTrack`,
`docs/017`).

## What SI does (settings, MCM text and DLL strings only)

- MCM: "Prompts to perform actions related to quests and/or start tracking quests." Under it, the
  one quest action SI lists: "Greybeards: Dragonborn, show us your Thu'um." with
  "Quest Action Prompt: Equip Unrelenting Force".
- DLL: `QuestActions::Act`, `QuestActionsHelper::ResolveAlias` / `GetItemName`,
  `Equipper::ReadBook` ("Book not found"), "Shout not found", sinks for `ObjectiveState`,
  `TESQuestStageEvent` and `TESQuestStartStopEvent`. No quest editor ID or alias name appears as a
  string, so SI's quest table (if it has more rows) is not readable without disassembly, which its
  licence forbids.
- `SI/_ABSORPTION/_MAP.md` 4c records an official video showing a party-clothes swap and a
  note-reading prompt as well; whether the installed 1.0.6 still has them is unknown.

## As built

- `src/QuestAction.cpp`, once a second: while MQ105 The Way of the Voice (`0242BA:Skyrim.esm`) runs
  with objective 20 or 40 displayed ("Demonstrate your Unrelenting Force Shout"), and the player
  knows Unrelenting Force (`013E07`) but does not have it in the voice slot, 장착하기 (길게):
  <샤우트> shows (out of combat, movement controls on). Accepting calls
  `ActorEquipManager::EquipShout` and checks the voice slot at once.
- Objective 60 ("Demonstrate your Whirlwind Sprint Shout", `02F7BA`) is handled the same way. SI
  lists only Unrelenting Force; this is CIGAR's extension of the same action.
- Shout names come from the records (Stormcrown's Korean names on this order).

## Open

- In-game at High Hrothgar. If SI had further quest actions (party clothes, notes), switching
  `QuestActions.enabled` off removed them; none is known to exist in 1.0.6.

## Console setup (2026-09-25)

For a test save (it pushes the main quest forward). Stage→objective map read from
`QF_MQ105_000242BA` (USSEP's copy) and the quest's fragment table: objective 20 is shown at stage
30, 40 at stage 80, 60 at stage 120. `startquest MQ105` runs stage 0 → 1, a skip-ahead stage that
completes MQ103/MQ104 and sets 5 and 10.

```
startquest MQ105
player.addshout 00013E07
player.teachword 00013E22
player.unlockword 00013E22
setobjectivedisplayed MQ105 20 1
```

Expected gate line: `mq105=10 objective=20 shout=거침없는 힘 known=true equipped=-`. For objective 60
the same with `0002F7BA` / word `0002F7BB` and `setobjectivedisplayed MQ105 60 1`. If
`player.addshout` is not accepted by the console, `player.psb` grants every shout. Afterwards
`setobjectivecompleted MQ105 20 1` must take the prompt away.

Result (2026-09-25): not reached. After the console lines the gate still read `mq105=0 objective=0`,
so MQ105 never started (the user also could not absorb a dragon soul to reach it the normal way).
Deferred by the user; the module loads (`ready: MQ105=true unrelentingForce=거침없는 힘
whirlwindSprint=회오리의 질주`) but its prompt has never been offered in game.
