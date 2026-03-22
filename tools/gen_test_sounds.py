#!/usr/bin/env python3
"""
gen_test_sounds.py — genera sonidos WAV de prueba en assets/sounds/
Sin dependencias externas: usa el módulo wave + math de stdlib.
"""

import wave, struct, math
from pathlib import Path

OUT = Path(__file__).parent.parent / "assets" / "sounds"
OUT.mkdir(parents=True, exist_ok=True)

RATE = 44100

def write_wav(name: str, samples: list[float]):
    path = OUT / name
    with wave.open(str(path), "w") as f:
        f.setnchannels(1)
        f.setsampwidth(2)   # 16-bit
        f.setframerate(RATE)
        data = struct.pack(f"<{len(samples)}h",
                           *[max(-32768, min(32767, int(s * 32767))) for s in samples])
        f.writeframes(data)
    print(f"  {path}  ({len(samples)/RATE*1000:.0f} ms)")

def tone(freq, dur, amp=0.6, fade=0.01):
    n = int(RATE * dur)
    fade_n = int(RATE * fade)
    out = []
    for i in range(n):
        t = i / RATE
        s = amp * math.sin(2 * math.pi * freq * t)
        # fade in/out
        if i < fade_n:
            s *= i / fade_n
        elif i > n - fade_n:
            s *= (n - i) / fade_n
        out.append(s)
    return out

def click():
    # Breve burst de ruido blanco con decay rápido
    import random
    n = int(RATE * 0.04)
    return [0.5 * random.uniform(-1, 1) * math.exp(-i / (n * 0.15)) for i in range(n)]

def belt_loop():
    # Tono grave + vibrato suave simulando motor de cinta
    n = int(RATE * 1.0)
    out = []
    for i in range(n):
        t = i / RATE
        mod = 1.0 + 0.03 * math.sin(2 * math.pi * 8 * t)   # vibrato 8 Hz
        s  = 0.25 * math.sin(2 * math.pi * 60 * mod * t)   # fundamental 60 Hz
        s += 0.10 * math.sin(2 * math.pi * 120 * mod * t)  # 2ª armónica
        out.append(s)
    return out

def item_pickup():
    # Arpegio corto ascendente
    freqs = [523, 659, 784, 1047]   # C5 E5 G5 C6
    samples = []
    for f in freqs:
        samples += tone(f, 0.07, amp=0.5, fade=0.005)
    return samples

print("Generando sonidos de prueba en assets/sounds/:")
write_wav("click.wav",        click())
write_wav("belt_loop.wav",    belt_loop())
write_wav("item_pickup.wav",  item_pickup())
print("Listo.")
