# 016 · Gamepad, and why CIGAR adds no binding layer

**Status:** settled 2026-09-20. **CIGAR targets keyboard and mouse.** The pad is
not an audience — the user's words: "키보드로 하겠지, 애초에 엑박유저를 대상으로
한 모드도 아님". What works on a pad works because SkyPrompt supplies its own
per-device buttons, not because CIGAR aims at it, so no gamepad feature belongs
in this mod and no gamepad gap is a blocker.

## SkyPrompt already solves it

CIGAR passes SkyPrompt one button per prompt, and it is a keyboard one
(`src/Prompt.cpp`, `buttons[0] = { RE::INPUT_DEVICE::kKeyboard, key }`). A
device with no listed button falls back to SkyPrompt's own key for that slot —
the DLL carries the string `No fallback keys for device {}` for the case where
even that is missing.

Those fallbacks are not a guess. SkyPrompt's `settings.json` (here:
`mods\TAKEALOOK - MCM and INI\SKSE\Plugins\SkyPrompt\settings.json`) holds four
buttons **per device**, in exactly the shape CIGAR's panel edits for the
keyboard:

```json
"enabled_devices": {"Keyboard & Mouse":true, "Gamepad (Xbox)":true, "Gamepad (PS4)":true},
"n_max_buttons": 4,
"keys": {
  "Keyboard & Mouse": [2,3,4,5],          // 1 2 3 4
  "Gamepad (Xbox)":   [277,278,279,276],  // B X Y A
  "Gamepad (PS4)":    [277,279,276,278]
},
"cycle_L": {"Keyboard & Mouse":203, "Gamepad (Xbox)":268},  // left arrow / DPad Left
"cycle_R": {"Keyboard & Mouse":205, "Gamepad (Xbox)":269}
```

Gamepad codes follow SKSE's linear mapping from 266: 266 DPad Up … 276 A,
277 B, 278 X, 279 Y, 280 LT, 281 RT. SkyPrompt renders them with its own names
(`Gamepad A`, `Gamepad LT`, …).

**So CIGAR should not add a gamepad key picker.** Its prompts share SkyPrompt's
four slots with every other SkyPrompt client (Grapple's own QTE, for one). If CIGAR forced its own pad buttons, a pad player would get
different buttons for CIGAR prompts than for everyone else's. The keyboard
picker exists only because CIGAR moves other mods' hotkeys out of the way and
has to know which keys it took.

`cycle_L` / `cycle_R` is the escape hatch above four prompts: more prompts are
paged, not given more buttons.

## The four gamepad mods that were weighed (2026-09-20)

All four were installed but disabled. Only the first was enabled.

| Mod | Verdict | What it is |
|---|---|---|
| **Auto Input Switch** | **enabled** | Four files, one INI key (`iPreferredPlatform`, -1 = auto), no ESP, no bindings. It detects which device was last used so the UI's hints, cursor and rumble follow. This is what lets SkyPrompt decide between drawing `1` and `Ⓑ`. |
| Virtual Keyboard | left off | Text entry with a pad inside SkyUI menus (enchanting names, item search, map). No gameplay bindings. Only needed for pad-only play. |
| Gamepad++ | left off | A binding layer; see below. |
| Complete Controller Setup | left off | A whole layout built on Gamepad++, which it requires. |

### Why Gamepad++ is against this mod's design

From Gamepad++'s own translation file: four **Combo Keys**, and per action key
**Single / Double / Triple Press / Hold**, plus **Combo + Single / Double /
Triple / Hold**. One pad button therefore carries up to eight meanings,
separated by press count and a held modifier. And each one is defined as:

> "Assign the keyboard key you would like holding the Combo Key and single
> pressing the Action Key to **mimic**"

It is a keyboard-emulation layer: pad combo → synthetic keyboard key → whatever
mod listens for that key.

CIGAR answers the same shortage of buttons from the other end. The context
decides what is offered, the prompt says what it does, and one press does it —
nothing to memorise, no modifier, and an action that is not possible right now
is not on screen at all. That is also why prompt-only mode moves Grapple,
Acheron, Valhalla and Private Needs' hotkeys to F13–F15: the direction is
*fewer* bindings, not more.

To be fair to Gamepad++: it exists because a pad's buttons are all spoken for,
and mods that want a *keyboard key* rather than a context prompt have nowhere
to go. CIGAR only covers the mods it has a module for; a pad player still needs
some scheme for Wheeler, One Click Power Attack, dodge and sprint. The two are
not competing for the same job — but wherever CIGAR does cover an action, the
binding layer is redundant for it.

### Complete Controller Setup would also break the panel

Besides requiring Gamepad++, it replaces `controlmap.txt`, ships an ESL, several
DLLs including `ExtendedHotkeySystem.dll`, and MCM settings files for about
eight other mods — **including its own `SKSEMenuFramework.ini`**, which would
win over `TAKEALOOK - Font Edit` and take the Korean glyphs out of CIGAR's
control panel. `tools/check_menu_framework.py` tests exactly that file.

## Test (2026-09-20 20:22–20:46, release build)

`CIGAR.log`: `CIGAR 0-2-0-0 loaded`, all thirteen modules ticking, **20 prompts
offered and 11 accepted** across 록온, 그래플, 유술 and 소변 보기, with **no WARN
line in the whole run**.

The user reports the run passed. Two things the log cannot show, and which rest
on their observation rather than on this file:

- **which physical button was pressed.** CIGAR logs only the keyboard key it
  registered; a pad accept and a keyboard accept produce the same line, because
  the pad button comes from SkyPrompt's settings, not from CIGAR.
- **what the control panel displayed.** No panel content is logged, so the
  release panel showing the player descriptions instead of the `조건:` gate line
  was confirmed by eye.

## Known limits, not open items

These were never chased, and by the scope above they should not be:

- Whether the SKSE Menu Framework panel can be *operated* with a pad. If it
  cannot, a pad-only player edits `CIGAR.json` by hand. No MCM is planned for
  this.
- Whether `cycle_L` / `cycle_R` paging works on a pad past four prompts.
- Slot 4 on Xbox is `A`, the activate button; no conflict was reported, but it
  was not deliberately exercised.

## Review 2026-09-28: gamepad slots in CIGAR's panel? (review only, nothing changed)

The Nexus requests for modifier and arrow keys (IAMTOKKO) most likely mean a pad player who wants the
prompts on pad buttons such as the D-pad. The user asked whether "gamepad support" needs a pad button
mapping in CIGAR. Modifier and arrow keys stay rejected.

**Verdict: do not add pad slots to CIGAR's panel; point pad players to SkyPrompt's own Controls
settings.** The mapping already exists there, it applies to CIGAR's prompts, and a CIGAR-side mapping
would reopen the reason this file gives against it.

1. **The API can take a pad button per prompt.** `SkyPromptAPI::Prompt::button_key` is a span of
   `(RE::INPUT_DEVICE, ButtonID)` pairs (`include/SkyPrompt/API.hpp:60`), API 2.0, the same in
   2.3.15 and 2.4.0 (`docs/007`). `ButtonID` is commented as `RE::BSWin32GamepadDevice::Key` among
   others (line 38), and that enum has the D-pad (`kUp` 0x0001 ... `kRight` 0x0008). **But** SkyPrompt's
   own `settings.json` stores pad buttons as SKSE's linear codes (266 D-pad Up ... 276 A, 279 Y), so
   which code space the per-prompt pairs expect is unresolved; a wrong guess gives a prompt no pad
   button at all. It needs SkyPrompt's source or one in-game test to settle.
2. **SkyPrompt already lets the player remap the pad.** Its 2.4.0 DLL carries the control-panel
   strings `$SkyPromptMCPControlsDeviceSelection`, `$SkyPromptMCPControlsButton`,
   `$SkyPromptMCPControlsMaxButtons`, `$SkyPromptMCPControlsCycleLeft/Right` (its own SKSE Menu
   Framework panel, "Controls"), and `settings.json` keeps four buttons per device. CIGAR lists only a
   keyboard key per prompt (`src/Prompt.cpp:164`), so on a pad every CIGAR prompt uses SkyPrompt's slot
   buttons: whatever the player sets there, D-pad included, applies to CIGAR as to every other
   SkyPrompt mod. (Read from the DLL strings; the panel itself was not opened in game.)
3. **What a CIGAR pad row would take** (if the user still wants one):
   - `Settings`: a second array of four pad buttons, "unset = SkyPrompt's own" by default, saved in
     `CIGAR.json`.
   - `Panel`: a pad list beside the keyboard list (a fixed list of pad buttons, like `kKeys`; no
     capture code), with the conflict warnings below.
   - `Prompt.cpp:164`: a second pair `{ kGamepad, padKey }` when set.
   - `Rest.cpp:843/881`: nothing; the pass-time backstop already treats a non-keyboard hold as ending
     on SkyPrompt's key-up.
   - No other code assumes the prompt key is a keyboard key (the prompt-only keys F13-F15 and the
     synthetic presses are other mods' keyboard keys and stay so).
   - Size: about 150-250 lines, plus settling the code space and one in-game session with a pad.
4. **Risks.**
   - **The D-pad is taken.** In this modlist's `controlmap.txt` (winner: "No Numpad Hotkeys and Custom
     Keybinds") the gameplay D-pad is Favorites (up and down) and Hotkey1/Hotkey2 (left and right),
     and SkyPrompt pages prompts with D-pad left and right. Whether SkyPrompt swallows a button while
     its prompt shows is unverified; if not, a D-pad prompt also opens Favorites or equips a hotkey.
   - **Split buttons.** Per-prompt pad keys make CIGAR's prompts use other buttons than every other
     SkyPrompt mod on screen at the same time (Grapple's QTE, for one), the reason given above.
   - **Regression.** With "unset" as the default nothing changes for pad players on SkyPrompt's
     buttons; any other default would move their buttons.
   - **Tests if built:** each D-pad direction as a slot (the prompt fires; do Favorites or a hotkey also
     fire), a hold prompt and pass time on the pad, more than four prompts (paging), keyboard and pad
     alternated, the "unset" regression.
5. **Next release.** Not with push-through and the 유술 gate: the code space is unsettled and it needs its
   own pad session. What can go in now is one line in the readme and the pinned FAQ: on a gamepad, set
   the prompt buttons in SkyPrompt's own settings (Controls, gamepad device); CIGAR follows them.

## Revised the same day: under the seasoning principle, fill the gap

The user's principle (README "Principles", `docs/022`): the interaction is designed and closed; personal
adjustments are all on the table, on every device the mod says it supports. CIGAR's panel offers the
keyboard keys but no pad buttons, so the pad is a missing seasoning. **The verdict above is reversed:
add pad buttons to CIGAR's key page.** SkyPrompt's own Controls stay what they are (a setting for every
SkyPrompt mod at once); CIGAR's row is CIGAR's own. The earlier "the pad is not an audience" (2026-09-20)
and "CIGAR should not add a gamepad key picker" are superseded.

Two open points of the first review are now settled from SkyPrompt's source (`QTR-Modding/SkyPrompt`,
`main`, MIT, read 2026-09-28):

- **Code space: both work.** `Input::Manager::Convert` passes a pad key of 266 or more (SKSE's linear
  codes, the ones SkyPrompt's own `settings.json` stores) through, and converts anything lower as an
  XInput mask (`GamepadMaskToKeycode`). CIGAR should store the linear codes, the same as SkyPrompt.
- **No double action.** `InputHook::ProcessInput` swallows a button that belongs to a prompt on
  screen when the prompt type blocks input, and `kSinglePress`, `kHold` and `kHoldAndKeep` (all
  CIGAR uses) do (`PromptTypeFlags`, `kBlock`). So a D-pad slot does not also open Favorites or equip
  a hotkey while its prompt shows; with no prompt up, the D-pad does what the game says. A prompt key
  that equals SkyPrompt's paging key wins over paging (the paging check runs only when nothing was
  blocked).
- SkyPrompt's own gamepad picker offers every pad button from 266 up, D-pad included (`GetKeys`).

**What to build** (about 150-250 lines, no engine code):

- `Settings`: `padKeys[4]`, each "0 = follow SkyPrompt" by default, saved in `CIGAR.json`. The default
  changes nothing for pad players today.
- `Panel` (4. Keys): a pad row under the keyboard row, a list of pad buttons with "SkyPrompt 기본" first,
  and the same kind of warnings the keyboard row has (two slots on one button; a slot on D-pad left or
  right, SkyPrompt's paging).
- `Prompt.cpp` `Offer`: a second pair `{ kGamepad, padKey }` when set.
- Also a keyboard-and-mouse gap on the same principle: the keyboard list has no mouse buttons although
  "keyboard and mouse" is what the mod says; the middle and side buttons (SkyPrompt's 256+ mouse codes)
  could join the list, left and right staying out (attack and block). The pass-time backstop treats a
  non-keyboard hold as ending on key-up already.

**One decision for the user:** whether the pad list includes the D-pad. On the keyboard the arrow keys
were rejected because other mods' SkyPrompt prompts (Grapple's QTE) need them; the same reasoning could
apply to the D-pad on a pad, while IAMTOKKO's request is most likely exactly the D-pad.

**Tests:** each chosen pad slot fires its prompt; with that prompt up, Favorites and the hotkeys do not
also fire, and with no prompt they still work; "follow SkyPrompt" unchanged from today; a hold prompt
and pass time on the pad; switching between keyboard and pad; more than four prompts (paging).

**Release:** it can join the next release (push-through, 유술 gate) if its pad test fits the same run;
nothing in it is unsettled any more.
