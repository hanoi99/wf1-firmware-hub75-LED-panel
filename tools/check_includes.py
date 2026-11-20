#!/usr/bin/env python3
"""Prueft, dass jedes #include "..." unter src/ so aufloesbar ist wie bei PlatformIO
(relativ zur einbindenden Datei oder relativ zu src/ bzw. lib/*). Gibt Fehler zurueck."""
import os, re, sys, glob
root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
src = os.path.join(root, "src")
libdirs = [d for d in glob.glob(os.path.join(root, "lib", "*")) if os.path.isdir(d)]
libdirs += [os.path.join(d, "src") for d in libdirs]
bad = 0
for dp, _, fs in os.walk(src):
    for f in fs:
        if not f.endswith((".h", ".cpp", ".c")): continue
        p = os.path.join(dp, f)
        for n, line in enumerate(open(p, errors="ignore"), 1):
            m = re.match(r'\s*#\s*include\s+"([^"]+)"', line)
            if not m: continue
            inc = m.group(1)
            cands = [os.path.join(dp, inc), os.path.join(src, inc)] + [os.path.join(d, inc) for d in libdirs]
            if not any(os.path.exists(c) for c in cands):
                # secrets.h ist optional (__has_include)
                if inc.endswith("secrets.h"): continue
                print("%s:%d: include nicht gefunden: %s" % (os.path.relpath(p, root), n, inc)); bad += 1
sys.exit(1 if bad else 0)
