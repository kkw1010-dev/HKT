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
