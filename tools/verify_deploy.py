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
