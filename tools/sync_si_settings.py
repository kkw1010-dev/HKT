"""Keep Streamlined Interactions' own modules from duplicating CIGAR ones.

CIGAR ships an override of SI's settings.json in its own mod folder, which
wins over the original because CIGAR has higher MO2 priority. SI writes
in-game setting changes back to that winning copy, so an existing override is
kept and only the replaced modules are forced off. The first deploy seeds the
override from SI's original file.

SI's non-Power-User presets re-apply their own module switches when its menu is
opened (observed: opening the menu under the Interactive preset turned Bathe back
on), so the override is also pinned to the Power User preset.
"""
import json
import os
import shutil

MODS = r"C:\TAKEALOOK\mods"
REL = os.path.join("SKSE", "Plugins", "StreamlinedInteractions", "settings.json")
ORIGINAL = os.path.join(MODS, "[NoDelete] 0008 StreamlinedInteractions", REL)
OVERRIDE = os.path.join(MODS, "CIGAR", REL)

# (SI module, switch) pairs CIGAR turns off because it replaces them.
REPLACED = [
    ("Bathe", "enabled"),
    ("DressActions", "enabled_water"),
    ("DressActions", "enabled_bed"),
    ("DressActions", "enabled_wardrobe"),
    ("QuestActions", "enabled_track"),
    # One switch for SI's sit, lie down, lean, warm hands, chair eat/drink and tidy-up. Off at the
    # user's call (2026-09-21) ahead of CIGAR's own sit/lie; the rest is on the backlog.
    ("IdleActions", "enabled"),
    # Every ItemUse action SI turns on by default is CIGAR's now (spellbook equip was off by the
    # user's choice); ItemUse.enabled itself is left alone.
    ("ItemUse", "enabled_equip_weapon"),
    ("ItemUse", "enabled_equip_armor"),
    ("ItemUse", "enabled_hp_pot"),
    ("ItemUse", "enabled_stamina_potion"),
    ("ItemUse", "enabled_magicka_potion"),
    ("ItemUse", "enabled_curedisease_potion"),
    ("ItemUse", "enabled_curepoison_potion"),
    ("ItemUse", "enabled_waterbreath_potion"),
    ("ItemUse", "enabled_makelight"),
    ("ItemUse", "enabled_recharge_weapon"),
]
# SI presets: 0 Default, 1 Interactive, 2 PowerUser.
POWER_USER_PRESET = 2


def main():
    if not os.path.exists(OVERRIDE):
        os.makedirs(os.path.dirname(OVERRIDE), exist_ok=True)
        shutil.copyfile(ORIGINAL, OVERRIDE)
        print("seeded override from", ORIGINAL)

    with open(OVERRIDE, encoding="utf-8") as f:
        settings = json.load(f)
    modules = settings["MCP"]["modules"]
    changed = False
    if settings["MCP"].get("preset") != POWER_USER_PRESET:
        print("SI preset %s -> %s (Power User)" % (settings["MCP"].get("preset"), POWER_USER_PRESET))
        settings["MCP"]["preset"] = POWER_USER_PRESET
        changed = True
    for module, switch in REPLACED:
        if modules.get(module, {}).get(switch) is not False:
            modules.setdefault(module, {})[switch] = False
            changed = True
            print("disabled SI %s.%s" % (module, switch))
    if changed:
        with open(OVERRIDE, "w", encoding="utf-8", newline="") as f:
            json.dump(settings, f, separators=(",", ":"))
    print("SI override OK:", OVERRIDE)


if __name__ == "__main__":
    main()
