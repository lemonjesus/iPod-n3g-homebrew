#!/usr/bin/env python3
"""Write an eApp Manifest.plist (XML plist) for an unsigned, unencrypted game.

Manifest schema:

  root dict   BuildIdentifier(str) GUID(str) HeapSize(int, KB, capped at 5120)
              Version(str) Name(str, shown in Games menu) UserDataPath(str)
              Files(array) Platforms(array) LocalizedNames(optional)
  Files[]     Path(str) Size(int) Digest(data) Verify(bool) DRM(bool) DRMLevel(int)
  Platforms[] BuildID(int) PlatformID(int) PlatformVersion(int)
              ExecutablePath(str, must equal a Files[].Path) LaunchingArtwork(str)

Games are listed only if Platforms has an entry matching the device's
(GamesPlatformID, PlatformVersion): HW 0xB -> (1,1), 0x14 -> (2,1), 0x13 -> (3,1), else (0,0).
Every non-executable Files[] entry must exist with the exact Size, so only list the executable.
"""

# Created by an LLM

import argparse
import os
from xml.sax.saxutils import escape


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--exe", required=True, help="built executable (for Size)")
    ap.add_argument("--exe-path", default=None, help="path inside the game folder")
    ap.add_argument("--name", required=True)
    ap.add_argument("--guid", required=True)
    ap.add_argument("--version", default="1.0")
    ap.add_argument("--heap-kb", type=int, default=1024)
    ap.add_argument("--platforms", default="1:1,2:1,3:1,0:0",
                    help="comma list of PlatformID:PlatformVersion")
    a = ap.parse_args()

    exe_path = a.exe_path or os.path.basename(a.exe)
    size = os.path.getsize(a.exe)

    def s(k, v, ind):
        return '%s<key>%s</key>\n%s<string>%s</string>\n' % (ind, k, ind, escape(v))

    def i(k, v, ind):
        return '%s<key>%s</key>\n%s<integer>%d</integer>\n' % (ind, k, ind, v)

    def b(k, v, ind):
        return '%s<key>%s</key>\n%s<%s/>\n' % (ind, k, ind, "true" if v else "false")

    # Match retail layout: keys sorted, Size repeated in Platforms[].
    t = '\t'
    x = ('<?xml version="1.0" encoding="UTF-8"?>\n'
         '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" '
         '"http://www.apple.com/DTDs/PropertyList-1.0.dtd">\n'
         '<plist version="1.0">\n<dict>\n')
    x += s("BuildIdentifier", a.guid, t)
    x += t + "<key>Files</key>\n" + t + "<array>\n" + t * 2 + "<dict>\n"
    x += b("DRM", False, t * 3)
    x += s("Path", exe_path, t * 3)
    x += i("Size", size, t * 3)
    x += b("Verify", False, t * 3)
    x += t * 2 + "</dict>\n" + t + "</array>\n"
    x += s("GUID", a.guid, t)
    x += i("HeapSize", a.heap_kb, t)
    x += s("Name", a.name, t)
    x += t + "<key>Platforms</key>\n" + t + "<array>\n"
    for p in a.platforms.split(","):
        pid, pver = (int(v) for v in p.split(":"))
        x += t * 2 + "<dict>\n"
        x += i("BuildID", 1, t * 3)
        x += s("ExecutablePath", exe_path, t * 3)
        x += i("PlatformID", pid, t * 3)
        x += i("PlatformVersion", pver, t * 3)
        x += i("Size", size, t * 3)
        x += t * 2 + "</dict>\n"
    x += t + "</array>\n"
    x += s("Version", a.version, t)
    x += "</dict>\n</plist>\n"
    with open(a.out, "w") as f:
        f.write(x)


if __name__ == "__main__":
    main()
