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

## Decided (the user, 2026-09-28): two pad presets, and the mouse's middle and side buttons

The pad gets a preset, not a per-button picker; the mouse joins the keyboard key list. Whether this
goes into the next release is still open, so nothing is coded yet.

- **Pad preset 1, "SkyPrompt 설정 따름" (default).** As today: CIGAR lists no pad button, so the pad
  uses SkyPrompt's slot buttons (A/B/X/Y by default) and whatever the player set in SkyPrompt's
  Controls.
- **Pad preset 2, "D-pad".** Slots 1-4 on the four D-pad directions.
- **Mouse.** The middle and side buttons join the prompt-key list; left and right stay out (attack and
  block).

### Implementation plan (Claude's design; nothing built)

**D-pad slot order: 1 Up, 2 Down, 3 Left, 4 Right** (SKSE linear codes 266, 267, 268, 269). Why:

- A prompt takes the lowest free slot (`AcquireKeySlot`), so slot 1 carries most prompts and slot 2
  most of the rest; three or four CIGAR prompts at once are rare.
- Up and Down are vanilla Favorites only, which SkyPrompt swallows just while such a prompt is up.
  Left and Right are also SkyPrompt's paging between mods' prompts (`cycle_L` 268 / `cycle_R` 269
  here), and a prompt on a paging key blocks paging while it shows. Putting them last keeps paging
  intact in almost every situation.
- Left before Right: Right is "next page", the paging move used more; it is the last to be taken.

**Code** (about 120-180 lines, no engine code):

- `Settings`: `prompt.padPreset` = `"skyprompt"` (default) or `"dpad"` in `CIGAR.json`; the
  prompt-key validation also accepts the mouse codes 258-263 (middle, then side buttons).
- `Prompt.cpp` `Offer`: the keyboard pair becomes `{ kMouse, key }` for 256 and up; with the D-pad preset
  a second pair `{ kGamepad, 266 + slot }`. The offer log line names the pad button.
- `Panel` (4. Keys): under the keyboard row, a "게임패드" choice of the two presets with a line on
  what each does; the key list gains "마우스 가운데", "마우스 4", "마우스 5" (and 6-8 for mice that have
  them). Changing the preset takes every prompt down so each is offered again with its new button, as a
  key change does now.
- Panel warnings:
  - D-pad preset and SkyPrompt's gamepad `cycle_L` / `cycle_R` on D-pad left or right (read from
    SkyPrompt's `settings.json` through the VFS): "3·4번 슬롯(D-pad 왼쪽·오른쪽)이 떠 있는 동안에는
    SkyPrompt 페이지 넘김 대신 프롬프트가 실행됩니다".
  - A prompt key equal to TDM's lock key, besides the Grapple, Acheron and Valhalla keys it checks
    now. This modlist's TDM lock key is 258, the middle mouse button.
- `Rest.cpp` pass time: a mouse key is not a keyboard hold (`KeyDown` maps scan codes), so it ends on
  SkyPrompt's key-up like a pad hold.
- `tools/verify_deploy.py`: the prompt-key check accepts the mouse codes.
- Texts: README (both), the Nexus description's Input line, and the pinned FAQ.

**Tests** (a new game, third person; `coc WhiterunBanneredMare` for the town, `player.placeatme
0001BCD8` for a bandit):

1. Default preset: pad prompts on A/B/X/Y as before (regression).
2. D-pad preset, one prompt (sit or lie at a floor spot): D-pad Up fires it; with it up, Favorites does
   not open; with no prompt, D-pad Up opens Favorites.
3. Two prompts (for example 주시하기 and 앉기): Up and Down; a hold prompt and pass time on the D-pad, pass
   time ending on release.
4. Three or four prompts in combat (bandit: 록온, 무기 전환, 유술, ...): Left and Right fire; the panel shows
   the paging warning; paging to another mod's prompts still works while slots 3-4 are free.
5. Preset changed in the panel: prompts come back with the new buttons.
6. Keyboard and pad alternated: the icons follow the device.
7. Mouse: slot 1 on a side button fires, including a hold prompt; slot 1 on the middle button shows
   the TDM warning.

Facts read from SkyPrompt's source on 2026-09-28 (`QTR-Modding/SkyPrompt`, `main`, MIT), the ground for
both options:

- SkyPrompt's own gamepad picker offers every pad button from 266 up, D-pad included
  (`Input::Manager::GetKeys`).
- A pad key reaches a prompt either as SKSE's linear code (266 and up) or as an XInput mask
  (`Input::Manager::Convert`), so the first review's open "code space" question has no risk left.
- While a prompt of type `kSinglePress`, `kHold` or `kHoldAndKeep` (all CIGAR uses) shows a button,
  SkyPrompt swallows that button (`InputHook::ProcessInput`, `PromptTypeFlags` `kBlock`): a D-pad slot
  set in SkyPrompt does not also open Favorites or equip a hotkey while its prompt is up, and the D-pad
  works as usual when none is. A prompt key equal to the paging key wins over paging.
