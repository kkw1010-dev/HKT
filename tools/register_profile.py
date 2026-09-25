"""Register CIGAR in the active MO2 profile. Safe to re-run.

- modlist.txt: +CIGAR, and every release copy ("CIGAR <version>" or "CIGAR <version> Nexus", e.g. CIGAR 2.0.1) disabled so
  only the author build loads. An existing CIGAR entry keeps its place; a new one goes to the top
  (highest priority). CIGAR ships no files another mod provides, so its place does not matter.

Refuses to run while Mod Organizer is open, because MO2 rewrites these files from memory.
The file is backed up first when it changes.
"""
import datetime
import os
import re
import subprocess

MO2 = r"C:\TAKEALOOK"
AUTHOR_MOD = "CIGAR"
RELEASE_COPY = re.compile(r"^[+-]CIGAR \d+\.\d+\.\d+( Nexus)?$")


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


def enable_mod(rows):
    rows = ["-" + r[1:] if RELEASE_COPY.match(r) else r for r in rows]
    for i, row in enumerate(rows):
        if row in ("+" + AUTHOR_MOD, "-" + AUTHOR_MOD):
            rows[i] = "+" + AUTHOR_MOD
            return rows
    top = next((i for i, row in enumerate(rows) if not row.startswith("#")), len(rows))
    rows.insert(top, "+" + AUTHOR_MOD)
    return rows


def main():
    if mo2_running():
        raise SystemExit("Mod Organizer is running; close it first.")
    profile = os.path.join(MO2, "profiles", active_profile())
    stamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    rewrite(os.path.join(profile, "modlist.txt"), enable_mod, stamp)


if __name__ == "__main__":
    main()
