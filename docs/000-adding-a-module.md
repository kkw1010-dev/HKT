# 000 · Adding a module (playbook)

This is how the `Bathe` and `Dress` modules were built. Follow it for the next
CIGAR module, so their lessons do not have to be learned again.

## 1. Find out what the target does (and whether SI already does it)

1. Dump the Streamlined Interactions DLL strings: run a Python regex over
   `mods/[NoDelete] 0008 StreamlinedInteractions/SKSE/Plugins/StreamlinedInteractions.dll`.
   Look for:
   - the module name;
   - its `Modules/<Name>` MCM label;
   - hard-coded asset paths;
   - idle and event names.

   SI is only a source of ideas and of conflicts; CIGAR never calls it.
2. Decompile the target mod's `.pex` with `housecarl_decompile_script`, using a
   temporary patch name. Copy the `.psc` files to the scratchpad, then delete
   the temporary `mods/houseCARL - ...` folder (check that it is not listed in
   modlist.txt).
3. Find the target's public entry points: an API script, `Try...` functions or
   ModEvents. Note what each one checks.
4. List the forms the module needs from the target. Read them from the
   target's quest script properties at load (`Util::ScriptObject` +
   `Util::ScriptProperty`), so no FormID is hard-coded. Use
   `housecarl_records` on the quest's `VirtualMachineAdapter.Scripts[0].Properties`
   to see the property names.
5. Record all of this as `docs/NNN-<module>.md`.

## 2. Write the module (C++)

- Add `src/<Name>.h/.cpp` with a singleton class deriving from `CIGAR::Module`,
  and add it to `Modules()` in `main.cpp`.
- Add a `Label` in `src/Panel.cpp`: the Korean title, the mod it waits for, and
  one or two sentences of what the player gets. That last field is what the
  release build shows in place of the gate line.
  - `OnGameLoaded()` resolves the integration. If the target is missing, log
    why, leave the module idle, and **return quietly**. Never make the target
    a hard requirement.
  - `Tick()` computes the gate, calls `LogGate(...)`, and drives each
    `PromptSlot::Update(can, text)`.
  - `OnAccepted(eventID)` performs the action. Every module method runs on the
    game thread.
- Give each on-screen prompt its own `PromptSlot` and a new ID in `PromptID`
  (`src/Prompt.h`). SkyPrompt merges prompts that share an (event, action)
  pair, so one press would fire both owners. SkyPrompt 2.3.15 has no
  `RemovePromptByID`, so removal is per sink. `PromptSlot` lists the keyboard
  key from CIGAR's key slots itself (`docs/007-control-panel.md`); a module
  never sets keys.
- **Input policy:** non-combat contextual actions use `SkyPromptAPI::kHold` by default, so an
  incidental tap cannot change player state. Single press is for timing-sensitive combat actions
  or an explicitly documented exception. `SetHoldMode(true)` is separate: use it only when the
  module needs key-down/key-up callbacks while the hold is in progress.
- Handle only `kAccepted` (0). The other event types are `kDeclined` (1),
  `kRemovedByMod` (2), `kTimingOut` (3), `kTimeout` (4), `kDown` (5),
  `kUp` (6) and `kMove` (7). The Papyrus test logs showed 5 before 0 and
  3 → 4 on expiry.
- Call Papyrus from C++ with `DispatchMethodCall2` on the target quest's
  handle. The result arrives on a VM thread, so only log there and marshal
  anything else through `SKSE::GetTaskInterface()->AddTask`.
- Event sinks (for example `SKSE::CrosshairRefEvent`) capture a handle and hand
  the work to the game thread with `AddTask`.
- If the module keeps state across saves, add a record to the `CIGR` co-save.
  Resolve every FormID on load with `ResolveFormID`, bound counts, and check
  every read length.
- Player-facing text is UTF-8 in the source (`/utf-8`). Use short,
  administrative Korean.
- Self-reporting is required:
  - log to `CIGAR.log` for the gate, offers, events and action results;
  - notify once for every state that silently blocks the module (the target
    disabled, a replaced SI switch on).

## 3. SI overlap

When the module replaces SI switches, add them to `REPLACED` in both
`tools/sync_si_settings.py` and `tools/verify_deploy.py`, and warn at runtime
with `Util::WarnIfSIModuleOn`. SI re-applies preset switches when its menu opens
unless the preset is Power User (2), which the sync tool pins.

## 3a. Taking over another mod's hotkey (the user's rule, 2026-09-25)

When a module takes over, moves or unbinds another mod's hotkey (prompt-only), do both:

1. **Remove it from the mod itself**: whatever the mod reads the key from (its MCM Helper INI, a
   global, a script property, its own settings file). "No key" is -1, SkyUI's value; 0 can still
   show as Esc in an MCM.
2. **Change it in MCM Memory's profile**,
   `mods\TAKEALOOK - MCM Memory Profile\SKSE\Plugins\MCMMemory\Profiles\Default.json` (the
   keymap row for that mod and option). MCM Memory auto-restores its stored keys on every new game,
   after CIGAR's load-time change, so a stored key silently brings the mod's hotkey back. A row with
   value 1 and `valueSource: menu.selectedKeyCode` is Esc recorded from a cancelled remap dialog.

Back up both files first. Add the row to `PROMPT_ONLY_PROFILE_KEYS` in `tools/verify_deploy.py`,
which fails the build when the profile would restore a taken-over key or holds a cancelled-remap Esc.

## 4. Build, deploy, verify

```powershell
powershell -ExecutionPolicy Bypass -File C:\TAKEALOOK\TKL-Agent\CIGAR\tools\Build.ps1 -Deploy
```

- Skyrim must be closed; MO2 may stay open.
- A clean build proves only that the code compiles. Test in game (a fresh
  save is fine), then read `CIGAR.log` before asking the user anything: every
  failure seen so far was explained by the log.
- Check `skse64.log` for `CIGAR.dll` ... `loaded correctly` when the log file
  is missing altogether.
