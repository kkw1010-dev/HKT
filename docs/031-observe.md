# 031 · Observe (주시하기)

Status (2026-09-25): passed in game (test run 3, actor and scenery; the double-tap decline passed
the same day). First built as 살펴보기 and renamed by the user: 살펴보기 read like turning a 3D model
around.

## As built

- Defaults: stand still 5 s, target within 5000 units, zoom by up to 40 degrees.
- Gate (100 ms): weapon sheathed, no movement input and out of combat for 5 s, looking and movement
  controls on, no zoom still easing out, and something to watch. Since 2026-09-25 (the user: it is for
  observation and scouting) that is a named actor under the crosshair within 5000 units, or **scenery**:
  nothing under the crosshair while not looking at the floor (pitch below Rest's 0.6 rad, so 앉기 and
  주시하기 never share a moment). Objects and furniture under the crosshair are not watched. Scenery
  reads `주시하기 (누르고 있기)` with no name.
- Hold-mode prompt (like 시간 보내기): while held, `PlayerCamera` `worldFOV` (or `firstPersonFOV` in
  first person) glides to base - 40 (at least 15) over 2 s on a smootherstep curve. Release, movement or
  combat glides it back over 0.7 s. A small thread posts one task a frame while an ease runs (the
  user asked for a smoother zoom than a step on the 100 ms tick).
- Declined (a double tap; the user, 2026-09-25): hidden until the player is 300 units away from where
  it was declined. SkyPrompt's hold-and-keep type sends no decline event (test: the double tap only
  zoomed), so CIGAR reads it: two presses shorter than 250 ms within 600 ms. The prompt stays up during
  that window so the second tap lands.
