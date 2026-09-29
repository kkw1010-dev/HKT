# 041 · Release notifications and bathing prompt dismissal

Status (2026-09-29): planned, not implemented. Author decision following Nexus comment
`176411097`; the public reply is `176415558`. This note is for the next code change, not a
release announcement.

## Diagnostic HUD notifications

The player reported top-right messages when a prompt is blocked. The author agreed that
diagnostic messages should not have reached the release HUD. Keep the diagnostic detail in
`Documents\My Games\Skyrim Special Edition\SKSE\CIGAR.log` instead of adding a player-facing
debug-message setting.

- Audit the `Util::Notify` call sites (it calls `ShowHUDMessage` in `src/Util.cpp`) and identify the
  exact trigger of the reported blocked-prompt message. `src/Bathe.cpp` currently notifies when
  Bathing in Skyrim is disabled; other modules have similar passive integration/gate notices.
- Remove passive gate, compatibility, and debug-status messages from the release HUD while retaining
  useful log entries. Do not silence `Util::Notify` globally: a result of an action the player just
  requested, or a critical failure requiring player action, needs a separate UX decision.
- Preserve enough context in the log to explain why a prompt did not appear. Check that ordinary
  blocked conditions do not put notifications in the upper-right corner during normal play.

## Bathe and Shower double-tap decline

SkyPrompt's `kDeclined` is already passed to `Module::OnDeclined` by `src/Prompt.cpp`.
`Bathe` has no override today. `PromptSlot::Update` continues to keep an eligible prompt alive,
so a double-tap decline can be followed by the prompt returning under the same conditions.

- Add `Bathe::OnDeclined` for `kBathe` and `kShower`, with independent dismissal state for each
  prompt. Withdraw the declined prompt and stop offering it while its underlying eligibility
  remains true.
- Rearm a prompt after its own eligibility becomes false, then true again. For Bathe, eligibility
  is in water, not busy, undressed, and Bathing in Skyrim enabled. For Shower, it additionally
  requires a real waterfall and the relevant water restriction. A continuing wash animation is
  not by itself a new offer episode. Reset dismissal state on game load.
- Preserve the existing acceptance path and the displayed dirtiness percentage. Do not add a
  dirtiness threshold: bathing may be a roleplay choice even when the character is clean.
- Log decline and rearm transitions in `CIGAR.log` so a missing or reappearing prompt can be
  diagnosed without HUD spam.

## Verification when implemented

1. Decline Bathe by double-tapping; it stays hidden while the player remains eligible. Leave the
   water and return; it can appear again.
2. Under a waterfall, decline Shower; it stays hidden while the Shower conditions remain true.
   Leave the waterfall and return; it can appear again. Declining one prompt does not dismiss the
   other.
3. Confirm accepted baths and showers still call Bathing in Skyrim, and a wash finishing does not
   immediately re-offer a prompt.
4. Confirm ordinary blocked-prompt conditions write to the log without a diagnostic HUD message.
