#!/usr/bin/env python3
"""Renders scene frames with build/sim and assembles a labelled contact sheet (LED-dot look).
Usage: render_sheet.py <out.png> <workdir> "label|scene|durMs|everyMs|k=v k=v" ..."""
import os, subprocess, sys
from PIL import Image, ImageDraw
out, work, cases = sys.argv[1], sys.argv[2], sys.argv[3:]
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
sim = os.path.join(root, "build", "sim")
S = 5  # px per LED
def led(frame):
    im = Image.new("RGB", (64 * S, 64 * S), (6, 6, 8)); d = ImageDraw.Draw(im)
    for y in range(64):
        for x in range(64):
            i = (y * 64 + x) * 3; c = tuple(frame[i:i + 3])
            d.rectangle([x * S, y * S, x * S + S - 2, y * S + S - 2], fill=c)
    return im
tiles = []
for case in cases:
    label, scene, dur, every, *rest = case.split("|")
    d = os.path.join(work, label.replace(" ", "_")); os.makedirs(d, exist_ok=True)
    for f in os.listdir(d): os.remove(os.path.join(d, f))
    subprocess.run([sim, d, scene, dur, every] + (rest[0].replace("_"," ").split("  ") if rest and rest[0] else []), check=True, stderr=subprocess.DEVNULL)
    frames = sorted(os.listdir(d))
    row = [(label, led(open(os.path.join(d, f), "rb").read())) for f in frames]
    tiles.append(row)
cols = max(len(r) for r in tiles)
W, H = cols * (64 * S + 8) + 8, len(tiles) * (64 * S + 26) + 8
sheet = Image.new("RGB", (W, H), (24, 26, 32)); dr = ImageDraw.Draw(sheet)
for r, row in enumerate(tiles):
    for c, (label, im) in enumerate(row):
        x, y = 8 + c * (64 * S + 8), 8 + r * (64 * S + 26)
        sheet.paste(im, (x, y + 16))
        if c == 0: dr.text((x, y), label, fill=(220, 230, 240))
sheet.save(out)
print("wrote", out, sheet.size)
