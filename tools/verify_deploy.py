"""Deployment checks for SI-Extensions. Exit 1 on any failure.

Encodes the failure modes that are silent in game (no error, no prompt):
the plugin or mod not enabled, a stale or unpatched .pex, a compile stub
shipped by accident (it would replace the real BiS script), SI's own Bathe
module still on (double prompts), or the override losing to SI's original.
"""
import json
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
MO2 = r"C:\TAKEALOOK"
MODS = os.path.join(MO2, "mods")
MOD = os.path.join(MODS, "SI-Extensions")
SI_MOD = "[NoDelete] 0008 StreamlinedInteractions"
PLUGIN = "SI-Extensions.esp"
BIS_PLUGIN = "Bathing in Skyrim.esp"
SI_SETTINGS = os.path.join(MOD, "SKSE", "Plugins", "StreamlinedInteractions", "settings.json")

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
            head = f.read(12)
        flags = struct.unpack_from("<I", head, 8)[0]
        check(head[:4] == b"TES4" and flags & 0x200, "plugin is ESL-flagged (flags=%#x)" % flags)
        with open(esp, "rb") as a, open(os.path.join(REPO, "plugin", PLUGIN), "rb") as b:
            check(a.read() == b.read(), "deployed plugin matches plugin/" + PLUGIN)

    # Profile registration and priority.
    modlist = read_lines(os.path.join(profile, "modlist.txt"))
    check("+SI-Extensions" in modlist, "mod enabled in modlist.txt")
    if "+SI-Extensions" in modlist and ("+" + SI_MOD) in modlist:
        # modlist.txt lists the highest priority first.
        check(modlist.index("+SI-Extensions") < modlist.index("+" + SI_MOD),
              "SI-Extensions outranks Streamlined Interactions (settings override wins)")
    plugins = read_lines(os.path.join(profile, "plugins.txt"))
    check("*" + PLUGIN in plugins, "plugin active in plugins.txt")
    check("*" + BIS_PLUGIN in plugins, "Bathing in Skyrim is active")

    # Compiled scripts.
    build = os.path.join(REPO, "build", "Scripts")
    for name in ("SIX_BatheQuestScript.pex", "SIX_BathePlayerAlias.pex"):
        deployed = os.path.join(MOD, "Scripts", name)
        check(os.path.isfile(deployed), "script deployed: " + name)
        if os.path.isfile(deployed):
            with open(deployed, "rb") as a, open(os.path.join(build, name), "rb") as b:
                data = a.read()
                check(data == b.read(), "deployed %s matches the build" % name)
            check(b"@SIX:" not in data, "no text placeholder left in " + name)
    with open(os.path.join(MOD, "Scripts", "SIX_BatheQuestScript.pex"), "rb") as f:
        pex = f.read()
    with open(os.path.join(REPO, "modules", "bathe", "strings.ko.json"), encoding="utf-8") as f:
        for text in json.load(f).values():
            check(text.encode("utf-8") in pex, "Korean text present: " + text.strip())

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
    ):
        check(os.path.isfile(os.path.join(MODS, rel)), "dependency present: " + rel)

    # SI's own Bathe module must be off, or both prompts appear in the water.
    check(os.path.isfile(SI_SETTINGS), "SI settings override exists")
    if os.path.isfile(SI_SETTINGS):
        with open(SI_SETTINGS, encoding="utf-8") as f:
            settings = json.load(f)
        check(settings["MCP"]["modules"]["Bathe"]["enabled"] is False, "SI Bathe module disabled")

    if failures:
        print("\n%d check(s) failed" % len(failures))
        sys.exit(1)
    print("\nall checks passed")


if __name__ == "__main__":
    main()
