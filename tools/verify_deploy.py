"""Deployment checks for CIGAR. Exit 1 on any failure.

Encodes the failure modes that are silent in game (no error, no prompt):
the mod not enabled, a stale DLL, leftovers of the earlier Papyrus/ESP builds
(which would run alongside the DLL and double every prompt), a replaced SI
module still on (double prompts), the override losing to SI's original, or a
missing hard dependency. Optional integrations (Bathing in Skyrim) are reported,
never required.
"""
import json
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
MO2 = r"C:\TAKEALOOK"
MODS = os.path.join(MO2, "mods")
MOD_NAME = "CIGAR"
MOD = os.path.join(MODS, MOD_NAME)
SI_MOD = "[NoDelete] 0008 StreamlinedInteractions"
DLL = os.path.join(MOD, "SKSE", "Plugins", "CIGAR.dll")
BUILT_DLL = os.path.join(REPO, "build", "release", "CIGAR.dll")
SI_SETTINGS = os.path.join(MOD, "SKSE", "Plugins", "StreamlinedInteractions", "settings.json")
REPLACED = [
    ("Bathe", "enabled"),
    ("DressActions", "enabled_water"),
    ("DressActions", "enabled_bed"),
    ("DressActions", "enabled_wardrobe"),
]
# Names src/BaboKey.cpp reads from BaboDialogue.
BABO_SCRIPTS = {
    "BaboDiaMonitorScript": ["OnKeyDown", "BDConfig", "BaboKidnapEvent", "BaboNPCAnimating"],
    "BaboDialogueConfigMenu": ["NotificationKey"],
    "BaboKidnapEvenScript": ["KeyPress", "bCaptured", "BaboKidnapTiedUp", "BaboKidnapScenarioe",
                             "CenterMarkerPlayer"],
}
# Read by src/Surrender.cpp when the BaboDialogue 6.2 Acheron patch is installed.
BABO_CONTROLLER = "BaboSexControllerManager"
BABO_SUSPENDED_VAR = "AcheronSuspendedByUs"
# TDM and Grapple, read by src/LockOn.cpp.
TDM_MOD = "True Directional Movement - Modernized Third Person Gameplay"
TDM_SETTINGS = [
    os.path.join(MODS, "TAKEALOOK - MCM and INI", "MCM", "Settings", "TrueDirectionalMovement.ini"),
    os.path.join(MODS, TDM_MOD, "MCM", "Config", "TrueDirectionalMovement", "settings.ini"),
]
GRAPPLE_MOD = "Grapple"
GRAPPLE_NEEDLES = ["Hotkey", "ModifierEnabled", "TargetLockKey", "UpdateGlobals", "ApplySettings"]
# Fill Her Up names read by src/Deflate.cpp.
FHU_MOD = "Fill Her Up Baka Edition"
FHU_SCRIPTS = {
    "sr_infDeflateAbility": ["OnKeyDown", "OnKeyUp", "inflater", "config", "keydown"],
    "sr_inflateQuest": ["GetMostRecentInflationType", "inflateFaction", "SR_InflateOralFaction",
                        "inflaterAnimatingFaction", "slAnimatingFaction"],
    "sr_inflateConfig": ["defKey"],
}
# Acheron (surrender), read by src/Surrender.cpp.
ACHERON_MOD = "Acheron - Death Alternative"
ACHERON_SETTINGS = os.path.join(MODS, "TAKEALOOK - MCM and INI", "SKSE", "Acheron", "Settings.yaml")
REQUIRED_EXPORTS = {b"SKSEPlugin_Load", b"SKSEPlugin_Query", b"SKSEPlugin_Version"}

failures = []


def check(ok, what):
    print(("PASS " if ok else "FAIL ") + what)
    if not ok:
        failures.append(what)


def note(what):
    print("INFO " + what)


def active_profile():
    with open(os.path.join(MO2, "ModOrganizer.ini"), encoding="utf-8", errors="replace") as f:
        for line in f:
            if line.startswith("selected_profile="):
                value = line.split("=", 1)[1].strip()
                if value.startswith("@ByteArray(") and value.endswith(")"):
                    value = value[len("@ByteArray("):-1]
                return value
    raise SystemExit("selected_profile not found in ModOrganizer.ini")


def read_lines(path):
    with open(path, encoding="utf-8-sig") as f:
        return [line.rstrip("\r\n") for line in f]


def dll_exports(path):
    with open(path, "rb") as f:
        d = f.read()
    pe = struct.unpack_from("<I", d, 0x3C)[0]
    nsec = struct.unpack_from("<H", d, pe + 6)[0]
    optsz = struct.unpack_from("<H", d, pe + 20)[0]
    opt = pe + 24
    exp_rva = struct.unpack_from("<I", d, opt + 112)[0]
    secs = []
    for i in range(nsec):
        o = opt + optsz + i * 40
        vs, va, _, raw = struct.unpack_from("<IIII", d, o + 8)
        secs.append((va, max(vs, 1), raw))

    def off(rva):
        for va, vs, raw in secs:
            if va <= rva < va + vs:
                return rva - va + raw
        raise ValueError("rva outside sections")

    e = off(exp_rva)
    count = struct.unpack_from("<I", d, e + 24)[0]
    names = off(struct.unpack_from("<I", d, e + 32)[0])
    result = set()
    for i in range(count):
        p = off(struct.unpack_from("<I", d, names + 4 * i)[0])
        result.add(d[p:d.index(b"\0", p)])
    return result


def check_prompt_keys():
    """The control panel's prompt keys. A key outside 1-255 is ignored by the DLL, and two equal keys
    make one press fire two prompts."""
    path = os.path.join(MOD, "SKSE", "Plugins", "CIGAR.json")
    try:
        with open(path, encoding="utf-8") as f:
            keys = json.load(f).get("prompt", {}).get("keys")
    except (OSError, ValueError) as e:
        check(False, "CIGAR.json readable: %s" % e)
        return
    if keys is None:
        note("CIGAR.json has no prompt keys: the defaults 1-4 apply")
        return
    valid = isinstance(keys, list) and len(keys) == 4 and all(isinstance(k, int) and 0 < k < 256 for k in keys)
    check(valid, "CIGAR.json prompt keys are four keyboard scan codes: %s" % keys)
    if valid:
        check(len(set(keys)) == 4, "CIGAR.json prompt keys are distinct: %s" % keys)


def check_babo(modlist):
    """The BaboKey module reads BaboDialogue's scripts by name. A BaboDialogue update that renames
    any of these makes the prompt vanish without an error, so check the compiled scripts here."""
    enabled = [line[1:] for line in modlist if line.startswith("+")]
    if not any(os.path.isfile(os.path.join(MODS, folder, "BaboInteractiveDia.esp")) for folder in enabled):
        note("BaboDialogue absent: BaboKey module idles")
        return
    # Every enabled mod is searched, not only BaboDialogue: a script patch such as the
    # BaboDialogue 6.2 Acheron patch overrides the .pex the game actually runs.
    # modlist.txt lists the highest priority first, so the first hit wins the VFS.
    scripts = {}
    for name in list(BABO_SCRIPTS) + [BABO_CONTROLLER]:
        for folder in enabled:
            path = os.path.join(MODS, folder, "scripts", name + ".pex")
            if os.path.isfile(path):
                with open(path, "rb") as f:
                    scripts[name] = (folder, f.read())
                break
        if name in BABO_SCRIPTS:
            check(name in scripts, "BaboDialogue script present: %s.pex (%s)" % (
                name, scripts[name][0] if name in scripts else "-"))
    for name, needles in BABO_SCRIPTS.items():
        if name not in scripts:
            continue
        folder, data = scripts[name]
        missing = [n for n in needles if n.encode() not in data]
        check(not missing, "%s.pex (%s) still has %s%s" % (
            name, folder, ", ".join(needles), " - missing: " + ", ".join(missing) if missing else ""))
    # Surrender hides its prompt while the Acheron patch has Acheron suspended. Without the patch
    # BaboDialogue never suspends Acheron, so the variable's absence is only a note.
    if BABO_CONTROLLER in scripts:
        folder, data = scripts[BABO_CONTROLLER]
        if BABO_SUSPENDED_VAR.encode() in data:
            note("BaboDialogue Acheron patch active (%s): Surrender reads %s" % (folder, BABO_SUSPENDED_VAR))
        else:
            note("BaboDialogue Acheron patch absent (%s): BaboDialogue never suspends Acheron" % folder)


def ini_int(path, section, key):
    current = None
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            text = line.strip()
            if text.startswith("["):
                current = text[1:-1].strip().lower()
            elif current == section.lower() and "=" in text and not text.startswith(";"):
                name, value = text.split("=", 1)
                if name.strip().lower() == key.lower():
                    return int(value.strip())
    return None


def check_lockon(modlist):
    """LockOn presses TDM's lock key and Grapple's hotkey; a key it cannot press, or a missing API
    export, leaves the prompt absent without an error."""
    if "+" + TDM_MOD not in modlist:
        note("True Directional Movement absent: LockOn module idles")
        return
    dll = os.path.join(MODS, TDM_MOD, "SKSE", "Plugins", "TrueDirectionalMovement.dll")
    check(os.path.isfile(dll) and b"RequestPluginAPI" in dll_exports(dll), "TDM DLL exports RequestPluginAPI")
    tdm_key = None
    for path in TDM_SETTINGS:
        if os.path.isfile(path):
            tdm_key = ini_int(path, "Keys", "uTargetLockKey")
            if tdm_key is not None:
                note("TDM lock key %d (%s)" % (tdm_key, path))
                break
    check(tdm_key is not None and 0 <= tdm_key < 264, "TDM lock key is a keyboard or mouse key: %s" % tdm_key)

    if "+" + GRAPPLE_MOD not in modlist:
        note("Grapple absent: grapple prompt idles")
        return
    base = os.path.join(MODS, GRAPPLE_MOD)
    check(os.path.isfile(os.path.join(base, "SKSE", "Plugins", "FH_Grapple_Plugin.dll")), "Grapple DLL present")
    pex = os.path.join(base, "Scripts", "FH_Grapple.pex")
    if os.path.isfile(pex):
        with open(pex, "rb") as f:
            data = f.read()
        missing = [n for n in GRAPPLE_NEEDLES if n.encode() not in data]
        check(not missing, "FH_Grapple.pex still has %s%s" % (", ".join(GRAPPLE_NEEDLES), " - missing: " + ", ".join(missing) if missing else ""))
    else:
        check(False, "FH_Grapple.pex present")
    grapple_ini = os.path.join(base, "SKSE", "Plugins", "FH_Grapple_Plugin.ini")
    if os.path.isfile(grapple_ini):
        lock = ini_int(grapple_ini, "Keys", "targetLockKey")
        kb = ini_int(grapple_ini, "Keys", "kbKey")
        mod = ini_int(grapple_ini, "Keys", "kbModifier")
        note("Grapple keys: hotkey=%s modifier=%s lock=%s (CIGAR syncs lock to TDM's at load)" % (kb, mod, lock))
        # A new game starts Grapple's MCM with no hotkey; CIGAR restores the key from this INI, so
        # an unset key here leaves the grapple prompt off in every new game.
        check(kb is not None and 0 <= kb < 264,
              "Grapple INI kbKey is a keyboard/mouse key CIGAR can restore on a new game: %s" % kb)
    else:
        check(False, "Grapple INI present: %s" % grapple_ini)


def check_fhu(modlist):
    """Deflate forwards the prompt key to FHU's own key events; renamed scripts or properties
    leave the prompt absent without an error."""
    if "+" + FHU_MOD not in modlist:
        note("Fill Her Up absent: Deflate module idles")
        return
    for name, needles in FHU_SCRIPTS.items():
        path = os.path.join(MODS, FHU_MOD, "Scripts", name + ".pex")
        if not os.path.isfile(path):
            check(False, "FHU script present: %s.pex" % name)
            continue
        with open(path, "rb") as f:
            data = f.read()
        missing = [n for n in needles if n.encode() not in data]
        check(not missing, "%s.pex still has %s%s" % (name, ", ".join(needles), " - missing: " + ", ".join(missing) if missing else ""))


def yaml_scalar(path, key):
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            if line.startswith(key + ":"):
                return line.split(":", 1)[1].strip().strip('"')
    return None


def check_surrender(modlist):
    """Surrender presses Acheron's surrender key; Acheron ignores it when unset, behind a modifier,
    or with processing off, and nothing on screen says why."""
    if "+" + ACHERON_MOD not in modlist:
        note("Acheron absent: Surrender module idles")
        return
    dll = os.path.join(MODS, ACHERON_MOD, "SKSE", "Plugins", "Acheron.dll")
    check(os.path.isfile(dll), "Acheron DLL present")
    if os.path.isfile(dll):
        with open(dll, "rb") as f:
            data = f.read()
        check(b"iSurrenderKey" in data and b"iHunterPrideKeyMod" in data, "Acheron DLL still reads iSurrenderKey / iHunterPrideKeyMod")
        # Surrender sets and reads back the surrender key through these AcheronMCM natives.
        check(all(n in data for n in (b"SetSettingInt", b"GetSettingInt", b"AcheronMCM")),
              "Acheron DLL still registers AcheronMCM.SetSettingInt / GetSettingInt")
    if not os.path.isfile(ACHERON_SETTINGS):
        check(False, "Acheron settings present: " + ACHERON_SETTINGS)
        return
    key = yaml_scalar(ACHERON_SETTINGS, "iSurrenderKey")
    mod = yaml_scalar(ACHERON_SETTINGS, "iHunterPrideKeyMod")
    proc = yaml_scalar(ACHERON_SETTINGS, "ProcessingEnabled")
    check(key is not None and 0 <= int(key) < 264, "Acheron surrender key is a keyboard or mouse key: %s" % key)
    check(mod is None or int(mod) == -1, "Acheron modifier key unset: %s" % mod)
    check(proc in (None, "true"), "Acheron processing enabled: %s" % proc)
    note("Yamete Kudasai %s" % ("enabled: its surrender consequences apply" if any(
        l.startswith("+YameteKudasai") for l in modlist) else "absent: Acheron's own consequences apply"))


def main():
    profile = os.path.join(MO2, "profiles", active_profile())
    print("profile:", profile)

    # The DLL: present, identical to the build, exporting the SE+AE+VR entry triad.
    check(os.path.isfile(DLL), "DLL deployed: " + DLL)
    if os.path.isfile(DLL) and os.path.isfile(BUILT_DLL):
        with open(DLL, "rb") as a, open(BUILT_DLL, "rb") as b:
            check(a.read() == b.read(), "deployed DLL matches the build")
        exports = dll_exports(DLL)
        check(REQUIRED_EXPORTS <= exports, "DLL exports %s" % ", ".join(sorted(e.decode() for e in REQUIRED_EXPORTS)))

    # No ESP and no Papyrus scripts from the earlier builds.
    leftovers = []
    for root, _, files in os.walk(MOD):
        for name in files:
            low = name.lower()
            if low.endswith((".esp", ".esl", ".esm", ".pex", ".psc", ".seq")):
                leftovers.append(os.path.relpath(os.path.join(root, name), MOD))
    check(not leftovers, "no plugin or Papyrus leftovers in the mod folder%s" % (": " + ", ".join(leftovers) if leftovers else ""))
    check(not os.path.exists(os.path.join(MODS, "SI-Extensions")), "pre-rename SI-Extensions mod folder is gone")

    # Profile registration and priority.
    modlist = read_lines(os.path.join(profile, "modlist.txt"))
    check("+" + MOD_NAME in modlist, "mod enabled in modlist.txt")
    if "+" + MOD_NAME in modlist and ("+" + SI_MOD) in modlist:
        # modlist.txt lists the highest priority first.
        check(modlist.index("+" + MOD_NAME) < modlist.index("+" + SI_MOD),
              "CIGAR outranks Streamlined Interactions (settings override wins)")
    plugins = read_lines(os.path.join(profile, "plugins.txt"))
    stale = [p for p in plugins if p.lstrip("*") in ("CIGAR.esp", "SI-Extensions.esp")]
    check(not stale, "no CIGAR/SI-Extensions plugin listed in plugins.txt%s" % (": " + ", ".join(stale) if stale else ""))

    # Hard dependencies.
    for rel in (
        os.path.join("[NoDelete] 0007 SkyPrompt NEW", "SKSE", "Plugins", "SkyPrompt.dll"),
        os.path.join("Address Library for SKSE Plugins", "SKSE", "Plugins"),
    ):
        check(os.path.exists(os.path.join(MODS, rel)), "dependency present: " + rel)

    # Optional integrations: reported, never required.
    bis = os.path.isfile(os.path.join(MODS, "Bathing in Skyrim - Renewed", "Bathing in Skyrim.esp"))
    note("Bathing in Skyrim - Renewed %s" % ("installed: bathe module active" if bis else "absent: bathe module idles"))
    check_prompt_keys()
    check_babo(modlist)
    check_lockon(modlist)
    check_fhu(modlist)
    check_surrender(modlist)

    # Replaced SI modules must be off, or both prompts appear.
    check(os.path.isfile(SI_SETTINGS), "SI settings override exists")
    if os.path.isfile(SI_SETTINGS):
        with open(SI_SETTINGS, encoding="utf-8") as f:
            settings = json.load(f)
        for module, switch in REPLACED:
            check(settings["MCP"]["modules"][module][switch] is False, "SI %s.%s disabled" % (module, switch))
        check(settings["MCP"].get("preset") == 2, "SI preset is Power User (menu keeps module switches)")

    if failures:
        print("\n%d check(s) failed" % len(failures))
        sys.exit(1)
    print("\nall checks passed")


if __name__ == "__main__":
    main()
