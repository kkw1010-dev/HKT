"""Register CIGAR in the active MO2 profile. Safe to re-run.

- modlist.txt: +CIGAR directly above Streamlined Interactions (so CIGAR's
  settings.json override wins); the pre-rename SI-Extensions entry is removed.
- plugins.txt / loadorder.txt: CIGAR.esp right after Bathing in Skyrim.esp;
  SI-Extensions.esp is removed.

Refuses to run while Mod Organizer is open, because MO2 rewrites these files
from memory. Each changed file is backed up first.
"""
import datetime
import os
import subprocess
import sys

MO2 = r"C:\TAKEALOOK"
SI_MOD = "[NoDelete] 0008 StreamlinedInteractions"
BIS_PLUGIN = "Bathing in Skyrim.esp"


def active_profile():
    with open(os.path.join(MO2, "ModOrganizer.ini"), encoding="utf-8", errors="replace") as f:
        for line in f:
            if line.startswith("selected_profile="):
                value = line.split("=", 1)[1].strip()
                if value.startswith("@ByteArray(") and value.endswith(")"):
                    value = value[len("@ByteArray("):-1]
                return value
    raise SystemExit("selected_profile not found in ModOrganizer.ini")


def mo2_running():
    # Scan the full CSV list and fail closed when it cannot be read.
    out = subprocess.run(["tasklist", "/FO", "CSV", "/NH"], capture_output=True).stdout
    names = {line.split(b",", 1)[0].strip(b'"').lower() for line in out.splitlines() if line}
    if not names:
        raise SystemExit("could not read the process list; refusing to touch the profile")
    return b"modorganizer.exe" in names


def rewrite(path, edit, stamp):
    with open(path, encoding="utf-8", newline="") as f:
        text = f.read()
    newline = "\r\n" if "\r\n" in text else "\n"
    rows = text.split(newline)
    new_rows = edit(list(rows))
    if new_rows == rows:
        print("unchanged:", path)
        return
    with open(path + ".bak_" + stamp + "_cigar", "w", encoding="utf-8", newline="") as f:
        f.write(text)
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(newline.join(new_rows))
    print("updated:", path)


def place_after(rows, anchor, line, remove):
    rows = [r for r in rows if r not in remove and r != line]
    rows.insert(rows.index(anchor) + 1, line)
    return rows


def place_before(rows, anchor, line, remove):
    rows = [r for r in rows if r not in remove and r != line]
    rows.insert(rows.index(anchor), line)
    return rows


def main():
    if mo2_running():
        raise SystemExit("Mod Organizer is running; close it first.")
    profile = os.path.join(MO2, "profiles", active_profile())
    stamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")

    rewrite(os.path.join(profile, "modlist.txt"),
            lambda rows: place_before(rows, "+" + SI_MOD, "+CIGAR",
                                      {"+SI-Extensions", "-SI-Extensions", "-CIGAR"}),
            stamp)
    rewrite(os.path.join(profile, "plugins.txt"),
            lambda rows: place_after(rows, "*" + BIS_PLUGIN, "*CIGAR.esp",
                                     {"*SI-Extensions.esp", "SI-Extensions.esp", "CIGAR.esp"}),
            stamp)
    rewrite(os.path.join(profile, "loadorder.txt"),
            lambda rows: place_after(rows, BIS_PLUGIN, "CIGAR.esp", {"SI-Extensions.esp"}),
            stamp)


if __name__ == "__main__":
    main()
