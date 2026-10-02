"""The next Nexus description must not lose what the posted one carries: images and links.

Run before a description is posted (Build.ps1 -Package runs it). `description-live.bbcode` must first be
refreshed from the real page: the author edits the page by hand (feature GIFs, 2026-10-02), and a stale
copy here makes this check blind. Exit 1 when the next text has fewer [img] or [url] tags than the live one,
or drops an image the live one shows.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PAGE = os.path.join(os.path.dirname(HERE), "dist", "nexus-page")


def read(name):
    with open(os.path.join(PAGE, name), encoding="utf-8") as f:
        return f.read()


def main():
    live, nxt = read("description-live.bbcode"), read("description-next.bbcode")
    failures = []
    for tag in ("img", "url"):
        a, b = len(re.findall(r"\[%s[\]=]" % tag, live, re.I)), len(re.findall(r"\[%s[\]=]" % tag, nxt, re.I))
        ok = b >= a
        print("%s next has %d [%s] tag(s), live has %d" % ("PASS" if ok else "FAIL", b, tag, a))
        if not ok:
            failures.append(tag)
    lost = [u for u in re.findall(r"\[img\](.*?)\[/img\]", live, re.I | re.S) if u.strip() not in nxt]
    print("%s every image of the live description is in the next one%s" % ("PASS" if not lost else "FAIL", "" if not lost else ": missing " + ", ".join(lost)))
    return 1 if failures or lost else 0


if __name__ == "__main__":
    sys.exit(main())
