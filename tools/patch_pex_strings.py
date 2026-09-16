"""Replace ASCII placeholder literals in a compiled Skyrim .pex with UTF-8 text.

The Creation Kit's PapyrusCompiler.exe decodes .psc files in the system ANSI code
page (949 on this machine), which corrupts UTF-8 Korean literals and breaks the
parse. Sources therefore carry placeholders like "@CIGAR:bathe@", and this tool
rewrites them in the .pex string tables after compiling. Every other section of a
.pex refers to strings by index, so changing a string's length is safe.

Fails (exit 1) when any "@CIGAR:" placeholder is unmapped, or when a mapped
placeholder is used by none of the given files.

usage: patch_pex_strings.py <strings.json> <file.pex>...
       patch_pex_strings.py --verify <strings.json> <file.pex>...
"""
import json
import struct
import sys

MAGIC = 0xFA57C0DE
PLACEHOLDER_PREFIX = b"@CIGAR:"


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

    if not any(hits.values()):
        return hits

    patched = data[:table_pos] + struct.pack(">H", count) + b"".join(out) + rest
    with open(pex_path, "wb") as f:
        f.write(patched)
    used = [k.decode() for k, n in hits.items() if n]
    print("patched %s: %s" % (pex_path, ", ".join(used)))
    return hits


def verify(mapping_path, pex_paths):
    """Re-read patched files: no placeholder left, every mapped text present somewhere."""
    with open(mapping_path, encoding="utf-8") as f:
        values = [v.encode("utf-8") for v in json.load(f).values()]
    strings = set()
    for pex_path in pex_paths:
        with open(pex_path, "rb") as f:
            data = f.read()
        pos = locate_string_table(data)
        (count,) = struct.unpack_from(">H", data, pos)
        pos += 2
        for _ in range(count):
            s, pos = read_wstring(data, pos)
            if PLACEHOLDER_PREFIX in s:
                raise SystemExit("placeholder left in %s: %r" % (pex_path, s))
            strings.add(s)
    absent = [v.decode("utf-8") for v in values if v not in strings]
    if absent:
        raise SystemExit("patched text missing: %s" % absent)
    print("verified %d text(s) across %d file(s)" % (len(values), len(pex_paths)))


def main(argv):
    if len(argv) >= 3 and argv[0] == "--verify":
        verify(argv[1], argv[2:])
        return
    if len(argv) < 2 or argv[0].startswith("--"):
        raise SystemExit(__doc__)
    mapping_path, pex_paths = argv[0], argv[1:]
    total = {}
    for pex_path in pex_paths:
        for k, n in patch(pex_path, mapping_path).items():
            total[k] = total.get(k, 0) + n
    unused = [k.decode() for k, n in total.items() if n == 0]
    if unused:
        raise SystemExit("mapped placeholders used by no script: " + ", ".join(unused))


if __name__ == "__main__":
    main(sys.argv[1:])
