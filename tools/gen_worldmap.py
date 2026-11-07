#!/usr/bin/env python3
"""Erzeugt src/data/WorldMapData.h (Land/Meer-Maske, 64x26, Equirectangular).
Quelle: Natural Earth 110m land (Public Domain).
Aufruf: python3 tools/gen_worldmap.py ne_110m_land.geojson src/data/WorldMapData.h
"""
import json, sys
from PIL import Image, ImageDraw

W, H = 64, 26
LAT_TOP, LAT_BOT = 84.0, -62.0     # 146 deg / 26 rows = 5.62 deg/px (wie 360/64)
SS = 16                             # Supersampling
THRESH = 0.30                       # Landanteil ab dem ein Pixel Land ist

def proj(lon, lat):
    x = (lon + 180.0) / 360.0 * W * SS
    y = (LAT_TOP - lat) / (LAT_TOP - LAT_BOT) * H * SS
    return (x, y)

src, dst = sys.argv[1], sys.argv[2]
gj = json.load(open(src))
img = Image.new("L", (W * SS, H * SS), 0)
d = ImageDraw.Draw(img)
for f in gj["features"]:
    g = f["geometry"]
    polys = g["coordinates"] if g["type"] == "MultiPolygon" else [g["coordinates"]]
    for poly in polys:
        d.polygon([proj(*p) for p in poly[0]], fill=255)
        for hole in poly[1:]:
            d.polygon([proj(*p) for p in hole], fill=0)
small = img.resize((W, H), Image.BOX)
bits = []
rows = []
for y in range(H):
    row = ""
    for x in range(W):
        land = small.getpixel((x, y)) / 255.0 >= THRESH
        row += "#" if land else "."
        bits.append(1 if land else 0)
    rows.append(row)
words = []
for y in range(H):  # 64 Bit pro Zeile -> 2x uint32 (Bit 0 = linkestes Pixel)
    lo = sum(bits[y*W + x] << x for x in range(32))
    hi = sum(bits[y*W + 32 + x] << x for x in range(32))
    words += [lo, hi]
with open(dst, "w") as o:
    o.write("#pragma once\n// Automatisch erzeugt von tools/gen_worldmap.py - nicht von Hand editieren.\n")
    o.write("// Natural Earth 110m land (Public Domain). Equirectangular, Lat %+.0f..%+.0f, %dx%d Pixel.\n" % (LAT_TOP, LAT_BOT, W, H))
    o.write("#include <stdint.h>\nnamespace WorldMap {\nconstexpr int W = %d, H = %d;\nconstexpr float LAT_TOP = %.1ff, LAT_BOT = %.1ff;\n" % (W, H, LAT_TOP, LAT_BOT))
    o.write("// Zeile y: Wort 2y = Pixel 0..31, Wort 2y+1 = Pixel 32..63 (Bit 0 = links)\n")
    o.write("static const uint32_t LAND[%d] = {\n" % len(words))
    for i in range(0, len(words), 4):
        o.write("  " + ", ".join("0x%08X" % w for w in words[i:i+4]) + ",\n")
    o.write("};\ninline bool isLand(int x, int y) {\n  if (x < 0 || x >= W || y < 0 || y >= H) return false;\n  return (LAND[y * 2 + (x >> 5)] >> (x & 31)) & 1;\n}\n}\n")
print("\n".join(rows))
