"""Prompt input checks for CIGAR (docs/016). Exit 1 on any failure; Build.ps1 runs it after every build.

Every failure here is silent in game: a D-pad slot sent with the wrong code gets no icon and never
fires, a mouse code the panel lists but Settings rejects falls back to the default key without a
word, a mouse key read as a scan code ends pass time after 0.3 s, and a shipped CIGAR.json with the
D-pad preset takes Favorites and the hotkeys from every pad player who never chose it. A SkyPrompt call
made outside the render-thread queue can race SkyPrompt's drawing (the CTD of 2026-09-28 23:30:09), and
a button code with no icon file shows SkyPrompt's unknown-key icon. The last part only reports how this
modlist's SkyPrompt and controlmap use the D-pad, which the r6 test reads.

Usage: check_input_map.py [path to the built CIGAR.dll]
"""
import json
import os
import pathlib
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
SRC = os.path.join(REPO, "src")
MO2 = r"C:\TAKEALOOK"
MODS = os.path.join(MO2, "mods")

# SKSE's linear gamepad codes, which SkyPrompt's Input::Manager::Convert takes for kGamepad.
DPAD = [266, 267, 268, 269]  # Up, Down, Left, Right
# SkyPrompt's mouse codes: 256 left (attack) and 257 right (block) stay out, the user's choice.
MOUSE = list(range(258, 264))
# XInput masks in controlmap.txt.
XINPUT_DPAD = {0x0001: "Up", 0x0002: "Down", 0x0004: "Left", 0x0008: "Right"}

failures = []


def check(ok, what):
    print(("PASS " if ok else "FAIL ") + what)
    if not ok:
        failures.append(what)


def note(what):
    print("INFO " + what)


def read(name):
    with open(os.path.join(SRC, name), encoding="utf-8") as f:
        return f.read()


def check_source():
    settings = read("Settings.h")
    m = re.search(r"kDpadButtons\{\s*([\d,\s]+)\}", settings)
    got = [int(x) for x in m.group(1).split(",")] if m else None
    check(got == DPAD, "Settings.h D-pad slots are Up, Down, Left, Right (266-269): %s" % got)
    first = re.search(r"kMouseFirst\s*=\s*(\d+)", settings)
    last = re.search(r"kMouseLast\s*=\s*(\d+)", settings)
    bounds = (int(first.group(1)), int(last.group(1))) if first and last else None
    check(bounds == (MOUSE[0], MOUSE[-1]), "Settings.h mouse keys are 258-263: %s" % (bounds,))

    panel = read("Panel.cpp")
    listed = sorted({int(c) for c in re.findall(r"KeyName\{\s*(\d+)\s*,", panel) if int(c) >= 256})
    check(listed == MOUSE, "the panel's key list offers exactly the mouse keys Settings accepts: %s" % listed)

    prompt = read("Prompt.cpp")
    check("key >= 256 ? RE::INPUT_DEVICE::kMouse : RE::INPUT_DEVICE::kKeyboard" in prompt,
          "Prompt.cpp sends a mouse key as a mouse button")
    check("RE::INPUT_DEVICE::kGamepad, pad" in prompt and "kDpadButtons[slot]" in prompt,
          "Prompt.cpp sends the slot's D-pad button as a gamepad button")

    rest = read("Rest.cpp")
    check("key < 256 && KeyDown(key)" in rest,
          "Rest.cpp does not read a mouse key as a scan code (pass time would stop after 0.3 s)")

    with open(os.path.join(HERE, "verify_deploy.py"), encoding="utf-8") as f:
        verify = f.read()
    check("258 <= k <= 263" in verify, "verify_deploy.py accepts the same mouse keys")


def check_prompt_queue():
    """Every SkyPrompt call goes through PromptSlot::Deliver, which the Present hook runs on the render
    thread (src/Prompt.cpp). A direct call from a tick races SkyPrompt 2.4.0's Manager::ShowQueue, which
    reuses row indices across a lock release, and a row removed in between makes it lock a null row."""
    calls = []
    for path in sorted(pathlib.Path(SRC).glob("*.cpp")):
        function = "?"
        for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            header = re.match(r"^\t(?:[\w:<>,\*& ]+\s)?([\w:~]+)\(.*\)\s*(?:const)?\s*$", line)
            if header and not line.strip().startswith(("if", "for", "while", "return", "switch")):
                function = header.group(1)
            if re.search(r"SkyPromptAPI::(SendPrompt|RemovePrompt)\(", line):
                calls.append("%s:%d %s" % (path.name, number, function))
    stray = [c for c in calls if not c.startswith("Prompt.cpp:") or not c.endswith(" PromptSlot::Deliver")]
    check(len(calls) == 2 and not stray,
          "SkyPrompt send/remove only in PromptSlot::Deliver: %s" % (", ".join(stray) or "%d calls" % len(calls)))
    main = read("main.cpp")
    load = main[main.find("SKSEPluginLoad"):]
    check("Prompts::InstallRenderHook();" in load, "the Present hook is installed at plugin load")
    prompt = read("Prompt.cpp")
    check("RelocationID(75461, 77246)" in prompt and "Relocate(0x9, 0x9, 0x15)" in prompt,
          "the hook sits on BSGraphics::Renderer::End's Present call, SkyPrompt's own draw site")


def check_icons():
    """SkyPrompt 2.4.0 draws D-pad 266-269 with the Up/Down/Left/Right icons and mouse 258-263 with
    Mouse3-Mouse8 (IconFont.cpp), loaded from Interface/ImGuiIcons/Icons of the winning mod."""
    if not os.path.isdir(MODS):
        note("no MO2 modlist here; icon files are not checked")
        return
    names = ["Up", "Down", "Left", "Right"] + ["Mouse%d" % n for n in range(3, 9)]
    missing = []
    for name in names:
        found = winner(os.path.join("Interface", "ImGuiIcons", "Icons", name + ".png"))
        if not found:
            missing.append(name)
    check(not missing, "SkyPrompt has an icon for every D-pad and mouse prompt key%s"
          % (": missing " + ", ".join(missing) if missing else ""))


def check_dll(path):
    if not path:
        note("no DLL given; its strings are not checked")
        return
    with open(path, "rb") as f:
        data = f.read()
    for text in (" pad={}", "settings: gamepad buttons {}", "padButtons", "player equip: {} {} ({:08X})",
                 "prompt queue: Present hooked (chained: {})", "prompt queue: ticks run on thread {}, Present on thread {}"):
        check(text.encode() in data, "the built DLL carries '%s'" % text)


def check_shipped_default():
    path = os.path.join(REPO, "dist", "CIGAR.json")
    with open(path, encoding="utf-8") as f:
        prompt = json.load(f).get("prompt", {})
    pad = prompt.get("padButtons", "skyprompt")
    check(pad == "skyprompt", "dist/CIGAR.json leaves pads on SkyPrompt's buttons: %s" % pad)
    keys = prompt.get("keys", [])
    check(all(0 < k < 256 for k in keys), "dist/CIGAR.json ships keyboard keys only: %s" % keys)


def active_profile():
    with open(os.path.join(MO2, "ModOrganizer.ini"), encoding="utf-8", errors="replace") as f:
        for line in f:
            if line.startswith("selected_profile="):
                value = line.split("=", 1)[1].strip()
                if value.startswith("@ByteArray(") and value.endswith(")"):
                    value = value[len("@ByteArray("):-1]
                return value
    return None


def winner(relpath):
    """The enabled mod whose copy of relpath wins, or the overwrite folder, or None."""
    overwrite = os.path.join(MO2, "overwrite", relpath)
    if os.path.isfile(overwrite):
        return "overwrite", overwrite
    profile = active_profile()
    if not profile:
        return None
    with open(os.path.join(MO2, "profiles", profile, "modlist.txt"), encoding="utf-8-sig") as f:
        mods = [line[1:].rstrip("\r\n") for line in f if line.startswith("+")]
    for mod in mods:
        candidate = os.path.join(MODS, mod, relpath)
        if os.path.isfile(candidate):
            return mod, candidate
    return None


def report_modlist():
    """What the D-pad does here without CIGAR; information only, never a failure."""
    if not os.path.isdir(MODS):
        note("no MO2 modlist here; the D-pad report is skipped")
        return
    found = winner(os.path.join("SKSE", "Plugins", "SkyPrompt", "settings.json"))
    if not found:
        note("SkyPrompt settings.json: none, so SkyPrompt's defaults apply (pad paging on D-pad Left/Right,"
             " which CIGAR's D-pad slots 3-4 take over while shown)")
    else:
        mod, path = found
        try:
            with open(path, encoding="utf-8") as f:
                mcp = json.load(f).get("MCP", {})
            codes = {side: mcp.get(side, {}).get("Gamepad (Xbox)") for side in ("cycle_L", "cycle_R")}
            on_dpad = any(c in (DPAD[2], DPAD[3]) for c in codes.values())
            note("SkyPrompt settings.json (%s): Xbox paging %s%s" % (
                mod, codes, " - on D-pad Left/Right, shared with CIGAR's D-pad slots 3-4" if on_dpad else ""))
        except (OSError, ValueError, AttributeError) as e:
            note("SkyPrompt settings.json (%s) unreadable: %s" % (mod, e))

    found = winner(os.path.join("interface", "controls", "pc", "controlmap.txt"))
    if not found:
        note("controlmap.txt: the game's own (D-pad Up/Down Favorites, Left/Right Hotkey1/2)")
        return
    mod, path = found
    binds = []
    started = False
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            if line.strip() == "":
                if started:
                    break  # a blank line after the first event ends Main Gameplay
                continue
            # Tab-separated; event names such as "Left Attack/Block" contain spaces.
            fields = [x.strip() for x in line.split("	") if x.strip()]
            if fields[0].startswith("//"):
                continue
            started = True
            if len(fields) < 4:
                continue
            for mask in fields[3].split(","):
                try:
                    value = int(mask, 16)
                except ValueError:
                    continue
                if value in XINPUT_DPAD:
                    binds.append("%s=%s" % (XINPUT_DPAD[value], fields[0]))
    note("controlmap.txt (%s), gameplay D-pad: %s" % (mod, ", ".join(binds) or "unbound"))


def main():
    check_source()
    check_prompt_queue()
    check_icons()
    check_dll(sys.argv[1] if len(sys.argv) > 1 else None)
    check_shipped_default()
    report_modlist()
    if failures:
        print(f"{len(failures)} prompt input check(s) failed")
        sys.exit(1)


if __name__ == "__main__":
    main()
