"""Source checks for CIGAR and the personal modules. Exit 1 on any failure; Build.ps1 runs it before
every build. Both failures below are silent in game: no error, only a prompt that never works.

1. Form lookups by a class without its own FORMTYPE. CommonLib's TESDataHandler::LookupForm<T> keeps
   a form only when form->Is(T::FORMTYPE). A base class such as TESBoundObject has no FORMTYPE of its
   own and inherits TESForm's kNone, so every lookup returns null (met in the r8b run, 2026-09-29).
   Use LookupForm(id, plugin) and then As<T>() for those.
2. Non-combat prompts must fill a ring (the user's rule, 2026-09-29). A single press acts on the first
   tap, so SkyPrompt's double-tap decline can never reach it. Every PromptSlot must be set to kHold or
   kHoldAndKeep unless it is listed in COMBAT below.
"""
import glob
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
SOURCES = [os.path.join(REPO, "src"), os.path.join(os.path.dirname(REPO), "CIGAR-Personal", "src")]
COMMONLIB = os.path.join(REPO, "lib", "commonlibsse-ng", "include", "RE")

# Prompts raised in or for combat keep the single press: a ring would cost time in a fight.
COMBAT = {
    ("WeaponSwap", "ranged"),
    ("WeaponSwap", "melee"),
    ("Execute", "execute"),
    ("Jujutsu", "jujutsu"),
    ("LockOn", "lock"),
    ("Grapple", "grapple"),
    ("Potion", "drink"),
    ("Helmet", "on"),  # offered only in combat
    ("WizardWarrior", "activate"),  # raised on weapon draw
}

failures = []


def check(ok, what):
    print(("PASS " if ok else "FAIL ") + what)
    if not ok:
        failures.append(what)


def read(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


def files(pattern):
    out = []
    for folder in SOURCES:
        out += glob.glob(os.path.join(folder, pattern))
    return sorted(out)


def has_own_formtype(cls):
    for header in glob.glob(os.path.join(COMMONLIB, "*", cls + ".h")):
        text = read(header)
        body = re.search(r"class\s+" + cls + r"\b[^;{]*\{(.*)", text, re.S)
        if body and re.search(r"FORMTYPE\s*=", body.group(1).split("\n\tclass ")[0]):
            return True
    return False


def lookups():
    seen = {}
    for path in files("*.cpp") + files("*.h"):
        for m in re.finditer(r"LookupForm(?:Raw)?<\s*RE::(\w+)\s*>", read(path)):
            seen.setdefault(m.group(1), []).append(os.path.basename(path))
    for cls, where in sorted(seen.items()):
        check(has_own_formtype(cls), f"LookupForm<{cls}> matches by its own FORMTYPE ({', '.join(sorted(set(where)))})")


def rings():
    for header in files("*.h"):
        module = os.path.splitext(os.path.basename(header))[0]
        slots = re.findall(r"PromptSlot\s+(\w+)\s*\{", read(header))
        if not slots:
            continue
        cpp = header[:-2] + ".cpp"
        text = read(header) + (read(cpp) if os.path.exists(cpp) else "")
        for slot in slots:
            if (module, slot) in COMBAT:
                check(True, f"{module}.{slot} is a combat prompt (single press allowed)")
                continue
            ring = re.search(r"\b" + slot + r"\.SetPromptType\(\s*SkyPromptAPI::kHold(AndKeep)?\s*\)", text)
            check(bool(ring), f"{module}.{slot} fills a ring (kHold or kHoldAndKeep)")


lookups()
rings()
if failures:
    print(f"{len(failures)} prompt-rule check(s) failed")
    sys.exit(1)
print("prompt rules: all passed")
