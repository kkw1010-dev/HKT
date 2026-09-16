"""Keep Streamlined Interactions' own modules from duplicating SI-Extensions ones.

SI-Extensions ships an override of SI's settings.json in its own mod folder, which
wins over the original because SI-Extensions has higher MO2 priority. SI writes
in-game setting changes back to that winning copy, so an existing override is
kept and only the replaced modules are forced off. The first deploy seeds the
override from SI's original file.
"""
import json
import os
import shutil

MODS = r"C:\TAKEALOOK\mods"
REL = os.path.join("SKSE", "Plugins", "StreamlinedInteractions", "settings.json")
ORIGINAL = os.path.join(MODS, "[NoDelete] 0008 StreamlinedInteractions", REL)
OVERRIDE = os.path.join(MODS, "SI-Extensions", REL)

# SI module -> the switch SI-Extensions turns off because it replaces that module.
REPLACED = {"Bathe": "enabled"}


def main():
    if not os.path.exists(OVERRIDE):
        os.makedirs(os.path.dirname(OVERRIDE), exist_ok=True)
        shutil.copyfile(ORIGINAL, OVERRIDE)
        print("seeded override from", ORIGINAL)

    with open(OVERRIDE, encoding="utf-8") as f:
        settings = json.load(f)
    modules = settings["MCP"]["modules"]
    changed = False
    for module, switch in REPLACED.items():
        if modules.get(module, {}).get(switch) is not False:
            modules.setdefault(module, {})[switch] = False
            changed = True
            print("disabled SI module", module)
    if changed:
        with open(OVERRIDE, "w", encoding="utf-8", newline="") as f:
            json.dump(settings, f, separators=(",", ":"))
    print("SI override OK:", OVERRIDE)


if __name__ == "__main__":
    main()
