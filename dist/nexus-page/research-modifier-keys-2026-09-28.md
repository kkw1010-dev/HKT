# Research: modifier keys with SkyPrompt prompts, 2026-09-28

Requested by the orchestrator session for the user, after IAMTOKKO's 2026-09-28 6:32AM comment
("need modifier King ,,, im using controller (Keyboard setup on my controllers Key)"). Three
questions: does any SkyPrompt mod support modifier + prompt key? Does SkyPrompt itself? Whose job
is it?

**Result: the finding does not support "only SkyPrompt could do it".** SkyPrompt has no key
combinations, but a client mod can gate its own prompts on a held modifier, and one published
SkyPrompt mod does exactly that. As instructed, no reply draft was written; the decision on what to
answer goes back to the user.

## a. SkyPrompt mods on Nexus

Source: Nexus "Mods using this mod" for SkyPrompt (148703), read from the page's own list endpoint
`/api/games/1704/mods/148703/required-by?show_adult_content=1` (36 mods, adult content included).
The public v2 GraphQL `modsRequiringThisMod` returns 29 of them. Every non-translation description
was scanned for modifier, Shift, Ctrl, Alt, combo, chord and gamepad wording. Mods that use
SkyPrompt without listing it as a requirement (Grapple, for one), and Streamlined Interactions
(Discord only), are not in this list.

| Mod | What it does with keys |
|---|---|
| **Bury - Take Bodies** ([184222](https://www.nexusmods.com/skyrimspecialedition/mods/184222), source [StinkyPumpkin/Bury-NPC](https://github.com/StinkyPumpkin/Bury-NPC)) | **Shift + prompt key.** "Shift + Tap F", "Shift + Hold E" and so on; a grave's prompt is "hidden until you hold Shift". |
| Camping Plus Plus ([122554](https://www.nexusmods.com/skyrimspecialedition/mods/122554)) | Without SkyPrompt it uses the DAK key (Shift by default, a separate framework). With SkyPrompt it switches to plain 1, 2 and 3. |
| Pick Up As Junk ([186759](https://www.nexusmods.com/skyrimspecialedition/mods/186759)) | Left Ctrl + Z undo is a plain hotkey, not a SkyPrompt prompt. |
| Dynamic Feed Overhaul ([171366](https://www.nexusmods.com/skyrimspecialedition/mods/171366)), NPC Lore at a Glance ([168614](https://www.nexusmods.com/skyrimspecialedition/mods/168614)), Junk It ([112282](https://www.nexusmods.com/skyrimspecialedition/mods/112282)) | Single keys with a separate gamepad button (feed G / pad A; N / pad Back; tap and hold counts). |
| SkyPlace ([149455](https://www.nexusmods.com/skyrimspecialedition/mods/149455)), Interactive Item Stack ([158180](https://www.nexusmods.com/skyrimspecialedition/mods/158180)) | JSON bindings: one key per device, a `doubleTap` trigger. No combinations are documented. |
| The other 20 non-translation mods (RSE - Shoulder Or Saddle among them) | Nothing about modifiers. |

### How Bury does it (source read)

`src/PromptManager.cpp`, `src/Settings.cpp` in StinkyPumpkin/Bury-NPC:

- `ModifierInputSink`, a `BSTEventSink<InputEvent*>`, watches one **keyboard** key
  (`graveDestroyModifier = 42`, Left Shift).
- On key down it calls `SkyPromptAPI::SendPrompt` for the corpse's or grave's prompts. On key up it
  calls `RemovePrompt`. With `shiftGatesPrompts = true` (the default) the prompts exist only while
  Shift is held.
- The prompts themselves carry ordinary single keys (E or F, and an optional gamepad button).
  SkyPrompt only ever sees a single key.
- The modifier is keyboard-only; the gamepad has no modifier. It also hides QuickLoot while Shift is
  held.

## b. SkyPrompt itself

- **API** (`include/SkyPrompt/API.hpp` 2.x, MIT): a `Prompt` has
  `button_key: span<pair<INPUT_DEVICE, ButtonID>>`, so at most one button per device. There is no
  modifier or chord field. Prompt types are `kSinglePress`, `kHold`, `kHoldAndKeep` and the hint
  variants. The Papyrus API ([Papyrus Index](https://papyrus.bellcube.dev/skyrimse/source/skyprompt/),
  local `Scripts/Source/SkyPrompt.psc`) is the same: `SendPrompt(..., Int[] devices, Int[] keys)`.
- **Gamepad:** a native part of SkyPrompt. `settings.json` keeps separate keys per device (keyboard
  1-4; Xbox B, X, Y, A; PS4 its own). A device the client lists no key for gets SkyPrompt's key for
  that slot. The API wiki
  ([Devices and Key Codes](https://github.com/QTR-Modding/SkyPromptAPI/wiki/Devices-and-Key-Codes))
  says "SkyPrompt has default keys that will be assigned to your prompts if you do not provide
  custom keys".
- **SkyPrompt's own modifier:** only `controls.vrNavigationModifier` ("Hold this button and use the
  left stick or trackpad…"), a VR control for picking between prompts (strings in `SkyPrompt.dll`
  2.4.0). It is not a key combination for a prompt.
- Nexus page ([148703](https://www.nexusmods.com/skyrimspecialedition/mods/148703)) and GitHub
  ([QTR-Modding/SkyPrompt](https://github.com/QTR-Modding/SkyPrompt)): no mention of modifiers or
  combinations. A web search turned up nothing further.

## c. Whose job is it

- **A real combination prompt** (SkyPrompt draws "Shift + E" and accepts only the chord) would have
  to come from SkyPrompt. The API cannot express it.
- **A modifier gate** (prompts appear only while a modifier is held, then take their usual single
  key) is something a client can do on its own; Bury ships it. So "an individual mod cannot" is not
  true.

What it would cost CIGAR, if it were ever wanted:

- **Design.** CIGAR's point is that the game offers the action by itself. Behind a held modifier,
  prompts are invisible until the player already knows to ask, which undoes that
  (`cigar-design-philosophy`). As a switch it becomes a per-player rule, which the same memory
  rules out.
- **Keys the game uses.** In Skyrim SE's defaults Shift is run, Left Alt is sprint and Left Ctrl is
  sneak. Holding one changes movement. Bury gets away with it next to a still corpse, but CIGAR's
  combat and movement prompts (potion, Jujutsu, execute, weapon swap, lock-on) would fight it.
- **Gamepad.** A keyboard modifier does nothing on a native pad, and every pad button is already
  taken, so a pad modifier would steal one. IAMTOKKO is a special case: a keyboard key through a
  mapper would reach them.
- **Behaviour.** Hold prompts (pass time, Observe) and SkyPrompt's double-tap decline would end or
  change when the modifier is released. Every module's prompt would need retesting in game.
- **Engineering.** Moderate: one input sink plus a gate in the prompt slot code, then an in-game
  pass over all modules.
- **Decision.** The user rejected modifier combinations on 2026-09-27
  (`docs/007-control-panel.md`, "Rejected"). This research changes only the technical reason one
  could give, not that decision.

For IAMTOKKO's own case, a mapper can already do it without CIGAR: a pad chord (Steam Input,
DS4Windows) that sends one spare key, picked on CIGAR's Keys page.

## Not written

No reply draft. The instruction was to report only when the finding differs from "only SkyPrompt
could do it". A reply built on "technically impossible in CIGAR" would be untrue: Bury shows
otherwise, and IAMTOKKO could point to it.
