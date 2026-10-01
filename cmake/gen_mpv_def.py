#!/usr/bin/env python3
# =============================================================================
# NeoFlux - gen_mpv_def.py
#
# Generate a MSVC-compatible module-definition (.def) file for libmpv-2.dll by
# parsing `dumpbin /exports` output. The mpv bundle ships a MinGW import library
# (libmpv.dll.a) that MSVC link.exe cannot consume; we rebuild an MSVC import
# library (mpv.lib) from the DLL exports instead.
#
# Usage:
#   gen_mpv_def.py <dumpbin.exe> <libmpv-2.dll> <out.def>
# =============================================================================

import subprocess
import sys


def main() -> int:
    if len(sys.argv) != 4:
        sys.stderr.write(
            "usage: gen_mpv_def.py <dumpbin> <dll> <out.def>\n")
        return 2

    dumpbin, dll_path, out_def = sys.argv[1], sys.argv[2], sys.argv[3]

    process = subprocess.Popen(
        [dumpbin, "/nologo", "/exports", dll_path],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    out_bytes, err_bytes = process.communicate()
    if process.returncode != 0:
        sys.stderr.write("dumpbin failed:\n" +
                         err_bytes.decode("ascii", errors="replace"))
        return process.returncode

    # dumpbin emits the OEM/ANSI codepage; only the ASCII export names matter.
    out = out_bytes.decode("ascii", errors="replace")

    names = []
    for line in out.splitlines():
        tokens = line.split()
        # An export row looks like:
        #   "          1    0 03BE5F10 mpv_create"
        # i.e. ordinal(decimal) hint(hex) rva(hex) name. Header prose lines are
        # rejected by requiring the first token to be a decimal ordinal and the
        # next two tokens to be hexadecimal.
        if len(tokens) < 4 or not tokens[0].isdigit():
            continue
        try:
            int(tokens[1], 16)
            int(tokens[2], 16)
        except ValueError:
            continue
        names.append(tokens[3])

    if not names:
        sys.stderr.write("no exports parsed from %s\n" % dll_path)
        return 1

    with open(out_def, "w", encoding="ascii", newline="\r\n") as handle:
        handle.write("LIBRARY libmpv-2.dll\n")
        handle.write("EXPORTS\n")
        for name in names:
            handle.write("    %s\n" % name)

    sys.stdout.write("wrote %s with %d exports\n" % (out_def, len(names)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
