#!/usr/bin/env python3
"""Convert an image to the 240x240 raw RGB565 format used by the project."""
from pathlib import Path
import sys
from PIL import Image

if len(sys.argv) != 3:
    print("Usage: python tools/convert_image.py input.png assets/tacho.rgb565")
    raise SystemExit(2)

img = Image.open(sys.argv[1]).convert("RGB")
if img.size != (240, 240):
    img = img.resize((240, 240), Image.Resampling.LANCZOS)

out = bytearray()
for r, g, b in img.getdata():
    v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    out += v.to_bytes(2, "big")

dst = Path(sys.argv[2])
dst.parent.mkdir(parents=True, exist_ok=True)
dst.write_bytes(out)
assert len(out) == 115200
print(f"Created {dst} ({len(out)} bytes)")
