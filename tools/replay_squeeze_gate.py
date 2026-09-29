"""Replay Squeeze's blocked decision from CIGAR.log, without the game (docs/038).

Squeeze logs `trace near=... ahead=... side=... dz=... box=... refuse=... speed=... contact=... blocked=...`
whenever those rounded inputs change while an actor is near ahead. This reads them back, groups them
into contacts (one NPC in the box, not refused) and says, for each, whether the prompt would show
with the given thresholds, next to what the game logged.

  python tools/replay_squeeze_gate.py [CIGAR.log] [--speed 60] [--for 0.3]
"""
import argparse
import os
import re
import sys

DEFAULT_LOG = os.path.join(os.path.expanduser("~"), "OneDrive", "Documents", "My Games", "Skyrim Special Edition",
                           "SKSE", "CIGAR.log")
TRACE = re.compile(r"^\[(?P<t>[\d:]+)\] \[info\] \[Squeeze\] trace near=(?P<name>.+?) ahead=(?P<ahead>-?\d+) "
                   r"side=(?P<side>[+-]\d+) dz=(?P<dz>[+-]\d+) box=(?P<box>true|false) refuse=(?P<refuse>.+?) "
                   r"speed=(?P<speed>-?\d+) contact=(?P<contact>[\d.]+) blocked=(?P<blocked>true|false) "
                   r"shown=(?P<shown>true|false)$")
OFFER = re.compile(r"^\[(?P<t>[\d:]+)\] \[info\] \[Squeeze\] offer ")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("log", nargs="?", default=DEFAULT_LOG)
    parser.add_argument("--speed", type=float, default=60.0, help="blocked below this average speed (u/s)")
    parser.add_argument("--for", dest="hold", type=float, default=0.3, help="seconds of contact before blocked")
    args = parser.parse_args()
    sys.stdout.reconfigure(encoding="utf-8")

    contacts = []
    current = None
    offers = []
    refused = {}
    with open(args.log, encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.rstrip("\n")
            if OFFER.match(line):
                offers.append(OFFER.match(line)["t"])
                continue
            m = TRACE.match(line)
            if not m:
                continue
            if m["box"] == "true" and m["refuse"] != "-":
                refused[(m["name"], m["refuse"])] = refused.get((m["name"], m["refuse"]), 0) + 1
            in_contact = m["box"] == "true" and m["refuse"] == "-"
            if not in_contact:
                current = None
                continue
            if current is None or current["name"] != m["name"]:
                current = {"name": m["name"], "start": m["t"], "max_contact": 0.0, "min_speed": None,
                           "game_blocked": False, "replay_blocked": False, "lines": 0}
                contacts.append(current)
            contact = float(m["contact"])
            speed = float(m["speed"])
            current["lines"] += 1
            current["max_contact"] = max(current["max_contact"], contact)
            if speed >= 0:
                current["min_speed"] = speed if current["min_speed"] is None else min(current["min_speed"], speed)
            current["game_blocked"] |= m["blocked"] == "true"
            current["replay_blocked"] |= contact >= args.hold and 0 <= speed < args.speed

    print(f"{len(contacts)} contacts, {len(offers)} offers; replay with speed < {args.speed:g} for {args.hold:g} s")
    for c in contacts:
        lowest = "-" if c["min_speed"] is None else f"{c['min_speed']:.0f}"
        print(f"  {c['start']} {c['name']}: contact up to {c['max_contact']:.1f} s, lowest speed {lowest}, "
              f"game blocked {c['game_blocked']}, replay blocked {c['replay_blocked']} ({c['lines']} lines)")
    if refused:
        print("refused in the box:")
        for (name, why), n in sorted(refused.items(), key=lambda kv: -kv[1]):
            print(f"  {name}: {why} ({n} lines)")


if __name__ == "__main__":
    main()
