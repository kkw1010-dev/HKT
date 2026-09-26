"""Assemble the installable CIGAR folder (and its .7z) from a player-facing build.

What a player installs is the mod folder and nothing else: the DLL, the default settings the
control panel writes back to, the readmes and the license files. Everything an author reads -- the
source, docs/, HANDOFF.md, tools/, the CMake files, the .pdb, MO2's meta.ini -- stays out.

One package serves Nexus and every other host: the CIGAR_RELEASE build (preset "dist"), "CIGAR
<version>", an English README.md and a Korean README-ko.md. Other mods are integrated at runtime
only, so the package must hold no file of theirs; the folder is checked against PACKAGE_FILES.

Usage: python tools/make_release.py <path to CIGAR.dll>
"""
import glob
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

# Everything the package may hold. Anything else (a mesh, a script, another mod's file) fails the
# build: CIGAR integrates other mods at runtime and ships none of their files (docs/035).
PACKAGE_FILES = {
    "SKSE/Plugins/CIGAR.dll", "SKSE/Plugins/CIGAR.json", "README.md", "README-ko.md", "LICENSE.txt",
    "THIRD-PARTY-NOTICES.txt",
}


def version():
    with open(os.path.join(REPO, "CMakeLists.txt"), encoding="utf-8") as f:
        found = re.search(r"project\(CIGAR VERSION ([0-9.]+)", f.read())
    if not found:
        raise SystemExit("could not read the version out of CMakeLists.txt")
    return found.group(1)


def notices(dll):
    """THIRD-PARTY-NOTICES.txt from the build's own license files, so it cannot go stale.

    CommonLibSSE-NG is GPL-3.0-or-later (the package's LICENSE.txt); the rest are the notices the
    permissive licenses of the code compiled into CIGAR.dll ask to be kept: CommonLibSSE-NG's
    bundled legacy notices, the vendored API headers, and every vcpkg package of this build.
    """
    nl = "\n"
    rule = "=" * 78
    parts = [
        "CIGAR is licensed under the GNU General Public License v3.0 or later (LICENSE.txt)." + nl,
        "Source: https://github.com/kkw1010-dev/HKT" + nl + nl,
        "CIGAR.dll contains code from the projects below. Their notices follow." + nl,
        "CommonLibSSE-NG (alandtse/CommonLibVR, branch ng): GPL-3.0-or-later, see LICENSE.txt." + nl,
    ]
    sections = []
    for path in sorted(glob.glob(os.path.join(REPO, "lib", "commonlibsse-ng", "licenses", "LICENSE-*.txt"))):
        sections.append(("CommonLibSSE-NG bundled notice: " + os.path.basename(path), path))
    for path in sorted(glob.glob(os.path.join(REPO, "include", "*", "LICENSE"))):
        sections.append(("API header: " + os.path.basename(os.path.dirname(path)), path))
    share = os.path.join(os.path.dirname(os.path.abspath(dll)), "vcpkg_installed", "x64-windows-static-md", "share")
    found = sorted(glob.glob(os.path.join(share, "*", "copyright")))
    if not found:
        raise SystemExit("no vcpkg copyright files under " + share + "; build with tools/Build.ps1 first")
    for path in found:
        sections.append(("vcpkg package: " + os.path.basename(os.path.dirname(path)), path))
    for title, path in sections:
        with open(path, encoding="utf-8", errors="replace") as f:
            parts.append(nl + rule + nl + title + nl + rule + nl + nl + f.read().strip() + nl)
    return "".join(parts)


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
            "CIGAR_RELEASE. Build with: tools\\Build.ps1 -Package"
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
    settings["language"] = "auto"
    with open(os.path.join(plugins, "CIGAR.json"), "w", encoding="utf-8", newline="\n") as f:
        json.dump(settings, f, ensure_ascii=False, indent=2)
        f.write("\n")
    shutil.copyfile(os.path.join(REPO, "dist", "README-en.md"), os.path.join(out, "README.md"))
    shutil.copyfile(os.path.join(REPO, "dist", "README-ko.md"), os.path.join(out, "README-ko.md"))

    # GPL-3.0 asks for the license text with every copy, and the permissive licenses for their notices.
    shutil.copyfile(os.path.join(REPO, "LICENSE"), os.path.join(out, "LICENSE.txt"))
    with open(os.path.join(out, "THIRD-PARTY-NOTICES.txt"), "w", encoding="utf-8", newline="\n") as f:
        f.write(notices(dll))

    found = set()
    for root, _, files in os.walk(out):
        for f in files:
            found.add(os.path.relpath(os.path.join(root, f), out).replace(os.sep, "/"))
    if found != PACKAGE_FILES:
        raise SystemExit("the package does not hold exactly its own files: extra %s, missing %s"
                         % (sorted(found - PACKAGE_FILES), sorted(PACKAGE_FILES - found)))
    print("PASS the package holds only CIGAR's own %d files" % len(PACKAGE_FILES))

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

    # The archive holds the folder's contents, SKSE/ at its root, so a mod manager installs it as is.
    # Up to 2.0.1 it held the folder itself, which Vortex would unpack as Data/CIGAR <version>/SKSE.
    archive = out + ".7z"
    if os.path.exists(archive):
        os.remove(archive)
    if os.path.isfile(SEVEN_ZIP):
        result = subprocess.run([SEVEN_ZIP, "a", "-t7z", archive, "*"], cwd=out, capture_output=True, text=True)
        if result.returncode != 0:
            raise SystemExit("7-Zip failed:\n" + result.stdout + result.stderr)
        listing = subprocess.run([SEVEN_ZIP, "l", "-slt", archive], capture_output=True, text=True).stdout
        # -slt prints the archive's own path first; the entries follow the "----------" line.
        entries = listing.split("----------", 1)[1]
        roots = {line[7:].replace("/", "\\").split("\\")[0] for line in entries.splitlines() if line.startswith("Path = ")}
        if roots != {"SKSE", "README.md", "README-ko.md", "LICENSE.txt", "THIRD-PARTY-NOTICES.txt"}:
            raise SystemExit("the archive root is %s; SKSE and the documents belong at its root" % sorted(roots))
        print("PASS the archive has SKSE at its root")
        print("archive:", archive, os.path.getsize(archive), "bytes")
    else:
        print("WARN 7-Zip not found at %s; no archive made" % SEVEN_ZIP)


if __name__ == "__main__":
    main()
