"""Assemble the installable CIGAR folder (and its .7z) from a player-facing build.

What a player installs is the mod folder and nothing else: the DLL, the default settings the
control panel writes back to, and a readme. Everything an author reads -- the source, docs/,
HANDOFF.md, tools/, the CMake files, the .pdb, MO2's meta.ini -- stays out.

Two editions:
- --standard: the CIGAR_RELEASE build (preset "dist"), "CIGAR <version>", Korean readme.
- --nexus: the CIGAR_NEXUS build (preset "nexus"), "CIGAR <version> Nexus", base game only,
  English readme plus a Korean one. The DLL must name no other mod (docs/035-nexus-edition.md).

Usage: python tools/make_release.py --standard|--nexus <path to CIGAR.dll>
"""
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
OUT_ROOT = os.path.join(os.path.expanduser("~"), "Downloads")
DEFAULTS = os.path.join(REPO, "dist", "CIGAR.json")
SEVEN_ZIP = r"C:\Program Files\7-Zip\7z.exe"

# Strings that give another mod away. The Nexus edition's DLL must carry none of them: a hit means an
# integration was compiled in. Base-game and Creation Club names (Skyrim.esm, Dragonborn.esm,
# ccqdrsse001-survivalmode.esl, ccBGSSSE001_FishingPoleKW) are allowed. SkyPrompt is the requirement
# and SKSE Menu Framework the optional panel, so both are allowed too.
NEXUS_FORBIDDEN = [
    "SexLab", "OStim", "zad_", "ValhallaCombat", "Valhalla Combat", "TrueDirectionalMovement", "TDM_API",
    "Acheron", "YameteKudasai", "Kudasai", "BaboInteractiveDia", "BaboDialogue", "sr_FillHerUp",
    "Fill Her Up", "FH_Grapple", "Private Needs", "PNO_", "Bathing in Skyrim", "mzin",
    "TorchesCandlelightLanterns", "Helmet Toggle", "SurvivalModeImproved", "Gourmet", "MAG_FoodType",
    "OCF_", "_SH_Alcohol", "bIsDodging", "bInIframe",
]


def version():
    with open(os.path.join(REPO, "CMakeLists.txt"), encoding="utf-8") as f:
        found = re.search(r"project\(CIGAR VERSION ([0-9.]+)", f.read())
    if not found:
        raise SystemExit("could not read the version out of CMakeLists.txt")
    return found.group(1)


def main():
    if len(sys.argv) != 3 or sys.argv[1] not in ("--standard", "--nexus"):
        raise SystemExit(__doc__)
    nexus = sys.argv[1] == "--nexus"
    dll = sys.argv[2]
    if not os.path.isfile(dll):
        raise SystemExit("no DLL at " + dll)

    # A build without the define would ship the author panel under a release name.
    with open(dll, "rb") as f:
        blob = f.read()
    marker = "조건: %s".encode("utf-8")
    if marker in blob:
        raise SystemExit(
            "this DLL still carries the author-side gate line, so it was built without "
            "CIGAR_RELEASE. Build with: tools\\Build.ps1 -Package (or -Nexus)"
        )
    if nexus:
        leaks = [s for s in NEXUS_FORBIDDEN if s.encode("utf-8") in blob]
        if leaks:
            raise SystemExit("the Nexus DLL still names other mods: " + ", ".join(leaks))
        if b"Nexus edition, base game only" not in blob:
            raise SystemExit("this DLL was not built with CIGAR_NEXUS. Build with: tools\\Build.ps1 -Nexus")
        print("PASS the Nexus DLL names none of %d other-mod strings" % len(NEXUS_FORBIDDEN))
    elif b"Nexus edition, base game only" in blob:
        raise SystemExit("this is the Nexus DLL; package it with --nexus")

    name = "CIGAR %s%s" % (version(), " Nexus" if nexus else "")
    out = os.path.join(OUT_ROOT, name)
    if os.path.isdir(out):
        shutil.rmtree(out)
    plugins = os.path.join(out, "SKSE", "Plugins")
    os.makedirs(plugins)

    shutil.copyfile(dll, os.path.join(plugins, "CIGAR.dll"))
    # Shipping the defaults keeps the panel's writes in the mod folder instead of MO2's overwrite.
    with open(DEFAULTS, encoding="utf-8") as f:
        settings = json.load(f)
    # Never ship this machine's own module switches; every module defaults on and idles when its
    # target mod is absent.
    settings["modules"] = {}
    settings["language"] = "auto"
    if nexus:
        # Keys for the modules and hotkey takeovers the Nexus edition does not have.
        for key in ("promptOnly", "eat", "needs"):
            settings.pop(key, None)
    with open(os.path.join(plugins, "CIGAR.json"), "w", encoding="utf-8", newline="\n") as f:
        json.dump(settings, f, ensure_ascii=False, indent=2)
        f.write("\n")
    if nexus:
        shutil.copyfile(os.path.join(REPO, "dist", "README-nexus.md"), os.path.join(out, "README.md"))
        shutil.copyfile(os.path.join(REPO, "dist", "README-nexus-ko.md"), os.path.join(out, "README-ko.md"))
    else:
        shutil.copyfile(os.path.join(REPO, "dist", "README-release.md"), os.path.join(out, "README.md"))

    print("release folder:", out)
    total = 0
    for root, _, files in os.walk(out):
        for f in sorted(files):
            path = os.path.join(root, f)
            size = os.path.getsize(path)
            total += size
            print("  %-42s %8d bytes" % (os.path.relpath(path, out), size))
    print("  %d bytes total" % total)
    print("  CIGAR.dll sha256 %s" % hashlib.sha256(blob).hexdigest())

    # The archive holds the folder itself, as the 2.0.0 archive did.
    archive = out + ".7z"
    if os.path.exists(archive):
        os.remove(archive)
    if os.path.isfile(SEVEN_ZIP):
        result = subprocess.run([SEVEN_ZIP, "a", "-t7z", archive, name], cwd=OUT_ROOT, capture_output=True, text=True)
        if result.returncode != 0:
            raise SystemExit("7-Zip failed:\n" + result.stdout + result.stderr)
        print("archive:", archive, os.path.getsize(archive), "bytes")
    else:
        print("WARN 7-Zip not found at %s; no archive made" % SEVEN_ZIP)


if __name__ == "__main__":
    main()
