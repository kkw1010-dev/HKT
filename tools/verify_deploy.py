"""Deployment checks for CIGAR. Exit 1 on any failure.

Encodes the failure modes that are silent in game (no error, no prompt):
the mod not enabled, a stale DLL, leftovers of the earlier Papyrus/ESP builds
(which would run alongside the DLL and double every prompt), a replaced SI
module still on (double prompts), the override losing to SI's original, or a
missing hard dependency. Optional integrations (Bathing in Skyrim) are reported,
never required.
"""
import hashlib
import json
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
MO2 = r"C:\TAKEALOOK"
MODS = os.path.join(MO2, "mods")
MOD_NAME = "CIGAR"
RELEASE_MOD = "CIGAR 0.2.0"
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
    ("QuestActions", "enabled_track"),
    # One switch for SI's sit, lie down, lean, warm hands, chair eat/drink and tidy-up. Off at the
    # user's call (2026-09-21) ahead of CIGAR's own sit/lie; the rest is on the backlog.
    ("IdleActions", "enabled"),
    # ItemUse stays on because recharge and spellbook are not absorbed yet.
    ("ItemUse", "enabled_equip_weapon"),
    ("ItemUse", "enabled_equip_armor"),
    ("ItemUse", "enabled_hp_pot"),
    ("ItemUse", "enabled_stamina_potion"),
    ("ItemUse", "enabled_magicka_potion"),
    ("ItemUse", "enabled_curedisease_potion"),
    ("ItemUse", "enabled_curepoison_potion"),
    ("ItemUse", "enabled_waterbreath_potion"),
    ("ItemUse", "enabled_makelight"),
]
# Names src/BaboKey.cpp reads from BaboDialogue.
# SI features CIGAR has not absorbed. They must stay on: an absorption that was reverted in git
# leaves its switch off in the deployed settings, and the feature then vanishes from the game
# silently (this happened with pass time on 2026-09-21).
KEPT = [
    ("ItemUse", "enabled"),
    ("ItemUse", "enabled_recharge_weapon"),
    ("WeaponSwap", "enabled"),
    ("QuestActions", "enabled"),
]
BABO_SCRIPTS = {
    "BaboDiaMonitorScript": ["OnKeyDown", "BDConfig", "BaboKidnapEvent", "BaboNPCAnimating"],
    "BaboDialogueConfigMenu": ["NotificationKey"],
    "BaboKidnapEvenScript": ["KeyPress", "bCaptured", "BaboKidnapTiedUp", "BaboKidnapScenarioe",
                             "CenterMarkerPlayer"],
}
# Read by src/Surrender.cpp when the BaboDialogue 6.2 Acheron patch is installed.
BABO_CONTROLLER = "BaboSexControllerManager"
BABO_SUSPENDED_VAR = "AcheronSuspendedByUs"
# TDM, read by src/TDMLock.cpp for the LockOn and Grapple modules.
TDM_MOD = "True Directional Movement - Modernized Third Person Gameplay"
TDM_SETTINGS = [
    os.path.join(MODS, "TAKEALOOK - MCM and INI", "MCM", "Settings", "TrueDirectionalMovement.ini"),
    os.path.join(MODS, TDM_MOD, "MCM", "Config", "TrueDirectionalMovement", "settings.ini"),
]
# Grapple (a Patreon mod), read by src/Grapple.cpp.
GRAPPLE_MOD = "Grapple"
GRAPPLE_NEEDLES = ["Hotkey", "ModifierEnabled", "TargetLockKey", "UpdateGlobals", "ApplySettings"]
# Behaviour-graph files a module was confirmed against; see the file's own comment.
BEHAVIOUR_BASELINE = os.path.join(HERE, "behaviour_baseline.json")
# Fill Her Up names read by src/Deflate.cpp.
FHU_MOD = "Fill Her Up Baka Edition"
FHU_SCRIPTS = {
    "sr_infDeflateAbility": ["OnKeyDown", "OnKeyUp", "inflater", "config", "keydown"],
    "sr_inflateQuest": ["GetMostRecentInflationType", "inflateFaction", "SR_InflateOralFaction",
                        "inflaterAnimatingFaction", "slAnimatingFaction"],
    "sr_inflateConfig": ["defKey"],
}
# Private Needs - Orgasm names read and called by src/Needs.cpp (Papyrus names are case-insensitive).
PNO_MOD = "[SL+] Private Needs - Orgasm KOR"
PNO_SCRIPTS = {
    "pno_configscript": ["Universal_keyCode", "CheckNeeds_keyCode", "Urinate_KeyCode", "Excrete_KeyCode",
                         "Wetself_KeyCode", "Toilet_Keycode", "bladdertoggleVal", "boweltoggleVal",
                         "bladdercontent", "bowelcontent", "mapKey"],
    "pno_utilityscript": ["UrinateAndDefecate", "IsInSexScene"],
    "pno_qf_mainquest": ["bladder_lastlevel", "bowel_lastlevel", "bladderSize", "bowelSize"],
}
# Acheron (surrender), read by src/Surrender.cpp.
ACHERON_MOD = "Acheron - Death Alternative"
ACHERON_SETTINGS = os.path.join(MODS, "TAKEALOOK - MCM and INI", "SKSE", "Acheron", "Settings.yaml")
# Valhalla Combat (execution), read by src/Execute.cpp.
VALHALLA_MOD = "Valhalla Combat"
VALHALLA_SETTINGS = os.path.join(MO2, "overwrite", "MCM", "Settings", "ValhallaCombat.ini")
VALHALLA_HIDDEN_KEY = 0x66  # F15
# Every section data::loadKillMoveIni reads (spelling as in Valhalla, including "Ginat-2HW"). An empty
# one leaves Valhalla picking a kill move from an empty list for that race and weapon.
VALHALLA_KILLMOVE_SECTIONS = """Humanoid-Unarmed Humanoid-Dagger Humanoid-Sword Humanoid-Axe Humanoid-Mace
Humanoid-GreatSword Humanoid-2HW Humanoid-DW Humanoid-1HM-Back Humanoid-2HM-Back Humanoid-2HW-Back
Humanoid-Unarmed-Back Undead-1HM Undead-2HM Undead-2HW Falmer-1HM Falmer-2HM Falmer-2HW Spider-1HM
Spider-2HM Spider-2HW Gargoyle-1HM Gargoyle-2HM Gargoyle-2HW Giant-1HM Giant-2HM Ginat-2HW Bear-1HM
Bear-2HM Bear-2HW SabreCat-1HM SabreCat-2HM SabreCat-2HW Wolf-1HM Wolf-2HM Wolf-2HW Troll-1HM Troll-2HM
Troll-2HW Hagraven-1HM Hagraven-2HM Hagraven-2HW Spriggan-1HM Spriggan-2HM Spriggan-2HW Boar-1HM Boar-2HM
Boar-2HW Riekling-1HM Riekling-2HM Riekling-2HW AshHopper-1HM AshHopper-2HM AshHopper-2HW
DwarvenBallista-1HM DwarvenBallista-2HM DwarvenBallista-2HW SteamCenturion-1HM SteamCenturion-2HM
SteamCenturion-2HW ChaurusFlyer-1HM ChaurusFlyer-2HM ChaurusFlyer-2HW Lurker-1HM Lurker-2HM Lurker-2HW
Dragon-1HM Dragon-2HM Dragon-2HW""".split()
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


GAME_DATA = os.path.join(MO2, "Stock Game", "Data")
# FormIDs src/Eat.cpp hard-codes, with the EditorID each must carry. A mismatch leaves the eat
# prompt off (runtime warning) or excludes the wrong food, so it is checked against the plugins.
EAT_FORMS = [
    ("ccqdrsse001-survivalmode.esl", {
        0x826: "Survival_ModeEnabled", 0x81A: "Survival_HungerNeedValue",
        0x806: "Survival_HungerStage1Value", 0x802: "Survival_HungerStage2Value",
        0x803: "Survival_HungerStage3Value", 0x804: "Survival_HungerStage4Value",
        0x805: "Survival_HungerStage5Value", 0x8B0: "Survival_FoodRawMeat"}),
    ("Update.esm", {
        0x2EE1: "Survival_FoodRestoreHungerVerySmall", 0x2EE2: "Survival_FoodRestoreHungerSmall",
        0x2EE3: "Survival_FoodRestoreHungerMedium", 0x2EE4: "Survival_FoodRestoreHungerLarge"}),
    ("Skyrim.esm", {0xA0E56: "VendorItemFoodRaw"}),
    ("SurvivalModeImproved.esp", {0xF27: "SMI_HungerShouldBeEnabled"}),
    ("Gourmet.esp", {
        0x808: "MAG_FoodItemRaw", 0xA6A: "MAG_FoodTypePoisoned", 0x969: "MAG_FoodItemDrugs",
        0xA4D: "MAG_FoodTypeDrugs", 0xA4B: "MAG_FoodTypeAle", 0xA4C: "MAG_FoodTypeWine"}),
]


def plugin_editor_ids(path, wanted):
    """EditorIDs of the records in a plugin whose object index (low 24 bits) is in wanted."""
    import zlib
    with open(path, "rb") as f:
        d = f.read()
    found = {}
    pos = 0
    end = len(d)

    def scan(buf, start, stop):
        i = start
        while i + 24 <= stop:
            sig = buf[i:i + 4]
            size = struct.unpack_from("<I", buf, i + 4)[0]
            if sig == b"GRUP":
                scan(buf, i + 24, i + size)
                i += size
                continue
            flags, fid = struct.unpack_from("<II", buf, i + 8)
            body = buf[i + 24:i + 24 + size]
            i += 24 + size
            if (fid & 0xFFFFFF) not in wanted:
                continue
            if flags & 0x00040000:
                body = zlib.decompress(body[4:])
            j = 0
            while j + 6 <= len(body):
                ftype = body[j:j + 4]
                fsize = struct.unpack_from("<H", body, j + 4)[0]
                if ftype == b"EDID":
                    found[fid & 0xFFFFFF] = body[j + 6:j + 6 + fsize].rstrip(b"\x00").decode("ascii", "replace")
                    break
                j += 6 + fsize

    # Skip the TES4 header record.
    size = struct.unpack_from("<I", d, 4)[0]
    scan(d, 24 + size, end)
    return found


def find_plugin(modlist, name):
    for folder in [line[1:] for line in modlist if line.startswith("+")]:
        path = os.path.join(MODS, folder, name)
        if os.path.isfile(path):
            return path
    path = os.path.join(GAME_DATA, name)
    return path if os.path.isfile(path) else None


def check_eat(modlist):
    """The Eat module reads Survival Mode, SMI and Gourmet forms by FormID."""
    for plugin, forms in EAT_FORMS:
        path = find_plugin(modlist, plugin)
        if not path:
            note("%s absent: the Eat module skips its forms" % plugin)
            continue
        ids = plugin_editor_ids(path, set(forms))
        wrong = ["%06X=%s (expected %s)" % (fid, ids.get(fid, "missing"), edid)
                 for fid, edid in forms.items() if ids.get(fid) != edid]
        check(not wrong, "Eat forms in %s%s" % (plugin, ": " + ", ".join(wrong) if wrong else ""))


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
    """LockOn presses TDM's lock key; a key it cannot press, or a missing API export, leaves the
    prompt absent without an error."""
    if "+" + TDM_MOD not in modlist:
        note("True Directional Movement absent: LockOn module idles, Grapple loses its re-lock")
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


def check_grapple(modlist):
    """Grapple is a Patreon mod, so most setups do not have it and the module simply idles. When it
    is installed, CIGAR presses its hotkey, so a renamed script or an unset key leaves the prompt
    absent without an error."""
    if "+" + GRAPPLE_MOD not in modlist:
        note("Grapple absent: Grapple module idles")
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
        note("Grapple keys: hotkey=%s modifier=%s lock=%s (CIGAR syncs lock to TDM's at load when TDM is present)" % (kb, mod, lock))
        # A new game starts Grapple's MCM with no hotkey. With prompt-only on, CIGAR sets its hidden
        # key whatever the INI says (log 2026-09-24: "Grapple Hotkey -1 -> hidden key 100", usable),
        # and Grapple itself was seen writing kbKey=-1 back afterwards. With prompt-only off, CIGAR
        # restores the key from this INI, so an unset key would leave the prompt off in a new game.
        prompt_only = True
        cigar_json = os.path.join(MOD, "SKSE", "Plugins", "CIGAR.json")
        if os.path.isfile(cigar_json):
            with open(cigar_json, encoding="utf-8") as f:
                prompt_only = json.load(f).get("promptOnly", {}).get("grapple", {}).get("enabled", True)
        if prompt_only:
            note("Grapple INI kbKey=%s; prompt-only is on, so CIGAR sets its hidden key at load" % kb)
        else:
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


def check_pno(modlist):
    """Needs reads PNO's fill levels and keys and calls UrinateAndDefecate by name; a renamed
    variable leaves the prompts absent or the keys bound without an error."""
    if "+" + PNO_MOD not in modlist:
        note("Private Needs - Orgasm absent: Needs module idles")
        return
    for name, needles in PNO_SCRIPTS.items():
        path = os.path.join(MODS, PNO_MOD, "scripts", name + ".pex")
        if not os.path.isfile(path):
            check(False, "PNO script present: %s.pex" % name)
            continue
        with open(path, "rb") as f:
            data = f.read().lower()
        missing = [n for n in needles if n.lower().encode() not in data]
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


def ini_sections(path):
    """{section: [(key, value)]} of an INI file, keeping duplicate-free order."""
    sections, current = {}, None
    with open(path, encoding="utf-8-sig", errors="replace") as f:
        for line in f:
            text = line.strip()
            if not text or text[0] in ";#":
                continue
            if text.startswith("["):
                current = text[1:-1].strip()
                sections.setdefault(current, [])
            elif current is not None and "=" in text:
                name, value = text.split("=", 1)
                sections[current].append((name.strip(), value.strip()))
    return sections


def check_valhalla(modlist):
    """Execute presses Valhalla's execution key; Valhalla reads it from one INI, re-reads it through
    one Papyrus native, and plays a kill move from Killmoves.ini. Any of these changing makes the
    prompt do nothing, with nothing on screen."""
    if "+" + VALHALLA_MOD not in modlist:
        note("Valhalla Combat absent: Execute module idles")
        return
    root = os.path.join(MODS, VALHALLA_MOD)
    dll = os.path.join(root, "SKSE", "Plugins", "valhallaCombat.dll")
    check(os.path.isfile(dll), "Valhalla DLL present")
    if os.path.isfile(dll):
        with open(dll, "rb") as f:
            data = f.read()
        needles = [rb"Data\MCM\Settings\ValhallaCombat.ini", b"iExecutionKey", b"bStunToggle", b"OnConfigClose",
                   b"Data/SKSE/Plugins/ValhallaCombat/RaceMapping", b"RequestPluginAPI"]
        missing = [n.decode() for n in needles if n not in data]
        check(not missing, "Valhalla DLL still reads its MCM INI, iExecutionKey, the race map and OnConfigClose%s" % (
            " - missing: " + ", ".join(missing) if missing else ""))
        check(b"RequestPluginAPI" in dll_exports(dll), "Valhalla DLL exports RequestPluginAPI")
    pex = os.path.join(root, "scripts", "valhallaCombat_MCM.pex")
    check(os.path.isfile(pex), "valhallaCombat_MCM.pex present (settings reload)")
    if os.path.isfile(pex):
        with open(pex, "rb") as f:
            check(b"OnConfigClose" in f.read(), "valhallaCombat_MCM.pex declares OnConfigClose")
    races = os.path.join(root, "SKSE", "Plugins", "ValhallaCombat", "RaceMapping", "Vanilla.ini")
    check(os.path.isfile(races) and len(ini_sections(races).get("Humanoid", [])) > 0, "Valhalla race map has humanoid races")
    killmoves = os.path.join(root, "SKSE", "Plugins", "ValhallaCombat", "Killmoves.ini")
    if not os.path.isfile(killmoves):
        check(False, "Valhalla Killmoves.ini present")
    else:
        sections = ini_sections(killmoves)
        empty = []
        for name in VALHALLA_KILLMOVE_SECTIONS:
            usable = [v for _, v in sections.get(name, []) if "|" in v and find_plugin(modlist, v.split("|")[0].strip())]
            if not usable:
                empty.append(name)
        check(not empty, "every Valhalla kill-move section has an idle from a present plugin%s" % (
            " - empty: " + ", ".join(empty) if empty else ""))
    if os.path.isfile(VALHALLA_SETTINGS):
        stun = ini_int(VALHALLA_SETTINGS, "Stun", "bStunToggle")
        check(stun in (None, 1), "Valhalla stun enabled: %s" % ("default 1" if stun is None else stun))
        key = ini_int(VALHALLA_SETTINGS, "Stun", "iExecutionKey")
        note("Valhalla iExecutionKey %s (CIGAR sets F15 = %d at the next launch when prompt-only is on)" % (
            "unset (-1)" if key is None else key, VALHALLA_HIDDEN_KEY))
    else:
        note("Valhalla settings file absent: DLL defaults (stun on, no execution key) until CIGAR writes it")


def check_jujutsu(modlist):
    """Jujutsu plays these kill-move idles and keeps the victim alive by swallowing KillActor. If the
    idles move, or the victim clips stop ending with 2_KillActor, the design no longer holds."""
    idles = [("Skyrim.esm", {0x0F9958: "pa_KillMoveH2HComboA", 0x100EF8: "H2HKillMoveSlamA00"}),
             ("Update.esm", {0x820: "H2HKillMoveBodySlam", 0x821: "H2HKillMoveKneeThrow"})]
    if "+" + VALHALLA_MOD in modlist:
        idles.append(("ValhallaCombat.esp", {0xAA3A: "Val_H2HKillMoveKneeThrow", 0xAA3B: "Val_H2HKillMoveBodySlam",
                                             0xAA3C: "Val_H2HKillMoveComboA", 0xAA3D: "Val_H2HKillMoveSlamA"}))
    for plugin, forms in idles:
        path = find_plugin(modlist, plugin)
        if not path:
            check(False, "Jujutsu idle plugin present: " + plugin)
            continue
        ids = plugin_editor_ids(path, set(forms))
        wrong = ["%06X=%s (expected %s)" % (fid, ids.get(fid, "missing"), edid) for fid, edid in forms.items() if ids.get(fid) != edid]
        check(not wrong, "Jujutsu idles in %s%s" % (plugin, ": " + ", ".join(wrong) if wrong else ""))
    # The behaviour engine output that wins the VFS (modlist.txt lists the highest priority first).
    adsf = None
    for folder in [l[1:] for l in modlist if l.startswith("+")]:
        for name in ("meshes", "Meshes"):
            path = os.path.join(MODS, folder, name, "animationdatasinglefile.txt")
            if os.path.isfile(path):
                adsf = path
                break
        if adsf:
            break
    if not adsf:
        note("no animationdatasinglefile.txt in the mods: Jujutsu relies on the vanilla one")
        return
    with open(adsf, encoding="utf-8", errors="replace") as f:
        lines = [l.strip() for l in f]
    clips = ["NPCPaired_H2HKillMoveSlamA", "PairedNPC_H2HKillMoveComboA", "Paired_NPCH2HKillMoveBodySlam",
             "NPCPaired_H2HKillMoveKneeThrow"]
    missing = []
    for clip in clips:
        # A clip entry: name, clip id, speed, crop start, crop end, event count, then that many events.
        at = [i for i, l in enumerate(lines) if l == clip]
        events = []
        if at and at[0] + 5 < len(lines) and lines[at[0] + 5].isdigit():
            count = int(lines[at[0] + 5])
            events = lines[at[0] + 6:at[0] + 6 + count]
        if not any(e.startswith("2_KillActor:") for e in events):
            missing.append(clip)
    check(not missing, "victim clips end with 2_KillActor in %s%s" % (os.path.relpath(adsf, MODS),
          " - not found: " + ", ".join(missing) if missing else ""))


def winning_file(modlist, relative):
    """The copy of a Data-relative path that wins MO2's VFS, or None. modlist.txt lists the highest
    priority first, and the path is matched case-insensitively because mods spell it either way."""
    parts = relative.split("/")
    for folder in [l[1:] for l in modlist if l.startswith("+")]:
        path = os.path.join(MODS, folder)
        for part in parts:
            if not os.path.isdir(path):
                path = None
                break
            match = next((e for e in os.listdir(path) if e.lower() == part.lower()), None)
            if match is None:
                path = None
                break
            path = os.path.join(path, match)
        if path and os.path.isfile(path):
            return path
    return None


def check_rest(modlist):
    """Rest sends vanilla idle events to the player's graph; a behaviour build without them makes
    the prompt do nothing in game. Case-insensitive, as the game matches event names."""
    master = winning_file(modlist, "meshes/actors/character/behaviors/0_master.hkx")
    check(master is not None, "player behaviour graph 0_master.hkx found")
    if master is None:
        return
    with open(master, "rb") as f:
        data = f.read().lower()
    mt = winning_file(modlist, "meshes/actors/character/behaviors/mt_behavior.hkx")
    check(mt is not None, "player behaviour graph mt_behavior.hkx found")
    if mt is not None:
        with open(mt, "rb") as f:
            mt_data = f.read().lower()
        for event in ["IdleWarmHandsStanding", "IdleWarmHandsCrouched"]:
            check(event.lower().encode() in mt_data, "mt_behavior.hkx has Rest event %s" % event)
    for event in ["IdleSitCrossLeggedEnter", "IdleSitLedgeEnter", "IdleLayDownEnter", "IdleChairExitStart", "IdleStop",
                  "IdleWallLeanStart", "IdleLeanTableEnter", "IdleRailLeanEnter", "IdleRailLeanExit"]:
        check(event.lower().encode() in data, "0_master.hkx has Rest event %s" % event)


def md5_of(path):
    with open(path, "rb") as f:
        return hashlib.md5(f.read()).hexdigest()


def check_behaviour(modlist, accept=False):
    """Behaviour-graph files a module was confirmed against. Pandora rewrites its whole output on
    every run, and the rewrite is silent in game: the animation simply refuses to play. A run made
    for an unrelated mod would otherwise be found only by re-testing in game."""
    if not os.path.isfile(BEHAVIOUR_BASELINE):
        note("no behaviour baseline recorded (%s)" % BEHAVIOUR_BASELINE)
        return
    with open(BEHAVIOUR_BASELINE, encoding="utf-8") as f:
        baseline = json.load(f)
    changed = False
    for relative, expected in baseline.get("files", {}).items():
        path = winning_file(modlist, relative)
        if not path:
            note("%s not in any enabled mod: the vanilla BSA copy is used" % relative)
            continue
        actual = md5_of(path)
        if accept:
            if actual != expected["md5"]:
                print("  baseline %s: %s -> %s" % (relative, expected["md5"], actual))
                expected["md5"] = actual
                changed = True
            continue
        spare = os.path.join(REPO, expected["spare"]) if expected.get("spare") else None
        hint = ""
        if actual != expected["md5"]:
            lines = [
                "",
                "      %s was rewritten, probably by a Pandora run." % os.path.relpath(path, MODS),
                "      %s" % expected.get("why", ""),
                "      Restore: copy %s over %s" % (expected.get("spare", "(no spare kept)"), path),
                "      Or, once %s is re-tested in game: python tools/verify_deploy.py --accept-behaviour"
                % expected.get("module", "the module"),
            ]
            if spare and os.path.isfile(spare) and md5_of(spare) == expected["md5"]:
                lines.append("      (the spare in the repo matches the recorded hash)")
            hint = "\n".join(lines)
        check(actual == expected["md5"], "%s is the copy %s was confirmed on%s"
              % (relative, expected.get("module", "CIGAR"), hint))
    if accept and changed:
        with open(BEHAVIOUR_BASELINE, "w", encoding="utf-8", newline="\n") as f:
            json.dump(baseline, f, ensure_ascii=False, indent=2)
            f.write("\n")
        print("behaviour baseline updated")


def main():
    accept_behaviour = "--accept-behaviour" in sys.argv
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
    check("-" + RELEASE_MOD in modlist and "+" + RELEASE_MOD not in modlist,
          "release mod disabled in modlist.txt")
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
    check_grapple(modlist)
    check_eat(modlist)
    check_fhu(modlist)
    check_pno(modlist)
    check_surrender(modlist)
    check_valhalla(modlist)
    check_jujutsu(modlist)
    check_rest(modlist)
    check_behaviour(modlist, accept_behaviour)

    # Replaced SI modules must be off, or both prompts appear.
    check(os.path.isfile(SI_SETTINGS), "SI settings override exists")
    if os.path.isfile(SI_SETTINGS):
        with open(SI_SETTINGS, encoding="utf-8") as f:
            settings = json.load(f)
        for module, switch in REPLACED:
            check(settings["MCP"]["modules"][module][switch] is False, "SI %s.%s disabled" % (module, switch))
        for module, switch in KEPT:
            check(settings["MCP"]["modules"][module][switch] is True,
                  "SI %s.%s still on (not absorbed by CIGAR)" % (module, switch))
        check(settings["MCP"].get("preset") == 2, "SI preset is Power User (menu keeps module switches)")

    if failures:
        print("\n%d check(s) failed" % len(failures))
        sys.exit(1)
    print("\nall checks passed")


if __name__ == "__main__":
    main()
