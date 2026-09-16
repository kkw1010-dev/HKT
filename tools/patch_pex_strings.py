"""Replace ASCII placeholder literals in a compiled Skyrim .pex with UTF-8 text.

The Creation Kit's PapyrusCompiler.exe decodes .psc files in the system ANSI code
page (949 on this machine), which corrupts UTF-8 Korean literals and breaks the
parse. Sources therefore carry placeholders like "@SIX:bathe@", and this tool
rewrites them in the .pex string table after compiling. Every other section of a
.pex refers to strings by index, so changing a string's length is safe.

Fails (exit 1) when a mapped placeholder is missing from the string table, or
when any "@SIX:" placeholder is left unmapped.

usage: patch_pex_strings.py <file.pex> <strings.json>
"""
import json
import struct
import sys

MAGIC = 0xFA57C0DE
PLACEHOLDER_PREFIX = b"@SIX:"


def read_wstring(data, pos):
    (length,) = struct.unpack_from(">H", data, pos)
    pos += 2
    return data[pos:pos + length], pos + length


def locate_string_table(data):
    (magic,) = struct.unpack_from(">I", data, 0)
    if magic != MAGIC:
        raise ValueError("not a big-endian Skyrim .pex (magic %08X)" % magic)
    pos = 4 + 1 + 1 + 2 + 8  # magic, major, minor, game id, compile time
    for _ in range(3):  # source file name, user name, machine name
        _, pos = read_wstring(data, pos)
    return pos


def patch(pex_path, mapping_path):
    with open(pex_path, "rb") as f:
        data = f.read()
    with open(mapping_path, encoding="utf-8") as f:
        mapping = {k.encode("ascii"): v.encode("utf-8") for k, v in json.load(f).items()}

    table_pos = locate_string_table(data)
    (count,) = struct.unpack_from(">H", data, table_pos)
    pos = table_pos + 2
    strings = []
    for _ in range(count):
        s, pos = read_wstring(data, pos)
        strings.append(s)
    rest = data[pos:]

    hits = {k: 0 for k in mapping}
    out = []
    for s in strings:
        if s in mapping:
            hits[s] += 1
            s = mapping[s]
        elif PLACEHOLDER_PREFIX in s:
            raise SystemExit("unmapped placeholder in %s: %r" % (pex_path, s))
        if len(s) > 0xFFFF:
            raise SystemExit("string too long after patching: %r" % s[:40])
        out.append(struct.pack(">H", len(s)) + s)

    missing = [k.decode() for k, n in hits.items() if n == 0]
    if missing:
        raise SystemExit("placeholders not found in %s: %s" % (pex_path, ", ".join(missing)))

    patched = data[:table_pos] + struct.pack(">H", count) + b"".join(out) + rest
    with open(pex_path, "wb") as f:
        f.write(patched)
    for k, n in hits.items():
        print("patched %s -> %s" % (k.decode(), mapping[k].decode("utf-8")))


def verify(pex_path, mapping_path):
    """Re-read a patched .pex and confirm every mapped value is present."""
    with open(pex_path, "rb") as f:
        data = f.read()
    with open(mapping_path, encoding="utf-8") as f:
        values = [v.encode("utf-8") for v in json.load(f).values()]
    pos = locate_string_table(data)
    (count,) = struct.unpack_from(">H", data, pos)
    pos += 2
    strings = set()
    for _ in range(count):
        s, pos = read_wstring(data, pos)
        if PLACEHOLDER_PREFIX in s:
            raise SystemExit("placeholder left in %s: %r" % (pex_path, s))
        strings.add(s)
    absent = [v.decode("utf-8") for v in values if v not in strings]
    if absent:
        raise SystemExit("patched text missing from %s: %s" % (pex_path, absent))
    print("verified %s: %d string(s) present" % (pex_path, len(values)))


if __name__ == "__main__":
    if len(sys.argv) == 4 and sys.argv[1] == "--verify":
        verify(sys.argv[2], sys.argv[3])
    elif len(sys.argv) == 3:
        patch(sys.argv[1], sys.argv[2])
    else:
        raise SystemExit(__doc__)
