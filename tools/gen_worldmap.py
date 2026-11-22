#!/usr/bin/env python3
"""Erzeugt src/data/WorldMap.h (Land/Meer-Maske, 64x26, Equirectangular).
Quelle: Natural Earth 110m land (Public Domain).
Aufruf: python3 tools/gen_worldmap.py ne_110m_land.geojson src/data/WorldMap.h
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
data = bytearray((W * H + 7) // 8)
for i, b in enumerate(bits):
    if b: data[i >> 3] |= 1 << (i & 7)
with open(dst, "w") as o:
    o.write("// Generiert von tools/gen_worldmap.py (Natural Earth, Public Domain). Nicht von Hand editieren.\n")
    o.write("#pragma once\n#include <stdint.h>\nnamespace WorldMap {\n")
    o.write("constexpr int W = %d, H = %d;\nconstexpr float LAT_TOP = %.1ff, LAT_BOT = %.1ff;\n" % (W, H, LAT_TOP, LAT_BOT))
    o.write("// Bit i = y*W+x, LSB zuerst; 1 = Land\n")
    o.write("static const uint8_t LAND[] = {" + ",".join(str(b) for b in data) + "};\n")
    o.write("inline bool isLand(int x, int y) { int i = y*W+x; return LAND[i>>3] & (1<<(i&7)); }\n}\n")
print("\n".join(rows))
