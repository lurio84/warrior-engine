#!/usr/bin/env python3
"""
gen_test_sprites.py — sprites procedurales limpios para el engine.
Sin dependencias de IA. Diseñados para tilear sin costuras.
"""

from PIL import Image, ImageDraw
from pathlib import Path

OUT = Path(__file__).parent.parent / "assets" / "raw"
OUT.mkdir(parents=True, exist_ok=True)

S = 32  # tile size

def save(img, name):
    path = OUT / name
    img.save(path)
    print(f"  {path}")

def px(img, x, y, c):
    if 0 <= x < S and 0 <= y < S:
        img.putpixel((x, y), c)

# ── Paleta ────────────────────────────────────────────────────────────────────
TRANS = (0, 0, 0, 0)

# ── floor_tile — suelo industrial gris, sin bordes blancos ───────────────────
# Diseño: base gris oscuro + rejilla de 1px cada 8px + brillo sutil en esquinas
def make_floor_tile():
    BASE   = (72, 72, 78, 255)
    GRID   = (52, 52, 58, 255)
    BRIGHT = (92, 92, 98, 255)

    img = Image.new("RGBA", (S, S), BASE)

    # Líneas de rejilla cada 8px (sin bordes externos para seamless)
    for i in (8, 16, 24):
        for x in range(S): img.putpixel((x, i), GRID)
        for y in range(S): img.putpixel((i, y), GRID)

    # Brillo en intersecciones internas
    for i in (8, 16, 24):
        for j in (8, 16, 24):
            img.putpixel((i, j), BRIGHT)

    return img

# ── wall_tile — muro industrial oscuro, contraste claro vs floor ──────────────
# Diseño: panel metálico oscuro con remaches en esquinas y borde interno
def make_wall_tile():
    BASE   = (45, 48, 55, 255)
    BORDER = (30, 32, 38, 255)
    RIVET  = (90, 95, 105, 255)
    LIGHT  = (65, 68, 78, 255)

    img = Image.new("RGBA", (S, S), BASE)
    d = ImageDraw.Draw(img)

    # Panel interior ligeramente más claro
    d.rectangle([3, 3, S-4, S-4], fill=LIGHT)
    # Borde externo oscuro
    d.rectangle([0, 0, S-1, S-1], outline=BORDER, width=2)
    # Borde interno
    d.rectangle([3, 3, S-4, S-4], outline=BORDER, width=1)

    # Remaches en esquinas del panel
    for rx, ry in [(4,4), (S-5,4), (4,S-5), (S-5,S-5)]:
        img.putpixel((rx, ry), RIVET)
        for dx, dy in [(-1,0),(1,0),(0,-1),(0,1)]:
            img.putpixel((rx+dx, ry+dy), RIVET)

    return img

# ── conveyor_belt_idle — cinta seamless con flechas ──────────────────────────
# Diseño: rieles laterales + superficie con chevrons apuntando →
# Seamless: el patrón se repite exactamente cada 32px
def make_conveyor_belt_idle():
    BASE  = (55, 52, 48, 255)
    RAIL  = (38, 36, 33, 255)
    BELT  = (70, 66, 60, 255)
    ARROW = (140, 130, 50, 255)
    ARROW_DK = (100, 92, 35, 255)

    img = Image.new("RGBA", (S, S), BASE)
    d = ImageDraw.Draw(img)

    # Superficie de la cinta (área central)
    d.rectangle([0, 4, S-1, S-5], fill=BELT)

    # Rieles laterales (superior e inferior)
    d.rectangle([0, 0, S-1, 3], fill=RAIL)
    d.rectangle([0, S-4, S-1, S-1], fill=RAIL)

    # Tornillos en el riel
    for rx in (4, 14, 24):
        img.putpixel((rx, 1), (80, 76, 70, 255))
        img.putpixel((rx, S-2), (80, 76, 70, 255))

    # Chevron → centrado (seamless: empieza en x=0, termina en x=31)
    # El patrón tiene dos chevrons separados 16px para que al tilear se vea continuo
    for ox in (0, 16):
        # Chevron pequeño en forma de >
        pts = [
            (ox+3,  10), (ox+9,  15), (ox+3,  21),   # flecha izquierda
            (ox+4,  10), (ox+10, 15), (ox+4,  21),
        ]
        for x in range(4):
            y1 = 10 + x
            y2 = 21 - x
            xp = (ox + 3 + x) % S
            img.putpixel((xp, y1), ARROW)
            img.putpixel((xp, y2), ARROW)
        # punta
        xp = (ox + 7) % S
        for y in range(13, 18):
            img.putpixel((xp, y), ARROW)

    # Líneas de separación de placas de la cinta
    for lx in (0, 16):
        for y in range(4, S-4):
            img.putpixel((lx, y), ARROW_DK)

    return img

# ── player — jugador top-down, círculo azul con dirección ────────────────────
def make_player():
    BODY    = (60, 130, 210, 255)
    OUTLINE = (30, 80, 150, 255)
    HEAD    = (220, 190, 150, 255)
    DIR     = (255, 240, 100, 255)

    img = Image.new("RGBA", (S, S), TRANS)
    d = ImageDraw.Draw(img)

    # Sombra
    d.ellipse([6, 8, 26, 28], fill=(0, 0, 0, 60))
    # Cuerpo
    d.ellipse([5, 5, 26, 26], fill=BODY, outline=OUTLINE, width=2)
    # Cabeza (círculo pequeño arriba-centro)
    d.ellipse([12, 7, 20, 15], fill=HEAD, outline=OUTLINE, width=1)
    # Indicador de dirección (punto amarillo en frente)
    d.ellipse([14, 8, 18, 12], fill=DIR)

    return img

# ── item_box — caja de madera con refuerzo metálico ──────────────────────────
def make_item_box():
    WOOD    = (180, 140, 70, 255)
    WOOD_DK = (140, 105, 45, 255)
    METAL   = (100, 100, 110, 255)
    METAL_L = (150, 150, 160, 255)
    DARK    = ( 60,  55,  50, 255)

    img = Image.new("RGBA", (S, S), WOOD)
    d = ImageDraw.Draw(img)

    # Borde oscuro externo (tile entero)
    d.rectangle([0, 0, S-1, S-1], outline=DARK, width=2)
    # Vetas de madera
    for y in (8, 14, 20, 26):
        d.line([(2, y), (S-3, y)], fill=WOOD_DK)
    # Flejes metálicos — de borde a borde
    d.rectangle([0, 0, S-1, 5],    fill=METAL, outline=METAL_L)
    d.rectangle([0, S-6, S-1, S-1], fill=METAL, outline=METAL_L)
    d.rectangle([0, 0, 5, S-1],    fill=METAL, outline=METAL_L)
    d.rectangle([S-6, 0, S-1, S-1], fill=METAL, outline=METAL_L)
    # Tornillos en esquinas
    for cx, cy in [(3,3), (S-4,3), (3,S-4), (S-4,S-4)]:
        d.ellipse([cx-2, cy-2, cx+2, cy+2], fill=METAL_L, outline=DARK)

    return img

# ── item — mineral de hierro: roca gris oscura con venas óxido ───────────────
def make_item():
    ORE    = (185,  85,  25, 255)   # naranja-herrumbre principal
    ORE_L  = (230, 130,  50, 255)   # faceta iluminada
    ORE_DK = ( 90,  35,  10, 255)   # sombra/contorno
    GREY   = (110, 100,  90, 255)   # veta gris mineral
    SHINE  = (255, 210, 140, 255)   # destello

    img = Image.new("RGBA", (S, S), TRANS)
    d = ImageDraw.Draw(img)

    # Sombra
    d.ellipse([9, 14, 23, 23], fill=(0, 0, 0, 80))
    # Cuerpo mineral irregular
    pts = [(10,19),(8,13),(12,8),(19,7),(25,12),(25,19),(21,24),(14,25),(10,22)]
    d.polygon(pts, fill=ORE, outline=ORE_DK)
    # Faceta superior iluminada
    d.polygon([(8,13),(12,8),(18,8),(15,13),(10,15)], fill=ORE_L)
    # Veta gris-mineral
    d.line([(14,22),(16,16),(20,12)], fill=GREY, width=1)
    # Destello
    img.putpixel((12, 11), SHINE)
    img.putpixel((13, 10), SHINE)
    img.putpixel((11, 12), SHINE)

    return img

# ── drill — taladro minero top-down, 2 frames de animación ───────────────────
# Frame 0: en reposo  — broca a la derecha, sin polvo
# Frame 1: activo     — broca girada 45°, chispas visibles
import math as _math

def _drill_body(d):
    """Cuerpo compartido: carcasa metálica oscura con remaches."""
    BASE   = (30,  28,  25,  255)
    FRAME  = (15,  14,  12,  255)
    METAL  = (65,  60,  55,  255)
    METAL_L= (100,  94,  88,  255)
    RIVET  = (130, 122, 112, 255)

    d.rectangle([0, 0, S-1, S-1], fill=METAL, outline=FRAME, width=2)
    d.rectangle([4, 4, S-5, S-5], fill=BASE, outline=METAL_L, width=1)
    for rx, ry in [(5,5),(S-6,5),(5,S-6),(S-6,S-6)]:
        d.ellipse([rx-2, ry-2, rx+2, ry+2], fill=RIVET, outline=FRAME)
    # Rejilla de ventilación izquierda
    for vy in (10, 14, 18, 22):
        d.line([(4, vy), (8, vy)], fill=FRAME)

def _draw_bit(img, d, angle_deg, bright=False):
    """Dibuja la broca girada a angle_deg (0=derecha, 90=arriba)."""
    BIT   = (210, 170,  40, 255)
    BIT_L = (255, 230, 120, 255) if bright else (240, 200,  80, 255)
    BIT_DK= (120,  95,  20, 255)
    HUB   = ( 80,  75,  68, 255)

    cx, cy = S//2, S//2
    rad = _math.radians(angle_deg)

    # Hub central
    d.ellipse([cx-4, cy-4, cx+4, cy+4], fill=HUB, outline=FRAME_C, width=1)

    # Brazo principal de la broca
    L = 10
    ex = int(cx + L * _math.cos(rad))
    ey = int(cy - L * _math.sin(rad))
    d.line([(cx, cy), (ex, ey)], fill=BIT, width=3)

    # Punta de diamante
    tip_pts = [
        (ex + int(3*_math.cos(rad)), ey - int(3*_math.sin(rad))),
        (ex + int(2*_math.cos(rad+_math.pi/2)), ey - int(2*_math.sin(rad+_math.pi/2))),
        (ex + int(2*_math.cos(rad-_math.pi/2)), ey - int(2*_math.sin(rad-_math.pi/2))),
    ]
    d.polygon(tip_pts, fill=BIT_L)

    # Brazo opuesto corto
    ex2 = int(cx - 6 * _math.cos(rad))
    ey2 = int(cy + 6 * _math.sin(rad))
    d.line([(cx, cy), (ex2, ey2)], fill=BIT_DK, width=2)

    # Chispas cuando está activo (bright=True)
    if bright:
        SPARK = (255, 240, 80, 255)
        for off in [(_math.pi/4, 4), (-_math.pi/6, 3), (_math.pi/3, 5)]:
            a2, dist = off
            sx = ex + int(dist * _math.cos(rad + a2))
            sy = ey - int(dist * _math.sin(rad + a2))
            if 0 <= sx < S and 0 <= sy < S:
                img.putpixel((sx, sy), SPARK)

FRAME_C = (15, 14, 12, 255)

def make_drill_frame(angle_deg, bright=False):
    img = Image.new("RGBA", (S, S), (0,0,0,0))
    d = ImageDraw.Draw(img)
    _drill_body(d)
    _draw_bit(img, d, angle_deg, bright=bright)
    return img

def make_drill_0(): return make_drill_frame(0)
def make_drill_1(): return make_drill_frame(-45)
def make_drill_2(): return make_drill_frame(-90)
def make_drill_3(): return make_drill_frame(-135)
def make_drill_4(): return make_drill_frame(180)
def make_drill_5(): return make_drill_frame(135)
def make_drill_6(): return make_drill_frame(90)
def make_drill_7(): return make_drill_frame(45)

# ── conveyor_corner (animado, 8 frames) ──────────────────────────────────────
# Cuarto-anillo circular con "ribs" (franjas radiales) que se mueven a lo largo
# del arco en cada frame, igual que los chevrones de las cintas rectas.
#
# CW  (giro derecha): borde inferior → borde derecho. Centro (31,31).
#   Parámetro t=0→1 mapea ángulo θ=π→π/2 (PIL: rib_x=31-r·cos(t·π/2), rib_y=31-r·sin(t·π/2))
# CCW (giro izquierda): borde inferior → borde izquierdo. Centro (0,31).
#   Parámetro t=0→1 mapea ángulo θ=0→π/2 (PIL: rib_x=r·cos(t·π/2), rib_y=31-r·sin(t·π/2))

import math as _math

def _corner_base(CX, CY):
    """
    Cuarto de círculo: BELT en r=4..27, RAIL en todo lo demás (incluyendo
    el área exterior al arco). Sin transparencia → el rail cubre el tile
    entero igual que en la cinta recta, evitando el gap visual del suelo.
    Alineación de bordes garantizada: RAIL(0..3)/BELT(4..27)/RAIL(28..31).
    """
    RAIL = (38, 36, 33, 255)
    BELT = (70, 66, 60, 255)
    img = Image.new("RGBA", (S, S), RAIL)   # fondo RAIL, no transparente
    for py in range(S):
        for px in range(S):
            r = _math.hypot(px - CX, py - CY)
            if 4 <= r < 28:                 # única zona de cinta
                img.putpixel((px, py), BELT)
    return img

def _arc_point(CX, CY, t, r, cw=True):
    """Posición PIL de un punto a radio r en parámetro t ∈ [0,1]."""
    angle = t * _math.pi / 2
    if cw:
        return (CX - r * _math.cos(angle), CY - r * _math.sin(angle))
    else:
        return (CX + r * _math.cos(angle), CY - r * _math.sin(angle))

def _arc_tangent(t, cw=True):
    """Dirección de avance (tangente) del arco en t, normalizada."""
    angle = t * _math.pi / 2
    if cw:
        # d/dt (31 - r·cos(angle)) = r·sin·(π/2),  d/dt (31 - r·sin(angle)) = -r·cos·(π/2)
        # normalizado: (sin, -cos)
        return (_math.sin(angle), -_math.cos(angle))
    else:
        # d/dt (r·cos(angle)) = -r·sin·(π/2),  d/dt (31 - r·sin(angle)) = -r·cos·(π/2)
        # normalizado: (-sin, -cos)
        return (-_math.sin(angle), -_math.cos(angle))

def _draw_belt_markings(img, CX, CY, frame, cw=True, N=8):
    """
    Replica el lenguaje visual de la cinta recta en el arco:
      - Línea separadora de placa (ARROW_DK) radial — como las líneas oscuras verticales
      - Chevron ">" rotado entre placas — como los de la cinta recta
    2 placas por arco (= 2 placas por tile en cinta recta).
    """
    ARROW    = (140, 130, 50, 255)
    ARROW_DK = (80,  72,  27, 255)
    offset   = frame / N

    def put(x, y, c=ARROW):
        xi, yi = int(round(x)), int(round(y))
        if 0 <= xi < S and 0 <= yi < S:
            img.putpixel((xi, yi), c)

    for i in range(2):                         # 2 placas por arco
        t_sep  = ((i * 0.5) + offset) % 1.0   # separador de placa

        # Línea radial oscura (de r=5 a r=26, zona de cinta)
        for r in range(5, 27):
            rx, ry = _arc_point(CX, CY, t_sep, r, cw)
            put(rx, ry, ARROW_DK)

        # Chevron en el centro de la placa (t_sep + 0.25)
        t_chev = (t_sep + 0.25) % 1.0
        cx, cy = _arc_point(CX, CY, t_chev, 14, cw)
        tx, ty = _arc_tangent(t_chev, cw)
        px, py = -ty, tx                       # perpendicular a la tangente

        arm = 4
        tip_x = cx + arm * tx
        tip_y = cy + arm * ty
        put(tip_x, tip_y)
        for step in range(1, arm + 2):
            s = float(step)
            put(tip_x - s*tx + s*px, tip_y - s*ty + s*py)
            put(tip_x - s*tx - s*px, tip_y - s*ty - s*py)

def make_conveyor_corner_cw_frame(frame):
    img = _corner_base(S-1, S-1)
    _draw_belt_markings(img, S-1, S-1, frame, cw=True)
    return img

def make_conveyor_corner_ccw_frame(frame):
    img = _corner_base(0, S-1)
    _draw_belt_markings(img, 0, S-1, frame, cw=False)
    return img


# ── enemy — criatura hostil top-down, cuerpo rojo oscuro con ojos ────────────
def make_enemy():
    BODY    = (160,  30,  30, 255)   # rojo oscuro
    BODY_DK = ( 90,  15,  15, 255)   # contorno
    BODY_L  = (210,  60,  60, 255)   # reflejo
    EYE     = (255, 220,  50, 255)   # ojos amarillos
    EYE_DK  = (180, 140,  20, 255)   # pupila
    SPIKE   = (120,  20,  20, 255)   # pinchos

    img = Image.new("RGBA", (S, S), TRANS)
    d = ImageDraw.Draw(img)

    # Sombra
    d.ellipse([5, 9, 27, 27], fill=(0, 0, 0, 70))
    # Cuerpo principal
    d.ellipse([4, 4, 27, 27], fill=BODY, outline=BODY_DK, width=2)
    # Reflejo (cuadrante superior izq)
    d.ellipse([7, 7, 17, 17], fill=BODY_L)
    # Ojos (dos puntos amarillos)
    for ex, ey in [(10, 12), (20, 12)]:
        d.ellipse([ex-3, ey-3, ex+3, ey+3], fill=EYE, outline=BODY_DK)
        d.ellipse([ex-1, ey-1, ex+1, ey+1], fill=EYE_DK)
    # Boca (arco oscuro)
    d.arc([11, 17, 21, 24], start=10, end=170, fill=BODY_DK, width=2)
    # Pinchos en los 4 diagonales
    for sx, sy, ex2, ey2 in [(4,4,0,0), (27,4,31,0), (4,27,0,31), (27,27,31,31)]:
        d.line([(sx, sy), (ex2, ey2)], fill=SPIKE, width=2)

    return img

# ── iron_ingot — lingote de hierro forjado, barra metálica plateada ───────────
def make_iron_ingot():
    METAL   = (160, 165, 175, 255)   # plateado base
    METAL_L = (210, 215, 225, 255)   # cara superior iluminada
    METAL_DK= ( 90,  92, 100, 255)   # cara frontal oscura
    EDGE    = ( 55,  57,  65, 255)   # contorno
    SHINE   = (240, 245, 255, 255)   # destello

    img = Image.new("RGBA", (S, S), TRANS)
    d = ImageDraw.Draw(img)

    # Sombra
    d.ellipse([6, 22, 26, 29], fill=(0, 0, 0, 60))
    # Cara frontal (trapecio que da sensación 3D)
    d.polygon([(4,14),(28,14),(26,26),(6,26)], fill=METAL_DK, outline=EDGE)
    # Cara superior (perspectiva isométrica leve)
    d.polygon([(4,14),(28,14),(30,8),(6,8)], fill=METAL_L, outline=EDGE)
    # Cara lateral derecha
    d.polygon([(28,14),(30,8),(30,22),(28,26)], fill=METAL, outline=EDGE)
    # Borde superior
    d.line([(6,8),(30,8)], fill=EDGE, width=1)
    # Destello en esquina superior izquierda de la cara superior
    for dx2, dy2 in [(8,9),(9,9),(8,10)]:
        img.putpixel((dx2, dy2), SHINE)

    return img


print("Generando sprites procedurales en assets/raw/:")
save(make_floor_tile(),          "floor_tile.png")
save(make_wall_tile(),           "wall_tile.png")
save(make_conveyor_belt_idle(),  "conveyor_belt_idle.png")
for f in range(8):
    save(make_conveyor_corner_cw_frame(f),  f"conveyor_corner_cw_{f}.png")
    save(make_conveyor_corner_ccw_frame(f), f"conveyor_corner_ccw_{f}.png")
save(make_player(),              "player.png")
save(make_item_box(),            "item_box.png")
save(make_item(),                "item.png")
save(make_enemy(),               "enemy.png")
save(make_iron_ingot(),          "iron_ingot.png")
for i, fn in enumerate([make_drill_0, make_drill_1, make_drill_2, make_drill_3,
                         make_drill_4, make_drill_5, make_drill_6, make_drill_7]):
    save(fn(), f"drill_{i}.png")
print("Listo.")
