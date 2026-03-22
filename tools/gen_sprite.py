#!/usr/bin/env python3
"""
gen_sprite.py — Genera un sprite para el engine usando ComfyUI + FLUX.1 schnell GGUF
Uso:
    python tools/gen_sprite.py "floor tile stone" floor_tile
    python tools/gen_sprite.py "conveyor belt mechanical" conveyor_belt_idle
    python tools/gen_sprite.py "wooden crate box" item_box

El PNG resultante se guarda en assets/raw/<name>.png (32x32 RGBA).
Luego corre tools/build_atlas.py para actualizar el atlas.
"""

import sys
import json
import random
import urllib.request
import urllib.error
import time
from pathlib import Path

# ── Config ────────────────────────────────────────────────────────────────────
COMFYUI_URL  = "http://127.0.0.1:8188"
OUTPUT_DIR   = Path(__file__).parent.parent / "assets" / "raw"
IMAGE_SIZE   = 512    # generamos a 512px, luego escalamos a 32px
TILE_SIZE    = 32

# Parámetros fijos para FLUX schnell
STEPS        = 4
CFG          = 1.0
SAMPLER      = "euler"
SCHEDULER    = "simple"

# Modelos — deben coincidir con lo instalado en ComfyUI
GGUF_MODEL   = "flux1-schnell-Q8_0.gguf"
VAE_MODEL    = "ae.safetensors"
CLIP_L       = "clip_l.safetensors"
T5_MODEL     = "t5xxl_fp8_e4m3fn.safetensors"

# ── Prompt base para sprites de juego ─────────────────────────────────────────
STYLE_PREFIX = (
    "pixel art, 32x32 sprite, top-down view, game asset, "
    "clean outlines, flat colors, no background, transparent background, "
    "factory game, tile, "
)
STYLE_SUFFIX = ", simple, iconic, 16-color palette"

# ── Workflow FLUX schnell GGUF ─────────────────────────────────────────────────

def build_workflow(prompt: str, seed: int) -> dict:
    return {
        "1": {
            "class_type": "UnetLoaderGGUF",
            "inputs": {"unet_name": GGUF_MODEL}
        },
        "2": {
            "class_type": "CLIPLoader",
            "inputs": {"clip_name": CLIP_L, "type": "flux"}
        },
        "3": {
            "class_type": "CLIPLoader",
            "inputs": {"clip_name": T5_MODEL, "type": "flux"}
        },
        "4": {
            "class_type": "DualCLIPLoader",
            "inputs": {
                "clip_name1": CLIP_L,
                "clip_name2": T5_MODEL,
                "type": "flux"
            }
        },
        "5": {
            "class_type": "VAELoader",
            "inputs": {"vae_name": VAE_MODEL}
        },
        "6": {
            "class_type": "CLIPTextEncode",
            "inputs": {
                "text": STYLE_PREFIX + prompt + STYLE_SUFFIX,
                "clip": ["4", 0]
            }
        },
        "7": {
            "class_type": "EmptyLatentImage",
            "inputs": {"width": IMAGE_SIZE, "height": IMAGE_SIZE, "batch_size": 1}
        },
        "8": {
            "class_type": "KSampler",
            "inputs": {
                "model":           ["1", 0],
                "positive":        ["6", 0],
                "negative":        ["6", 0],   # FLUX schnell: sin negative prompt
                "latent_image":    ["7", 0],
                "seed":            seed,
                "steps":           STEPS,
                "cfg":             CFG,
                "sampler_name":    SAMPLER,
                "scheduler":       SCHEDULER,
                "denoise":         1.0
            }
        },
        "9": {
            "class_type": "VAEDecode",
            "inputs": {"samples": ["8", 0], "vae": ["5", 0]}
        },
        "10": {
            "class_type": "SaveImage",
            "inputs": {"images": ["9", 0], "filename_prefix": "gen_sprite"}
        }
    }


# ── API helpers ───────────────────────────────────────────────────────────────

def post_json(url: str, data: dict) -> dict:
    body = json.dumps(data).encode()
    req  = urllib.request.Request(url, data=body,
                                  headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=30) as r:
        return json.loads(r.read())


def get_json(url: str) -> dict:
    with urllib.request.urlopen(url, timeout=10) as r:
        return json.loads(r.read())


def wait_for_job(prompt_id: str) -> str:
    """Espera hasta que el job termine y retorna el nombre del archivo generado."""
    print("  Generando", end="", flush=True)
    while True:
        hist = get_json(f"{COMFYUI_URL}/history/{prompt_id}")
        if prompt_id in hist:
            outputs = hist[prompt_id]["outputs"]
            for node_out in outputs.values():
                if "images" in node_out:
                    img = node_out["images"][0]
                    print(" ✓")
                    return img["filename"], img.get("subfolder", "")
        print(".", end="", flush=True)
        time.sleep(1)


def download_image(filename: str, subfolder: str) -> bytes:
    url = f"{COMFYUI_URL}/view?filename={filename}&subfolder={subfolder}&type=output"
    with urllib.request.urlopen(url, timeout=15) as r:
        return r.read()


# ── Scale to 32x32 ────────────────────────────────────────────────────────────

def scale_to_tile(image_bytes: bytes) -> bytes:
    """Escala la imagen a 32x32 usando PIL o fallback con struct."""
    try:
        from PIL import Image
        import io
        img = Image.open(io.BytesIO(image_bytes)).convert("RGBA")
        img = img.resize((TILE_SIZE, TILE_SIZE), Image.LANCZOS)
        out = io.BytesIO()
        img.save(out, format="PNG")
        return out.getvalue()
    except ImportError:
        print("  [warn] PIL no disponible — guardando imagen original sin escalar")
        print("  Instala con: pip install Pillow")
        return image_bytes


# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    prompt    = sys.argv[1]
    name      = sys.argv[2]
    seed      = int(sys.argv[3]) if len(sys.argv) > 3 else random.randint(0, 2**32)
    out_path  = OUTPUT_DIR / f"{name}.png"

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    print(f"[gen_sprite] prompt : {STYLE_PREFIX}{prompt}{STYLE_SUFFIX}")
    print(f"[gen_sprite] nombre : {name}.png")
    print(f"[gen_sprite] seed   : {seed}")

    # 1. Verificar que ComfyUI está corriendo
    try:
        get_json(f"{COMFYUI_URL}/system_stats")
    except urllib.error.URLError:
        print(f"\n[error] ComfyUI no responde en {COMFYUI_URL}")
        print("  Arráncalo con: cd ~/ComfyUI && source venv/bin/activate && python main.py --listen --fp16-unet --bf16-vae")
        sys.exit(1)

    # 2. Enviar workflow
    workflow = build_workflow(prompt, seed)
    resp     = post_json(f"{COMFYUI_URL}/prompt", {"prompt": workflow})
    prompt_id = resp["prompt_id"]
    print(f"[gen_sprite] job    : {prompt_id}")

    # 3. Esperar resultado
    filename, subfolder = wait_for_job(prompt_id)

    # 4. Descargar y escalar a 32x32
    print(f"  Descargando {filename}...")
    raw = download_image(filename, subfolder)
    png = scale_to_tile(raw)

    # 5. Guardar
    out_path.write_bytes(png)
    print(f"[gen_sprite] guardado → {out_path}")
    print()
    print("  Ahora corre: python tools/build_atlas.py")


if __name__ == "__main__":
    main()
