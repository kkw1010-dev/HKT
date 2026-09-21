# 017 · Quest tracking

Status: built on 2026-09-21; not yet tested in game.

## Contract

Streamlined Interactions shows a Track Quest prompt when a new objective arrives and tracks that
quest when the prompt is accepted. CIGAR replaces only `QuestActions.enabled_track`; the rest of
SI's `QuestActions` module stays enabled.

The exact SI prompt input type and lifetime are not documented. CIGAR uses a single press and a
15-second offer window. A newer untracked quest replaces the current offer.

## Implementation

- `RE::ObjectiveState::Event` is the gate. A transition to `kDisplayed` identifies the quest through
  `BGSQuestObjective::ownerQuest`; this is narrower than treating every quest-stage event as a new
  objective.
- The event sink copies only the quest FormID, then marshals work to the game thread. Event bursts
  are coalesced into one queued task and the latest quest wins.
- Already tracked, disabled, or completed quests are ignored. The same facts are checked again when
  the prompt is accepted.
- Acceptance dispatches the native Papyrus method `Quest.SetActive(true)`. Its callback returns to
  the game thread and verifies `TESQuest::IsActive()`; a failed result produces a HUD warning.
- No save state is needed. The offer is transient and is cleared on every load or new game.

## Test

1. Start with SI's Quest Tracking enabled, deploy the author build, and verify the deploy tool turns
   only `QuestActions.enabled_track` off.
2. Receive a new objective for a quest that is not currently tracked. Expect
   `추적하기: <퀘스트 이름>` within one second.
3. Accept it within 15 seconds. The quest marker should become active; `CIGAR.log` should show
   `SetActive returned ... active=true`.
4. Receive a new objective for an already tracked quest. No CIGAR prompt should appear.
5. Let the prompt sit for 15 seconds. It should disappear without changing quest tracking.
