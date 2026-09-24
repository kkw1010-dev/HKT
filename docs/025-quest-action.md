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
