# SI-Extensions

Personal extensions to **Streamlined Interactions** (SI) for the TAKEALOOK Skyrim SE
modlist (`C:\TAKEALOOK`, profile `TKL - MUNG ADDON`). Not distributed.

SI is a closed-source SKSE DLL. Extensions here never touch its files: each one
turns the SI module it replaces off through a settings override and provides its
own SkyPrompt prompt that drives the target mod's real logic.

## Modules

| Module | Replaces SI module | Target mod | Status |
|---|---|---|---|
| `bathe` | `Bathe` (plays `mzinBatheA5_T1` only; BiS dirt state untouched) and `DressActions.enabled_water` (strips the Softbody SMP carrier) | Bathing in Skyrim - Renewed 2.7.8 | **Working in game (2026-09-17)**: prompt → BiS wash, dirt reset confirmed. undress → bathe → dress confirmed (2026-09-17); shower path untested |

See `docs/001-bathe-bis-integration.md` for the analysis and test results behind
`bathe`, and `docs/000-adding-a-module.md` for the procedure to add the next one.

## Layout

```text
modules/<name>/Source/Scripts/   Papyrus sources (ASCII only)
modules/<name>/strings.ko.json   Korean player-facing text, patched into the .pex after compiling
stubs/                           compile-time declarations of third-party scripts; never deployed
plugin/SI-Extensions.records.json  houseCARL manifest the plugin was generated from
plugin/SI-Extensions.esp         the generated ESL-flagged plugin (deployed copy must match)
tools/Build.ps1                  compile, patch text, deploy, verify
tools/patch_pex_strings.py       placeholder -> UTF-8 rewrite of a .pex string table
tools/sync_si_settings.py        keeps replaced SI modules switched off in the override
tools/verify_deploy.py           deployment assertions (exit 1 on any failure)
```

Deployed to the MO2 mod `C:\TAKEALOOK\mods\SI-Extensions`, placed directly above
`[NoDelete] 0008 StreamlinedInteractions` so its `settings.json` override wins;
the plugin loads right after `Bathing in Skyrim.esp`.

## Build

Close Skyrim first (MO2 may stay open), then:

```powershell
powershell -ExecutionPolicy Bypass -File C:\TAKEALOOK\TKL-Agent\SI-Extensions\tools\Build.ps1 -Deploy
```

Without `-Deploy` it only compiles and patches into `build/`.

### Why the Korean text is patched in

The Creation Kit's `PapyrusCompiler.exe` decodes sources in the system ANSI code
page (949 here). UTF-8 Korean literals get mangled and the parse fails
(`mismatched character '\n' expecting '"'`). Sources therefore use placeholders
like `@SIX:bathe@`; `patch_pex_strings.py` swaps them for UTF-8 in the compiled
string table, which SkyPrompt renders correctly (Camping Plus Plus KR ships UTF-8
the same way). The build fails if a placeholder is missing or left behind,
including inside a docstring.

### Plugin changes

The plugin was generated with houseCARL (`housecarl_create_plugin` with ESL, then
`housecarl_create` with `records=@plugin/SI-Extensions.records.json` and
`into=SI-Extensions.esp`). After changing the manifest, regenerate the plugin, copy it
to `plugin/`, and rebuild. Changing quest scripts or properties needs a new game,
which is how this modlist is tested anyway.

## Runtime diagnostics

Every module writes a line on startup, on each prompt it offers, on every
SkyPrompt event, and on each action result:

- PapyrusUtil log `SIX_Bathe.log` (MO2 routes it to `overwrite`; search there for
  `SI-Extensions`), with timestamps.
- `Papyrus.0.log` lines prefixed `[SI-Extensions]` when Papyrus logging is on.
- A notification when startup fails; the reason is also in the log.
