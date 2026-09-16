# CIGAR

**C**ontextual **I**nteraction, **G**ameplay **A**cceleration & **R**hythm.

These are personal SkyPrompt interactions for the TAKEALOOK Skyrim SE modlist
(`C:\TAKEALOOK`, profile `TKL - MUNG ADDON`). They are not distributed.

CIGAR is its own mod. It does not depend on Streamlined Interactions (SI).
When SI ships a module that CIGAR replaces, CIGAR switches that SI module off
through a `settings.json` override in its own mod folder, so disabling CIGAR
brings SI's version back. SI keeps every module CIGAR does not replace.

This repository was called `SI-Extensions` until 2026-09-17. The scripts used
the `SIX_` prefix and the plugin was `SI-Extensions.esp`.

## Modules

| Module | Quest | Replaces in SI | Status |
|---|---|---|---|
| `bathe` | `CIGAR_BatheQuest` | `Bathe` (animation only; BiS dirt untouched) | Bathe confirmed in game 2026-09-17; shower untested |
| `dress` | `CIGAR_DressQuest` | `DressActions` water / bed / wardrobe undress (strips the Softbody SMP carrier) | Water undress→dress confirmed 2026-09-17; bed and wardrobe untested |

Prompts (keyboard):

| Where | Prompt | Key |
|---|---|---|
| In water, strippable items worn | 탈의하기 | 1 |
| In water, nothing strippable worn, BiS on | 목욕하기 (dirt %) | 1 |
| Under a waterfall | 샤워하기 (dirt %) | 2 |
| Aimed at a bed or wardrobe/dresser and within 250 units, strippable items worn | 탈의하기 | 1 |
| Away from water/bed/wardrobe with items CIGAR removed | 착용하기 | 1 |

Background: `docs/001-bathe-bis-integration.md` (bathe, test history, the
water-undress findings) and `docs/002-dress.md`. To add a module, see
`docs/000-adding-a-module.md`.

## Layout

```text
modules/core/     CIGAR_ModuleBase (SkyPrompt client, tick, prompts, log, SI checks),
                  CIGAR_Util (strip rules, worn description), CIGAR_PlayerAlias (reload)
modules/<name>/   one quest script per module, extending CIGAR_ModuleBase
strings.ko.json   Korean player-facing text, patched into the .pex after compiling
stubs/            compile-time declarations of third-party scripts; never deployed
plugin/CIGAR.records.json  houseCARL manifest the plugin was generated from
plugin/CIGAR.esp  the generated ESL-flagged plugin (deployed from here)
tools/Build.ps1   compile, patch text, deploy, sync SI settings, verify
tools/register_profile.py  enable CIGAR in the active MO2 profile (MO2 closed)
tools/sync_si_settings.py  keep replaced SI modules off and SI on the Power User preset
tools/verify_deploy.py     deployment assertions (exit 1 on any failure)
tools/patch_pex_strings.py placeholder -> UTF-8 rewrite of .pex string tables
```

The mod is deployed to `C:\TAKEALOOK\mods\CIGAR`, directly above
`[NoDelete] 0008 StreamlinedInteractions`. `CIGAR.esp` loads right after
`Bathing in Skyrim.esp`.

## Build

Close Skyrim first (MO2 may stay open), then:

```powershell
powershell -ExecutionPolicy Bypass -File C:\TAKEALOOK\TKL-Agent\CIGAR\tools\Build.ps1 -Deploy
```

Without `-Deploy`, the script only compiles and patches into `build/`. On a
fresh profile, run `python tools\register_profile.py` once with MO2 closed.

### Why the Korean text is patched in

The Creation Kit's `PapyrusCompiler.exe` decodes sources in the system ANSI
code page (949 here). UTF-8 Korean literals get mangled, and the parse fails
with `mismatched character '\n' expecting '"'`.

To work around this, sources use placeholders such as `@CIGAR:bathe@`.
`patch_pex_strings.py` then swaps them for UTF-8 in the compiled string tables,
and SkyPrompt renders that correctly. The build fails when a placeholder is
unmapped, unused, or left behind (including inside a docstring).

### Plugin changes

To change the plugin, edit `plugin/CIGAR.records.json` and regenerate the
plugin with houseCARL:

1. `housecarl_create_plugin` with `patch=CIGAR` and `esl=true`, into a fresh folder.
2. `housecarl_create` with `records=@plugin/CIGAR.records.json` and `into=CIGAR.esp`.
3. Copy the result to `plugin/`.

Then check it with
`housecarl_check plugins=["CIGAR.esp"] findings=["errors","scripts"]`.

Changing quests or script variables needs a new game, which is how this
modlist is tested anyway.

## Runtime diagnostics

Every module writes to `CIGAR.log`, which MO2 routes to
`overwrite\SKSE\Plugins\CIGAR\`. Lines carry a timestamp and a `[Bathe]` or
`[Dress]` tag. Each module logs:

- its startup;
- every change of its prompt-gate inputs;
- each prompt it offers and each SkyPrompt event;
- each action result.

On entering water, a bed or a wardrobe, the log also records every worn slot
(`(kept)` marks items undress leaves on) and what each hand holds.

A notification appears when:

- a module fails to start;
- BiS is disabled in its MCM;
- a replaced SI module has been switched on again.

`Papyrus.0.log` carries the same lines prefixed `[CIGAR]` when Papyrus logging
is on.
