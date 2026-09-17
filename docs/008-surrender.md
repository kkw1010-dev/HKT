# 008 · Surrender: Acheron / Yamete Kudasai surrender key

Status (2026-09-17): tested in game.

- **Results.** Slow motion, the pulse, the hold, Yamete Kudasai's surrender
  scene, the timer end, and the panel switch-off all worked.
- **Report: "no surrender event" after escaping.** After a surrender and a
  successful escape minigame, Acheron only answered that no surrender event
  could be found. This is Yamete Kudasai's design:
  - `Kudasai_Surrender` (`0008D0`) fills `Enemy01` only with an actor for
    which `HasMagicEffect(Kudasai_SurrenderTimeoutEFF) == 0`;
  - `KudasaiSurrender.TimeoutSpell` puts that effect on the enemies of a
    surrender, and its own comment says this is so the player cannot
    surrender to the same actor again within 3 minutes;
  - the quest therefore cannot start, and Acheron's `SelectQuestImpl` logs
    "No event quest found".

  CIGAR now hides the prompt (`yk-timeout`) while every hostile within 3000
  units carries that effect.

## Where surrender actually happens

Yamete Kudasai 2.2.3 has an MCM surrender key (`KudasaiMCM.iSurrenderKey`),
and `KudasaiMain.RegisterKeys` registers it. But `KudasaiMain.OnKeyDown` (both
the shipped source and the compiled `.pex`) only handles the assault key.
Surrender belongs to Acheron 1.9.3:

- **Key handling.** `Acheron::EventHandler::ProcessEvent`
  (KrisV-777/Acheron, `src/Acheron/EventSink.cpp`) is a
  `BSInputDeviceManager` sink. On a button *down* equal to
  `Settings::iSurrenderKey`, it:
  - finds an aggressor (`Processing::AggressorInfo`);
  - builds the member list;
  - selects a Surrender consequence (`Resolution::SelectQuest`).

  When either step fails, it shows `$Achr_SurrenderNoAggressor` or
  `$Achr_SurrenderNoQuest`.
- **Consequences.** Yamete Kudasai supplies them in
  `SKSE/Acheron/Consequences/Surrender/YameteKudasai*.yml`, which point at quest
  `0x8D0|YameteKudasai.esp`.
- **Settings.** They live in `Data/SKSE/Acheron/Settings.yaml`, from
  `TAKEALOOK - MCM and INI` on this modlist: `iSurrenderKey: 37` (K) and
  `iHunterPrideKeyMod: -1`. If a modifier is set, Acheron requires it for the
  surrender key as well.
- **Gate inside Acheron.** It ignores the key when `ProcessingEnabled` is
  false, the game is paused, the console is open, or movement controls are
  disabled.

## CIGAR

The 항복 (길게 누르기) prompt is a SkyPrompt `kHold` prompt, so it accepts only
after a full hold, because surrender cannot be undone. It is shown when:

- the player is in combat, below 40% health, alive, and not bleeding out;
- the player does not carry Acheron's `Defeated` keyword
  (`0x801|Acheron.esm`);
- the player is not in `SexLabAnimatingFaction`;
- movement controls are enabled;
- at least 5 s have passed since the last surrender press;
- when Yamete Kudasai is loaded, at least one hostile within 3000 units lacks
  `Kudasai_SurrenderTimeoutEFF` (`000808|YameteKudasai.esp`).

The 40% threshold is the user's choice. It was 20% by the user's instruction
until 2026-09-17 and was raised to 40% then (`35eb997`). It is not tied to
Acheron's knockdown threshold (`fKdHealthThresh`).

When the prompt appears, the moment is marked, following Streamlined
Interactions' low-health potion prompt (`slow_time_hp_pot`, 3 s):

- **Slow motion.** `BSTimer::SetGlobalTimeMultiplier(0.3)` (what `sgtm` does)
  is applied once per drop below 40%. The multiplier returns to 1.0 after 3 s
  of real time, or earlier when the prompt goes away, is accepted, or a game
  loads. CIGAR restores it only if the multiplier is still its own 0.3, so it
  never overrides a change made by another mod in between. SI's health-potion
  prompt may slow time at the same moment; whichever ends last wins.
- **Pulse.** The prompt text pulses between white and gold (1.5 Hz) while it
  is up. The installed SkyPrompt (DLL of 2026-03-04) has no per-prompt
  effects; upstream `PromptEffects` arrived on 2026-09-07. Instead, CIGAR
  re-sends the prompt with a new `text_color`, which SkyPrompt applies in
  place for a queued prompt (`ProcessSendPrompt` → `IsInQueue(..., true)` →
  `SubManager::Update`). Colours are ImGui ABGR.

The gate runs in `FastTick()`, every 100 ms, so the prompt and slow motion
follow a hit without the one-second delay of `Tick()`.

Accepting it re-checks the gate, ends the slow motion, and presses Acheron's
surrender key with `Util::PressKey`. Acheron then chooses the aggressor and
the consequence, so Yamete Kudasai's surrender applies when it is installed.

The prompt uses event ID 7, which no other module uses. SkyPrompt gives each
event ID its own key slot (`Manager::Add2Q`), so 항복 never shares a key with
록온 or 그래플, which can show at the same time.

## Self-reporting

- On load, the log records Acheron's settings as read (`processing`,
  `surrenderKey`, `modifier`, `defeatedKeyword`, `yk`). When the key cannot be
  pressed (unset, gamepad, modifier set, processing off), a HUD message
  appears once.
- The gate reason (`no-combat`, `health`, `down`, `defeated`, `sexlab`, `no-movement`,
  `quiet`, `yk-timeout`, `ok`) is logged when it changes. Every press is logged, and so is
  every slow-motion start and end, with its reason.
- `tools/verify_deploy.py` checks Acheron's DLL, its surrender key, the
  modifier and the processing switch.
