# 006 · Deflate: Fill Her Up's deflation key as a hold prompt

Status (2026-09-17): tested in game. Report 1 below was a problem in FHU
itself, and the user fixed it in FHU on 2026-09-18 (outside this repo).

- **Results.** The prompt showed only while FHU tracked an amount. A hold ran
  FHU's push loop.
- **Report 1: "too fast, no sound, no strip or puddle".** The Papyrus log shows
  FHU's own path ran in full:
  - `doPush`;
  - `StartLeakage animate:1`;
  - `FHUmoanSoundEffect Vaginal 1`;
  - SexLab cum FX.

  The pools held only 0.3 and 1.0. FHU drains `0.05 / animMult` every 0.3 s,
  so they emptied in about 2 s and 8 s; the speed is FHU's MCM slider
  `$FHU_ANIM_MULT` (1.0 here). Both presses came the moment a SexLab scene
  ended, while SexLab was still re-dressing the player and resetting the
  face. The prompt now waits until FHU's and SexLab's animating factions have
  been clear for 3 s.
- **Report 2: the prompt faded after a while and never came back.** SkyPrompt
  ends a prompt after its lifetime (`kTimeout`). `PromptSlot` now re-sends an
  offered prompt every 2 s, which resets that lifetime, and offers it again
  after a timeout.

FHU's own key (`defKey`) is unbound (-1) on this modlist; CIGAR passes the
same value, so the call still matches.

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
- neither `inflaterAnimatingFaction` nor `slAnimatingFaction` has held the
  player in the last 3 s.

While the key is held, the prompt stays up even though FHU puts the player in
its animating faction, so the release still reaches CIGAR.

## Self-reporting

- On load: `FHU quest=... alias=... ability=... key=...`. When FHU is
  installed but its scripts do not resolve, a HUD message appears once.
- Changes to tracked, type, animating, sexlab, settling and holding are logged as
  `gate` lines. Each forwarded key event is logged, and so is its return.
- `tools/verify_deploy.py` checks that the FHU scripts still contain the names
  read above.

## Prompt-only (2026-09-25)

At the user's request FHU joins the prompt-only list (panel: "Fill Her Up 배출: 프롬프트 전용",
default on). `Deflate::ApplyKeyMode` (at load and from the panel's key check button) sets
`sr_inflateConfig.defKey` to -1 and remembers the old key in `CIGAR.json`
(`promptOnly.fillherup.manualKey`, seeded with 82, the key MCM Memory had stored). This works
because `sr_infDeflateAbility.OnKeyDown/OnKeyUp` only compare the code they receive with `defKey`,
and CIGAR sends `defKey`, so -1 still matches; a physical key no longer does, and the ability
registers no key while `defKey` is below 0. Switching prompt-only off writes the remembered key back
and asks the ability to `RegisterForKey` it. MCM Memory's FHU row is -1 as well (see `docs/000`,
the hotkey rule).

Passed in game on 2026-09-25 (the user; no log sent): R does nothing, the hold prompt deflates, and
with prompt-only off R works again. That third step also made MCM Memory record R (82) in its
profile again at 09:36; `verify_deploy.py` failed the next build on it, and the row was set back to
-1 (backup `Default.json.bak_20260925_fhu-key-again`). Switching prompt-only off in a test is
therefore followed by that check failing until the row is reset.
