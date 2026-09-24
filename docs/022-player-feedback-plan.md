# 022 · Player feedback: prompts in menus, and custom rules (plan)

Status (2026-09-24): plan only. Feedback from another player, relayed by the user with a
screenshot: CIGAR's 탈의하기 prompt stays on screen over the Journal (System tab). The player asked
whether that is unavoidable, and whether players could make their own rules and keys, "like
Scratch or Arduino".

## 1. Prompts over menus

### Why it happens

`RunTick` stops ticking modules while the game is paused, but nothing takes the prompts already
on screen down, and SkyPrompt keeps drawing queued prompts. SkyPrompt's themes have a
`hide_in_menu` key (read by `Theme::Theme::ReLoad`, seen in `SkyPrompt.dll`); the default theme
CIGAR uses does not set it.

### Options

- **A. A CIGAR theme with `hide_in_menu` (preferred first step).** Ship
  `SKSE/Plugins/SkyPrompt/themes/CIGAR.json` (the default theme's values plus
  `"hide_in_menu": true`) and call `SkyPromptAPI::RequestTheme(clientID, <its name>)` after
  `RequestClientID`. SkyPrompt's own mechanism, per client, no state in CIGAR. Unknowns to check in
  game: which menus SkyPrompt counts as "menu", and whether the theme's other values are applied
  exactly as the default (the look must not change).
- **B. CIGAR suspends itself while menus are open (the user's idea, refined).** Rather than
  detecting SWF files, listen to the engine's `MenuOpenCloseEvent`, which names each menu
  (InventoryMenu, Journal Menu, MagicMenu, MapMenu, TweenMenu, Console, ContainerMenu, BarterMenu,
  Crafting Menu, Lockpicking Menu, Book Menu, Sleep/Wait Menu, ...). While any menu outside a short
  allow-list (HUD Menu, Fader Menu, Cursor Menu and the like) is open: `Prompts::WithdrawEverything()`
  and skip module updates; on the last close, prompts are offered again on the next tick.
  Independent of SkyPrompt's version, and it also keeps modules from acting mid-menu. CIGAR's own
  control panel is drawn by SKSE Menu Framework, not a game menu, so it needs its own check if it
  should hide prompts too.
- **Recommendation:** A and B together. A hides prompts the moment a menu opens, even before a
  CIGAR tick; B is the backstop that does not depend on SkyPrompt and covers non-pausing menus
  (dialogue, barter, crafting) where some prompts would still be offered. Both are small.
- **Self-check:** log each suspend and resume with the menu name, and have `verify_deploy.py`
  check that the theme file is deployed and carries `hide_in_menu`.

## 2. Player-made rules ("Scratch/Arduino")

An idea to scope, not a commitment: players write their own prompts as data, without C++.

- **Rule files.** `SKSE/Plugins/CIGAR/Rules/*.json`, each rule: prompt text, input (tap or hold),
  conditions, action, cooldown, and whether it hides in combat. Rule packs can then be shared as
  plain files.
- **Conditions: reuse the game's own condition functions.** CIGAR already evaluates `GetLightLevel`
  through a `TESConditionItem` (Light). The same route opens the engine's whole condition list
  (the ones the Creation Kit shows: `IsInCombat`, `GetActorValuePercent`, `GetInCurrentLocType`,
  `HasKeyword`, `IsSneaking`, `GetEquipped`, ...), with forms named as `Plugin.esp|FormID` like
  SPID and SkyPatcher. Plus a few CIGAR-native checks the engine lacks: crosshair target type,
  looking at the floor, standing still for N seconds, a nearby reference from a form list.
- **Actions.** Press any key (how CIGAR drives TCL, TDM and Valhalla today), send an animation
  event or play an idle, equip or use an item, cast a spell, run a console command, send a
  Papyrus mod event (so script mods can listen), set a global.
- **Control panel.** A rule list with on/off, an editor for the fields, and a live "why not shown"
  line per rule (the failing condition), in the style of the existing gate strings. A block-style
  editor is a later step, not the first.
- **Phases.** 1: JSON rules, conditions through the engine, key/console/mod-event actions, a
  read-only panel list with the failing condition. 2: panel editor and more actions. 3: shareable
  rule packs.
- **Risks.** Condition parameters need parsing and validation (a wrong one must say so, never fail
  silently); console commands are powerful, so they stay opt-in; dozens of rules at 10 ticks a
  second is cheap, hundreds may need throttling.
