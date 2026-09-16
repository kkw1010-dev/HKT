"""Deployment checks for CIGAR. Exit 1 on any failure.

Encodes the failure modes that are silent in game (no error, no prompt):
the plugin or mod not enabled, a stale or unpatched .pex, a compile stub
shipped by accident (it would replace the real BiS script), a replaced SI
module still on (double prompts), the override losing to SI's original, or
the pre-rename SI-Extensions build still loaded next to CIGAR.
"""
import glob
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
LEGACY_MOD = "SI-Extensions"
SI_MOD = "[NoDelete] 0008 StreamlinedInteractions"
PLUGIN = "CIGAR.esp"
BIS_PLUGIN = "Bathing in Skyrim.esp"
SI_SETTINGS = os.path.join(MOD, "SKSE", "Plugins", "StreamlinedInteractions", "settings.json")
REPLACED = [
    ("Bathe", "enabled"),
    ("DressActions", "enabled_water"),
    ("DressActions", "enabled_bed"),
    ("DressActions", "enabled_wardrobe"),
]

failures = []


def check(ok, what):
    print(("PASS " if ok else "FAIL ") + what)
    if not ok:
        failures.append(what)


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


def main():
    profile = os.path.join(MO2, "profiles", active_profile())
    print("profile:", profile)

    # Plugin file and header.
    esp = os.path.join(MOD, PLUGIN)
    check(os.path.isfile(esp), "plugin exists: " + esp)
    if os.path.isfile(esp):
        with open(esp, "rb") as f:
            data = f.read()
        flags = struct.unpack_from("<I", data, 8)[0]
        check(data[:4] == b"TES4" and flags & 0x200, "plugin is ESL-flagged (flags=%#x)" % flags)
        with open(os.path.join(REPO, "plugin", PLUGIN), "rb") as f:
            check(data == f.read(), "deployed plugin matches plugin/" + PLUGIN)

    # Profile registration and priority.
    modlist = read_lines(os.path.join(profile, "modlist.txt"))
    enabled = "+" + MOD_NAME
    check(enabled in modlist, "mod enabled in modlist.txt")
    if enabled in modlist and ("+" + SI_MOD) in modlist:
        # modlist.txt lists the highest priority first.
        check(modlist.index(enabled) < modlist.index("+" + SI_MOD),
              "CIGAR outranks Streamlined Interactions (settings override wins)")
    check("+" + LEGACY_MOD not in modlist, "pre-rename SI-Extensions mod is not enabled")
    plugins = read_lines(os.path.join(profile, "plugins.txt"))
    check("*" + PLUGIN in plugins, "plugin active in plugins.txt")
    check("*SI-Extensions.esp" not in plugins, "pre-rename SI-Extensions.esp is not active")
    check("*" + BIS_PLUGIN in plugins, "Bathing in Skyrim is active")

    # Compiled scripts: every built .pex deployed unchanged, nothing extra.
    build = os.path.join(REPO, "build", "Scripts")
    built = sorted(os.path.basename(p) for p in glob.glob(os.path.join(build, "*.pex")))
    deployed = sorted(os.path.basename(p) for p in glob.glob(os.path.join(MOD, "Scripts", "*.pex")))
    check(built and built == deployed, "deployed scripts are exactly the build: %s" % ", ".join(built))
    all_pex = b""
    for name in built:
        path = os.path.join(MOD, "Scripts", name)
        if not os.path.isfile(path):
            continue
        with open(path, "rb") as a, open(os.path.join(build, name), "rb") as b:
            data = a.read()
            check(data == b.read(), "deployed %s matches the build" % name)
        check(b"@CIGAR:" not in data, "no text placeholder left in " + name)
        all_pex += data
    with open(os.path.join(REPO, "strings.ko.json"), encoding="utf-8") as f:
        for text in json.load(f).values():
            check(text.encode("utf-8") in all_pex, "Korean text present: " + text.strip())

    for stub in os.listdir(os.path.join(REPO, "stubs")):
        pex_name = os.path.splitext(stub)[0] + ".pex"
        check(not os.path.exists(os.path.join(MOD, "Scripts", pex_name)),
              "compile stub not deployed: " + pex_name)

    # Dependencies the scripts call at runtime.
    for rel in (
        os.path.join("Bathing in Skyrim - Renewed", "Scripts", "mzinBatheQuest.pex"),
        os.path.join("Bathing in Skyrim - Renewed", "Scripts", "mzinAPI.pex"),
        os.path.join("[NoDelete] 0007 SkyPrompt NEW", "SKSE", "Plugins", "SkyPrompt.dll"),
        os.path.join("[NoDelete] 0007 SkyPrompt NEW", "Scripts", "SkyPrompt.pex"),
        os.path.join("powerofthree's Papyrus Extender", "SKSE", "Plugins", "po3_PapyrusExtender.dll"),
    ):
        check(os.path.isfile(os.path.join(MODS, rel)), "dependency present: " + rel)

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
