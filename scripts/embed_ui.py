#!/usr/bin/env python3
"""Bettet data/index.html, style.css und app.js (gzip) in src/data/WebUiData.h ein.

Laeuft als PlatformIO-Pre-Script bei jedem Build und kann auch direkt gestartet werden:
    python3 scripts/embed_ui.py
So ist die Weboberflaeche immer in der Firmware, auch wenn das LittleFS-Image fehlt.
"""
import gzip, os, sys

FILES = [("/index.html", "index.html", "text/html; charset=utf-8"),
         ("/style.css", "style.css", "text/css; charset=utf-8"),
         ("/app.js", "app.js", "application/javascript; charset=utf-8")]

def project_dir():
    try:
        Import("env")  # noqa: F821  (SCons)
        return env.subst("$PROJECT_DIR")  # noqa: F821
    except NameError:
        return os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(sys.argv[0])), ".."))

def generate(root):
    out = ["#pragma once",
           "// Automatisch erzeugt von scripts/embed_ui.py - nicht von Hand editieren.",
           "#include <stddef.h>", "#include <stdint.h>",
           "struct WebAsset { const char* path; const char* mime; const uint8_t* data; size_t len; };"]
    entries = []
    for i, (uri, name, mime) in enumerate(FILES):
        raw = open(os.path.join(root, "data", name), "rb").read()
        gz = gzip.compress(raw, 9, mtime=0)
        body = ",".join(str(b) for b in gz)
        out.append("static const uint8_t WEBUI_%d[] PROGMEM = {%s};" % (i, body))
        entries.append('    {"%s", "%s", WEBUI_%d, sizeof(WEBUI_%d)}' % (uri, mime, i, i))
    out.append("static const WebAsset WEBUI_ASSETS[] = {\n" + ",\n".join(entries) + "\n};")
    out.append("static const size_t WEBUI_ASSET_COUNT = %d;" % len(FILES))
    text = "\n".join(out) + "\n"
    dst = os.path.join(root, "src", "data", "WebUiData.h")
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if not os.path.exists(dst) or open(dst).read() != text:
        open(dst, "w").write(text)
        print("embed_ui: wrote", dst)

generate(project_dir())
