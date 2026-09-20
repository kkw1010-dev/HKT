"""Assemble the installable CIGAR folder from a CIGAR_RELEASE build.

What a player installs is the mod folder and nothing else: the DLL, the default settings the
control panel writes back to, and a readme. Everything an author reads -- the source, docs/,
HANDOFF.md, tools/, the CMake files, the .pdb, MO2's meta.ini -- stays out.

Usage: python tools/make_release.py <path to the CIGAR_RELEASE CIGAR.dll>
"""
import hashlib
import json
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
OUT_ROOT = os.path.join(os.path.expanduser("~"), "Downloads")
README = os.path.join(REPO, "dist", "README-release.md")
DEFAULTS = os.path.join(REPO, "dist", "CIGAR.json")


def version():
    with open(os.path.join(REPO, "CMakeLists.txt"), encoding="utf-8") as f:
        found = re.search(r"project\(CIGAR VERSION ([0-9.]+)", f.read())
    if not found:
        raise SystemExit("could not read the version out of CMakeLists.txt")
    return found.group(1)


def main():
    if len(sys.argv) != 2:
        raise SystemExit(__doc__)
    dll = sys.argv[1]
    if not os.path.isfile(dll):
        raise SystemExit("no DLL at " + dll)

    # A build without the define would ship the author panel under a release name.
    with open(dll, "rb") as f:
        blob = f.read()
    marker = "조건: %s".encode("utf-8")
    if marker in blob:
        raise SystemExit(
            "this DLL still carries the author-side gate line, so it was built without "
            "CIGAR_RELEASE. Build with: cmake --preset dist && cmake --build --preset dist"
        )

    name = "CIGAR %s" % version()
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
    with open(os.path.join(plugins, "CIGAR.json"), "w", encoding="utf-8", newline="\n") as f:
        json.dump(settings, f, ensure_ascii=False, indent=2)
        f.write("\n")
    shutil.copyfile(README, os.path.join(out, "README.md"))

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


if __name__ == "__main__":
    main()
