"""Build (and optionally deploy) the OAR test clips of docs/038 stage 0b: "CIGAR Push Test".

Author build only; never packaged. Each submod replaces Offset Movement Animation's
GPMAOffsetAnimation.hkx while iGPMAAnimationType holds its number (PushProbe sets it, then sends
OffsetGPMA), the way CIGAR - Helmet Motions does for the helmet.

  python tools/push_test_assets.py              build under build/push-test and check every clip
  python tools/push_test_assets.py --deploy     also copy it into mods\\CIGAR (Skyrim and MO2 closed)
  python tools/push_test_assets.py --remove     take it out of mods\\CIGAR again

Sources: the vanilla clips from an extracted Skyrim - Animations.bsa, and EVG Animated Traversal's
Squeeze from the user's own install. The Squeeze copy has its 86 annotations stripped with hkanno: they
carry Animation Motion Revolution's `animmotion` root motion (127 units forward) and the furniture
events IdleChairSitting / IdleStop / IdleFurnitureExit, which would move or unseat a walking player.
"""
import json
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
OUT = os.path.join(REPO, "build", "push-test")
MOD_NAME = "CIGAR Push Test"
TREE = os.path.join("meshes", "OpenAnimationReplacer", MOD_NAME)
CLIP_PATH = os.path.join("Actors", "Character", "Animations", "GPMAOffsetAnimation.hkx")
DEPLOY = r"C:\TAKEALOOK\mods\CIGAR"
VANILLA = r"C:\TAKEALOOK\_staging\vanilla-animations\meshes\actors\character\animations"
EVG = (r"C:\TAKEALOOK\mods\EVG Animated Traversal\meshes\actors\character\animations\OpenAnimationReplacer"
       r"\EVG Animated Traversal\Squeeze\mt_leverfloorpull.hkx")
HKANNO_DIR = r"C:\TAKEALOOK\TOOLS\hkanno64-001"
WORK = os.path.join(os.environ.get("SystemDrive", "C:") + os.sep, "hkanno_tmp")

# value, submod, source file, strip annotations; must match PushProbe's kGestures.
CLIPS = [
    (3801, "3801 activatedoor", os.path.join(VANILLA, "mt_activatedoor.hkx"), False),
    (3802, "3802 blockbash", os.path.join(VANILLA, "shd_blockbash.hkx"), False),
    (3803, "3803 idletake", os.path.join(VANILLA, "idletake.hkx"), False),
    (3804, "3804 idlegive", os.path.join(VANILLA, "idlegive.hkx"), False),
    (3805, "3805 evg squeeze", EVG, True),
    (3806, "3806 evg squeeze", EVG, True),
]


def hkanno(*args):
    """Runs hkanno from its own folder (HCT reads win32.hko there) and restores the files it overwrites."""
    keep = {}
    for name in ("win32.hkx", "anno.txt"):
        path = os.path.join(HKANNO_DIR, name)
        if os.path.isfile(path):
            keep[path] = open(path, "rb").read()
    try:
        win32 = os.path.join(HKANNO_DIR, "win32.hkx")
        if os.path.isfile(win32):
            os.remove(win32)
        run = subprocess.run([os.path.join(HKANNO_DIR, "hkanno64.exe"), *args], cwd=HKANNO_DIR,
                             capture_output=True, text=True, check=False)
        return run.returncode, (run.stdout + run.stderr).strip()
    finally:
        for path, data in keep.items():
            with open(path, "wb") as f:
                f.write(data)


def count_annotations(path):
    os.makedirs(WORK, exist_ok=True)
    short = os.path.join(WORK, "check.hkx")
    shutil.copyfile(path, short)
    dump = os.path.join(WORK, "check.txt")
    if os.path.isfile(dump):
        os.remove(dump)
    code, text = hkanno("dump", "-o", dump, short)
    if not os.path.isfile(dump):
        sys.exit("hkanno could not read %s (exit %s): %s" % (path, code, text[-300:]))
    for line in open(dump, encoding="utf-8", errors="replace"):
        if line.startswith("# numAnnotations:"):
            return int(line.split(":")[1])
    sys.exit("hkanno printed no annotation count for " + path)


def strip_annotations(src, dst):
    os.makedirs(WORK, exist_ok=True)
    short = os.path.join(WORK, "in.hkx")
    empty = os.path.join(WORK, "empty.txt")
    shutil.copyfile(src, short)
    before = os.path.getsize(short)
    with open(empty, "w", encoding="ascii") as f:
        f.write("# no annotations\n")
    # In place, as the tool's own update.bat does: the same path for input and output.
    code, text = hkanno("update", "-i", empty, short, short)
    if code != 0 or not os.path.isfile(short) or os.path.getsize(short) == 0:
        sys.exit("hkanno could not rewrite %s (exit %s, %d -> %d bytes): %s" % (
            src, code, before, os.path.getsize(short) if os.path.isfile(short) else 0, text[-300:]))
    shutil.copyfile(short, dst)


def build():
    if os.path.isdir(OUT):
        shutil.rmtree(OUT)
    root = os.path.join(OUT, TREE)
    os.makedirs(root)
    with open(os.path.join(root, "config.json"), "w", encoding="utf-8", newline="\n") as f:
        json.dump({"name": MOD_NAME, "author": "CIGAR (test only; vanilla and EVG Animated Traversal clips)",
                   "description": "docs/038 stage 0b: push-through clip candidates for Offset Movement Animation, "
                                  "keyed on iGPMAAnimationType 3801-3806. Author build only, never shipped."},
                  f, indent=4)
        f.write("\n")
    for value, submod, src, strip in CLIPS:
        if not os.path.isfile(src):
            sys.exit("missing source clip: " + src)
        folder = os.path.join(root, submod)
        clip = os.path.join(folder, CLIP_PATH)
        os.makedirs(os.path.dirname(clip))
        if strip:
            strip_annotations(src, clip)
        else:
            shutil.copyfile(src, clip)
        left = count_annotations(clip)
        if left:
            sys.exit("%s still has %d annotations" % (clip, left))
        with open(os.path.join(folder, "config.json"), "w", encoding="utf-8", newline="\n") as f:
            json.dump({"name": submod, "priority": 750000 + value, "conditions": [{
                "condition": "CompareValues", "requiredVersion": "1.0.0.0",
                "Value A": {"graphVariable": "iGPMAAnimationType", "graphVariableType": "Int"},
                "Comparison": "==", "Value B": {"value": float(value)}}]}, f, indent=4)
            f.write("\n")
        print("PASS %s: %s -> GPMAOffsetAnimation.hkx, %d bytes, 0 annotations%s" % (
            submod, os.path.basename(src), os.path.getsize(clip), " (stripped)" if strip else ""))
    shutil.rmtree(WORK, ignore_errors=True)
    print("built:", root)


def running():
    out = subprocess.run(["tasklist"], capture_output=True, text=True).stdout.lower()
    return [name for name in ("skyrimse.exe", "modorganizer.exe") if name in out]


def main():
    if "--remove" in sys.argv:
        target = os.path.join(DEPLOY, TREE)
        if running():
            sys.exit("close %s first" % ", ".join(running()))
        shutil.rmtree(target, ignore_errors=True)
        print("removed:", target)
        return
    build()
    if "--deploy" in sys.argv:
        if running():
            sys.exit("close %s first" % ", ".join(running()))
        target = os.path.join(DEPLOY, TREE)
        shutil.rmtree(target, ignore_errors=True)
        shutil.copytree(os.path.join(OUT, TREE), target)
        print("deployed:", target)


if __name__ == "__main__":
    main()
