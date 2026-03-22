#!/usr/bin/env python3
"""
build_atlas.py — empaqueta assets/raw/*.png en assets/atlas/atlas.png + atlas.json
Todos los sprites deben ser múltiplos de TILE_SIZE (32 px).
Uso: python3 tools/build_atlas.py
"""

import json, math
from pathlib import Path
from PIL import Image

TILE_SIZE  = 32
RAW_DIR    = Path(__file__).parent.parent / "assets" / "raw"
ATLAS_DIR  = Path(__file__).parent.parent / "assets" / "atlas"
ATLAS_PNG  = ATLAS_DIR / "atlas.png"
ATLAS_JSON = ATLAS_DIR / "atlas.json"

ATLAS_JSON.parent.mkdir(parents=True, exist_ok=True)

# ── Recoger sprites ───────────────────────────────────────────────────────────
pngs = sorted(RAW_DIR.glob("*.png"))
if not pngs:
    raise SystemExit(f"No hay PNGs en {RAW_DIR}")

sprites = {}
for p in pngs:
    img = Image.open(p).convert("RGBA")
    w, h = img.size
    if w % TILE_SIZE != 0 or h % TILE_SIZE != 0:
        raise SystemExit(f"{p.name}: tamaño {w}×{h} no es múltiplo de {TILE_SIZE}")
    sprites[p.stem] = {"img": img, "w": w, "h": h}

# ── Calcular tamaño del atlas (filas/columnas cuadradas, potencia de 2) ───────
total_tiles = sum((s["w"] // TILE_SIZE) * (s["h"] // TILE_SIZE) for s in sprites.values())
cols = math.ceil(math.sqrt(total_tiles))
rows = math.ceil(total_tiles / cols)

# Potencia de 2 más cercana por arriba
def next_pow2(n): return 1 << math.ceil(math.log2(n)) if n > 1 else 1

atlas_w = next_pow2(cols * TILE_SIZE)
atlas_h = next_pow2(rows * TILE_SIZE)

print(f"Atlas: {atlas_w}×{atlas_h} px  ({len(sprites)} sprites, {total_tiles} tiles)")

# ── Empaquetar ────────────────────────────────────────────────────────────────
atlas_img = Image.new("RGBA", (atlas_w, atlas_h), (0, 0, 0, 0))

cursor_x, cursor_y = 0, 0
row_height = 0
json_sprites = {}

for name, s in sprites.items():
    img, w, h = s["img"], s["w"], s["h"]
    # salto de línea si no cabe
    if cursor_x + w > atlas_w:
        cursor_x  = 0
        cursor_y += row_height
        row_height = 0

    atlas_img.paste(img, (cursor_x, cursor_y))

    json_sprites[name] = {
        "x": cursor_x,
        "y": cursor_y,
        "w": w,
        "h": h,
        "frames":   1,
        "anchor_x": 0.5,
        "anchor_y": 0.5,
        "tags":     []
    }
    print(f"  {name:30s} @ ({cursor_x:4d}, {cursor_y:4d})  {w}×{h}")

    cursor_x  += w
    row_height = max(row_height, h)

# ── Guardar ───────────────────────────────────────────────────────────────────
atlas_img.save(ATLAS_PNG)

atlas_data = {
    "version":   1,
    "texture":   "atlas.png",
    "tile_size": TILE_SIZE,
    "sprites":   json_sprites
}
ATLAS_JSON.write_text(json.dumps(atlas_data, indent=2))

print(f"\nGuardado: {ATLAS_PNG}")
print(f"Guardado: {ATLAS_JSON}")
