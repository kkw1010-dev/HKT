# 006 · Deflate: Fill Her Up's deflation key as a hold prompt

Status (2026-09-17): built and deployed. Not yet tested in game.

## What the original key does

Source: `Fill Her Up Baka Edition` 2.11 beta, `Scripts/Source/sr_infDeflateAbility.psc`.
The script is on alias 1 (`Player`) of `sr_inflateQuest`
(`000D63:sr_FillHerUp.esp`).

- `OnKeyDown(defKey)` enforces a 5 s real-time cooldown, then calls
  `SpermOutStart()`. That sets `keydown`, waits 0.1 s, and checks stamina
  (>= 30%), plugs, gag and burst. Each 0.3 s tick of the `doPush(type)` loop
  runs **while `keydown`** (or the expel effect) is active, stamina stays above
  2% and the pool holds more than 0.02.
- `OnKeyUp(defKey, time)` clears `keydown`, which ends the loop.

So a tap does almost nothing, and holding the key empties the most recent pool
until the key is released or stamina runs out. `defKey` lives on
`sr_inflateConfig` (MCM; the default is 81).

## CIGAR

SkyPrompt 2.3.15 sends `kDown` (5) and `kUp` (6) on the prompt key's press and
release, whatever the prompt type (`InputHook::ProcessInput`). `PromptSlot` has
a hold mode that:

- forwards down and up to `Module::OnHold`;
- treats `kRemovedByMod`, `kTimeout` and `kDeclined` as up, so a hold always
  ends;
- ignores `kAccepted`, so the prompt stays on screen.

`Deflate::OnHold` dispatches FHU's own `OnKeyDown(defKey)` and
`OnKeyUp(defKey, heldSeconds)` on the player alias. The cooldown, stamina and
plug checks, messages and animations are therefore all FHU's.

The prompt 배출 (길게 누르기) is shown only when:

- the player is in `inflateFaction` or `SR_InflateOralFaction` (quick filter);
- FHU's `GetMostRecentInflationType(player)` > 0, the same test the original
  key uses (async, refreshed each tick while in a faction; the oral faction
  can stay at rank 0 after emptying);
- the player is in neither `inflaterAnimatingFaction` nor
  `slAnimatingFaction`.

While the key is held, the prompt stays up even though FHU puts the player in
its animating faction, so the release still reaches CIGAR.

## Self-reporting

- On load: `FHU quest=... alias=... ability=... key=...`. When FHU is
  installed but its scripts do not resolve, a HUD message appears once.
- Changes to tracked, type, animating, sexlab and holding are logged as
  `gate` lines. Each forwarded key event is logged, and so is its return.
- `tools/verify_deploy.py` checks that the FHU scripts still contain the names
  read above.
