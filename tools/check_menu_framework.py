"""Control-panel checks for CIGAR. Exit 1 on any failure; Build.ps1 runs it after every build.

Every failure here is silent in game: the vendored SKSEMenuFramework.h skips or crashes on a
function the installed DLL does not export, the upstream static handle would stay null because
CIGAR.dll loads first, and Korean labels render as '?' unless the winning SKSEMenuFramework.ini
enables Korean with a font that has Hangul. SKSE Menu Framework itself stays optional: when it is
not enabled in the profile, that is reported and nothing else is checked against it.
"""
import glob
import os
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
MO2 = r"C:\TAKEALOOK"
MODS = os.path.join(MO2, "mods")
HEADER = os.path.join(REPO, "include", "SKSEMenuFramework", "SKSEMenuFramework.h")
PANEL = os.path.join(REPO, "src", "Panel.cpp")
FRAMEWORK_DLL = os.path.join("SKSE", "Plugins", "SKSEMenuFramework.dll")
FRAMEWORK_INI = os.path.join("SKSE", "Plugins", "SKSEMenuFramework.ini")

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


def enabled_mods_by_priority():
    """Enabled mods, highest priority (the one whose files win) first."""
    path = os.path.join(MO2, "profiles", active_profile(), "modlist.txt")
    with open(path, encoding="utf-8-sig") as f:
        return [line[1:].rstrip("\r\n") for line in f if line.startswith("+")]


def winner(mods, relpath):
    for mod in mods:
        candidate = os.path.join(MODS, mod, relpath)
        if os.path.isfile(candidate):
            return mod, candidate
    return None, None


def dll_exports(path):
    dumpbin = sorted(glob.glob(r"C:\Program Files*\Microsoft Visual Studio\*\*\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe"))
    if not dumpbin:
        raise SystemExit("dumpbin.exe not found (Visual Studio Build Tools)")
    out = subprocess.run([dumpbin[-1], "/nologo", "/exports", path], capture_output=True, text=True, check=True).stdout
    names = set()
    for line in out.splitlines():
        parts = line.split()
        # "ordinal hint RVA name", optionally followed by "= decorated-name"
        if len(parts) >= 4 and parts[0].isdigit() and re.fullmatch(r"[0-9A-F]{8}", parts[2]):
            names.add(parts[3])
    return names


def ini_values(path):
    values = {}
    with open(path, encoding="utf-8-sig", errors="replace") as f:
        for line in f:
            line = line.split(";", 1)[0].strip()
            if "=" in line:
                key, value = line.split("=", 1)
                values[key.strip().lower()] = value.strip()
    return values


def font_has_hangul(path):
    data = open(path, "rb").read()
    count = struct.unpack(">H", data[4:6])[0]
    tables = {data[12 + 16 * i:16 + 16 * i]: struct.unpack(">II", data[20 + 16 * i:28 + 16 * i]) for i in range(count)}
    if b"cmap" not in tables:
        return False
    base = tables[b"cmap"][0]
    for i in range(struct.unpack(">H", data[base + 2:base + 4])[0]):
        _, _, offset = struct.unpack(">HHI", data[base + 4 + 8 * i:base + 12 + 8 * i])
        sub = base + offset
        fmt = struct.unpack(">H", data[sub:sub + 2])[0]
        if fmt == 4:
            segs = struct.unpack(">H", data[sub + 6:sub + 8])[0] // 2
            ends = struct.unpack(">%dH" % segs, data[sub + 14:sub + 14 + 2 * segs])
            starts = struct.unpack(">%dH" % segs, data[sub + 16 + 2 * segs:sub + 16 + 4 * segs])
            if any(s <= 0xD7A3 and e >= 0xAC00 for s, e in zip(starts, ends)):
                return True
        elif fmt == 12:
            groups = struct.unpack(">I", data[sub + 12:sub + 16])[0]
            for g in range(groups):
                s, e, _ = struct.unpack(">III", data[sub + 16 + 12 * g:sub + 28 + 12 * g])
                if s <= 0xD7A3 and e >= 0xAC00:
                    return True
    return False


def main():
    header = open(HEADER, encoding="utf-8").read()
    check("static auto menuFramework = GetModuleHandle" not in header,
          "vendored header resolves the framework handle lazily (upstream static would be null)")
    check("#define menuFramework SKSEMenuFramework_Module()" in header, "lazy handle patch is present")
    wanted = set(re.findall(r'GetProcAddress\(menuFramework,\s*"(\w+)"', header))
    wanted |= set(re.findall(r'GetFunction<[^>]+>\("(\w+)"', header))
    panel = open(PANEL, encoding="utf-8").read()
    wanted |= set(re.findall(r'GetProcAddress\(framework,\s*"(\w+)"', panel))

    mods = enabled_mods_by_priority()
    mod, dll = winner(mods, FRAMEWORK_DLL)
    if not dll:
        note("SKSE Menu Framework is not enabled; CIGAR runs without a control panel")
        return
    note(f"SKSE Menu Framework from '{mod}'")
    exports = dll_exports(dll)
    missing = sorted(wanted - exports)
    check(not missing, f"all {len(wanted)} functions the header resolves are exported"
          + (f" (missing: {', '.join(missing[:10])})" if missing else ""))

    ini_mod, ini = winner(mods, FRAMEWORK_INI)
    check(ini is not None, "a SKSEMenuFramework.ini is present")
    if not ini:
        return
    values = ini_values(ini)
    check(values.get("enablekorean", "").lower() == "true",
          f"Korean glyphs enabled in the winning SKSEMenuFramework.ini ('{ini_mod}')")
    font_name = values.get("primaryfont", "")
    font_mod, font = winner(mods, os.path.join("SKSE", "Plugins", "fonts", font_name)) if font_name else (None, None)
    check(font is not None, f"primary font '{font_name}' exists")
    if font:
        check(font_has_hangul(font), f"primary font from '{font_mod}' has Hangul")


if __name__ == "__main__":
    main()
    if failures:
        print(f"{len(failures)} control-panel check(s) failed")
        sys.exit(1)
    print("control-panel checks OK")
