# 000 · Adding a module (playbook)

This procedure produced the working `bathe` module. Follow it for the next SI
replacement, so the lessons from `bathe` do not have to be learned again.

## 1. Find out what the SI module really does

1. Dump the DLL strings (Python regex over
   `mods/[NoDelete] 0008 StreamlinedInteractions/SKSE/Plugins/StreamlinedInteractions.dll`).
   Look for the module name, its `Modules/<Name>` MCM label, hard-coded asset
   paths, and idle or event names.
2. Absence of Papyrus dispatch, ModEvent names or plugin names means SI does not
   call the target mod. It only plays animations or equips items itself.
3. Decompile the target mod's `.pex` with `housecarl_decompile_script`, using a
   temporary patch name. Copy the `.psc` files to the scratchpad, then delete the
   temporary `mods/houseCARL - ...` folder (check that it is not in modlist.txt).
4. Find the target's public entry points (an API script, `Try...` functions,
   ModEvents) and note what each one checks. Record this as
   `docs/NNN-<module>.md`.

## 2. Write the module

- Put sources in `modules/<name>/Source/Scripts/` with the `SIX_` prefix,
  **ASCII only**.
- Write player-facing text as `"@SIX:<key>@"` and add the Korean text to
  `modules/<name>/strings.ko.json` in short administrative style. The build fails
  if a placeholder is left or missing, including one inside a docstring.
- Declare each third-party script you call as a stub in `stubs/`, with signatures
  taken from the decompile. Stubs are never deployed, and `verify_deploy.py`
  checks that.
- Look up every signature with the `housecarl:papyrus-reference` skill.
  Known traps:
  - `state` is a reserved word.
  - A `"\n"` literal breaks the CK compiler; use `StringUtil.AsChar(10)`.
  - JsonUtil paths are relative to `Data/SKSE/Plugins/StorageUtilData`.
  - A mod's "enabled" global may default to 0 (BiS does), so check it and log it.
- For SkyPrompt, call `RegisterForSkyPromptEvent(self, 2, 0)` on init and again
  on every load (a player alias `OnPlayerLoadGame`). Then use
  `SendPrompt(client, text, eventID, 0, 0, PlayerRef, [0], [DIK], 0.0)`.
  Handle only event type `0` (accept). Type `5` means shown, and `3`/`4` mean
  expired.
- Self-reporting is required:
  - log to `MiscUtil.WriteToFile` (it lands in
    `overwrite/SKSE/Plugins/SI-Extensions/`);
  - log each change of the prompt-gate inputs;
  - notify once for every configuration state that silently blocks the module.
- If the module replaces an SI module, add it to `REPLACED` in
  `tools/sync_si_settings.py`. SI re-applies preset switches when its menu opens
  unless the preset is Power User (2), and the tool pins that. Also read SI's
  settings at runtime and warn when the switch is back on.

## 3. Plugin records

- Add records to `plugin/SI-Extensions.records.json`, then run `housecarl_create`
  with `records=@<that file>` and `into="SI-Extensions.esp"`.
- Copy `mods/SI-Extensions/SI-Extensions.esp` to `plugin/`, because the verifier
  requires the two to be identical.
- Run `housecarl_check plugins=["SI-Extensions.esp"] findings=["errors","scripts"]`.
  The result must show 0 dangling references and 0 unbound properties.
- Keep the ESL flag. `housecarl_create` preserves it; the verifier checks `0x200`.

## 4. Build, deploy, verify

```powershell
powershell -ExecutionPolicy Bypass -File C:\TAKEALOOK\TKL-Agent\SI-Extensions\tools\Build.ps1 -Deploy
```

- Skyrim must be closed; MO2 may stay open.
- Add the new `.pex` names to the list in `tools/verify_deploy.py`.
- Test on a new game. Read `SIX_*.log` before asking the user anything: every
  failure seen so far was explained by that log.
