# 039: SkyPrompt calls go out on the render thread

## The crash (2026-09-28 23:30:09, r6 D2)

The user switched CIGAR's gamepad preset to D-pad and drew a weapon with the pad. The game crashed
inside SkyPrompt's frame drawing. The Crash Triage record is
`TKL-Agent/Crash Triage/crashes/2026-09-28-23-30-09.md`.

### What the code shows

This comes from SkyPrompt 2.4.0 source (`QTR-Modding/SkyPrompt`, tag `v2.4.0`, MIT) and from the
installed DLL, disassembled with capstone.

- The fault is `AcquireSRWLockShared` on address 0 (`lock cmpxchg [rcx], r8`, `r8 = 0x11`).
- `SkyPrompt.dll+0x70850` is `Manager::ShowPromptRow(index, isList, visibleCount)`. It reads
  `managers[index]` (`this+0x30` is `managers.data()`), then locks that `SubManager`'s `q_mutex_`,
  its first member. At the fault, `index` was 1 (`rbp`) and `managers[1]` was null (`rdi`).
- `+0x711A2` is the second loop of `Manager::ShowQueue`. That loop draws the rows attached to an
  object. Every CIGAR prompt is attached, to the player or to PromptAnchor's marker. The render batch
  already held one row (`rsi = 0x48`, one entry), so row 0 of the group had been drawn.
- `ShowQueue` collects the row indices in its first loop under `shared_lock(mutex_)`. It then drops
  the lock, draws the unattached window, and takes the lock again for the second loop, which reuses
  those indices. If a row is removed in between (`RemovePrompt` -> `Manager::RemoveFromQ` ->
  `CleanUpQueue` -> `RemoveEmptyRows`), the stale index reads past the vector's end. The moved-out
  slot there holds null.
- This index form arrived in 2.4.0 (`b0c86ed`, "Add List layout"). 2.3.x kept raw `SubManager*`
  across the same gap, which is a use-after-free that rarely shows.

### Why the D-pad was not the cause

The first hypothesis was that SkyPrompt had no icon or queue for codes 266-269. The source rules this
out:

- `IconFont::Manager::GetIcon` maps `kGamepadButtonOffset_DPAD_UP..RIGHT` (266-269) straight to the
  arrow icons.
- Rows are grouped by the attached object's FormID, not by key.
- Switching the input device (`Input::Manager::UpdateInputDevice`) does not rebuild any queue.
- D-pad prompts were on screen for about 17 s before the crash.

What happened at 23:30:09 is the weapon draw. Observe's gate went false (`drawn=true`), so CIGAR
withdrew its prompt. That left CIGAR's two attached rows (Observe and Dress) at one.

### Why a CIGAR withdraw can land inside SkyPrompt's draw

SkyPrompt draws from its hook on `BSGraphics::Renderer::End`'s Present call (REL 75461/77246,
`+0x9`/`+0x9`/`+0x15`), on thread 2508 in this session. CIGAR's modules run in SKSE tasks
(`BSTaskPool::ProcessTasks`). Other plugins' logs from the same session show game-logic callbacks on
pool threads (TypeMode's menu events on 33336, 33344 and 33348; OAR on 29764 at the crash moment), not
on 2508. A task that calls `RemovePrompt` while 2508 is between the two loops produces exactly this
fault. CIGAR also rewrote the text and buttons SkyPrompt reads through `GetPrompts()` from the same
tasks, for example Pass Time's live text. That is a second race of the same kind.

It is not yet proven from a log that CIGAR's tasks and the Present call run at the same time. The new
build logs it: see below.

## The change

- `Prompts::InstallRenderHook()` hooks the same Present call at plugin load (a chained
  `write_call<5>`), before the first frame. The hook first checks that the site holds an `E8` call.
- Every `SendPrompt` and `RemovePrompt` is queued by `PromptSlot` (game thread) and delivered by the
  hook on the render thread, before the next hook in the chain runs. SkyPrompt draws on that thread
  too, so a CIGAR call can no longer fall between the two loops.
- What SkyPrompt reads through `GetPrompts()` is a render-thread copy (`published`). It changes only
  in `PromptSlot::Deliver`. The previous text is kept one more round, for events SkyPrompt queued with
  a `string_view` into it. The game thread keeps its own `desired` copy.
- Module logic, key slots and all game-state reads stay in the tasks.
- Fallback: without a Present call for 5 s (another mod cut the chain, or the hook failed to
  install), the queue is delivered from the game thread as before, with one warning line.

## Self-report in the log

- `prompt queue: Present hooked (chained: true)` at load. An error line appears instead if the site
  is not a call.
- `prompt queue: first Present call on thread N (SkyPrompt draws on this thread)`.
- `prompt queue: ticks run on thread M, Present on thread N`. If M differs from N, the premise is
  confirmed.
- `prompt queue: a tick ran while the render thread was inside Present (threads M and N) ...`. This
  is logged once, the first time it is seen, and is direct evidence that the two overlap.
- Offer lines keep their form (`offer event=... slot=... key=... sent=...`). They are now written when
  the render thread delivers them, at most one frame later.

## Build check

`tools/check_input_map.py` (run by `Build.ps1`) fails the build in these cases:

- A `SkyPromptAPI::SendPrompt` or `RemovePrompt` appears anywhere but `PromptSlot::Deliver`.
- The hook is not installed at plugin load, or it is not on SkyPrompt's draw site.
- A D-pad or mouse prompt key has no icon file in the winning ImGui Icons mod (`Up`, `Down`, `Left`,
  `Right`, `Mouse3`-`Mouse8`). A missing icon would not crash, but SkyPrompt would show its
  unknown-key icon.

## Upstream

The index reuse in `Manager::ShowQueue` is SkyPrompt's bug. Two fixes would close it: one lock across
both loops, or collecting `SubManager*` and holding the lock throughout. It can reach any SkyPrompt
client that calls from a task. Reporting it upstream is public posting, so it waits for the user.
