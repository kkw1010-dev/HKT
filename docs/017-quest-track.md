# 017 · Quest tracking

Status: hold and tracking confirmed in game on 2026-09-21; label fallback fix built, awaiting runtime confirmation.

## Contract

Streamlined Interactions shows a Track Quest prompt when a new objective arrives and tracks that
quest when the prompt is accepted. CIGAR replaces only `QuestActions.enabled_track`; the rest of
SI's `QuestActions` module stays enabled.

SI uses a hold interaction for Quest Tracking. CIGAR follows that behavior and keeps the prompt
available for 15 seconds. A newer untracked quest replaces the current offer.

## Implementation

- `RE::ObjectiveState::Event` is the gate. A transition to `kDisplayed` identifies the quest through
  `BGSQuestObjective::ownerQuest`; this is narrower than treating every quest-stage event as a new
  objective.
- The event sink copies only the quest FormID and objective index, then marshals work to the game
  thread. Event bursts are coalesced into one queued task and the latest objective wins.
- Already tracked, disabled, or completed quests are ignored. The same facts are checked again when
  the prompt is accepted.
- The prompt uses the quest's journal name when the QUST record has one. Nameless miscellaneous
  quests use the newly displayed objective text instead; a FormID is never shown to the player.
- The prompt uses SkyPrompt's `kHold` type. Non-combat contextual actions default to a hold so an
  incidental tap cannot change player state; single press is reserved for timing-sensitive combat
  actions or an explicitly documented exception.
- Acceptance dispatches the native Papyrus method `Quest.SetActive(true)`. Its callback returns to
  the game thread and verifies `TESQuest::IsActive()`; a failed result produces a HUD warning.
- No save state is needed. The offer is transient and is cleared on every load or new game.

## Test

1. Start with SI's Quest Tracking enabled, deploy the author build, and verify the deploy tool turns
   only `QuestActions.enabled_track` off.
2. Receive a new objective for a quest that is not currently tracked. Expect
   `추적하기 (길게): <퀘스트 이름>` within one second. For a nameless miscellaneous
   quest, expect its new objective text instead of a hexadecimal FormID.
3. Accept it within 15 seconds. The quest marker should become active; `CIGAR.log` should show
   `SetActive returned ... active=true`.
4. Receive a new objective for an already tracked quest. No CIGAR prompt should appear.
5. Let the prompt sit for 15 seconds. It should disappear without changing quest tracking.
