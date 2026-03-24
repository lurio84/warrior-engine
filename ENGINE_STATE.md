# Engine State — The Warrior's Way

## Fase actual
**Fase 13 — C++ puro, sin Lua**
Motor base completo. Gameplay en sistemas C++/EnTT. Demo: drill animado → cinta → cofre.

---

## Stack de dependencias (FetchContent)
| Lib | Versión | Uso |
|---|---|---|
| SDL2 | 2.30.9 | Ventana, input, contexto GL |
| EnTT | 3.13.2 | ECS |
| GLM | 1.0.1 | Matemáticas |
| GLAD | 0.1.36 | OpenGL 4.5 core loader |
| stb_image | master | Carga de PNG para el atlas |
| efsw | 1.4.0 | File watcher (hot-reload shaders/atlas) |
| nlohmann/json | 3.11.3 | Parsing de atlas.json y tilemap |
| miniaudio | 0.11.21 | Audio (WAV/OGG/FLAC) |
| Dear ImGui | 1.91.0 | Debug UI en runtime |
| ENet | 1.3.17 | Multiplayer UDP fiable |

---

## Estructura de directorios
```
/warrior-engine
  /core           — motor: renderer, atlas, audio, input, camera, debug_ui, network, tilemap
    main.cpp      — game loop, eventos SDL, timestep 50 Hz
    game_scene.hpp — init de la escena (reemplaza main.lua)
    components.hpp — todos los componentes ECS
  /systems        — un archivo por sistema de gameplay (header-only inline)
    drill_system.hpp
    item_system.hpp
    conveyor_system.hpp
    machine_system.hpp
    player_system.hpp
  /assets
    /raw          — sprites fuente
    /sprites      — PNGs generados
    /atlas        — atlas.png + atlas.json (output del pipeline)
    /shaders      — GLSL (hot-reload por efsw)
    /sounds       — WAV de prueba
    /maps         — JSON de tilemaps
  /game
    /entities     — JSON de definiciones (stats, sprites, sonidos)
    /levels       — JSON de mapas
    /saves        — JSON de partidas
  /tools
    gen_test_sprites.py
    build_atlas.py
    gen_test_sounds.py
  CMakeLists.txt
  SYSTEMS_REGISTRY.md
  ENGINE_STATE.md
  engine_config.json
```

---

## Archivos del core C++
| Archivo | Responsabilidad |
|---|---|
| `main.cpp` | Loop principal, eventos SDL, timestep fijo 50 Hz, llama sistemas |
| `game_scene.hpp` | Inicialización de la escena (drill + belt + box + player) |
| `renderer.hpp/.cpp` | Draw calls OpenGL DSA, atlas UV, UV scroll animado |
| `file_watcher.hpp/.cpp` | efsw, detecta cambios en `assets/shaders/` |
| `atlas.hpp/.cpp` | Carga atlas.png + atlas.json, provee UV rects |
| `stb_impl.cpp` | Implementación única de stb_image |
| `audio.hpp/.cpp` | miniaudio: load/play/stop/volume |
| `audio_impl.cpp` | Implementación única de miniaudio |
| `camera.hpp` | Struct Camera (x, y, zoom) |
| `input.hpp` | InputSystem: snapshot teclado por frame |
| `debug_ui.hpp/.cpp` | Panels ImGui: perf, cámara, audio, entidades, red |
| `network.hpp/.cpp` | NetworkManager ENet: host/join, sync jugadores |
| `tilemap.hpp/.cpp` | Tilemap: load JSON, get/set tiles, draw |
| `components.hpp` | Todos los componentes ECS |

---

## Componentes ECS
```cpp
Transform     { x, y, rotation, scale_x, scale_y }
SpriteRef     { sprite_id, tint, size_w, size_h, scroll_x, scroll_y, layer }
Velocity      { vx, vy }             // tiles/segundo, aplicado por physics cada tick
NetworkPlayer { peer_id, is_local }
ConveyorTag   { speed, direction }   // marca entidad de cinta
DrillTag      { anim_t, frame, spawn_timer, dest_x, belt_speed }
ItemTag       { item_type, quantity, age, popping, source_x, dest_x, belt_speed }
MachineTag    { type, progress, recipe_id }
PlayerTag     { speed }
```

---

## Sistemas (SYSTEMS_REGISTRY.md)
Ver `SYSTEMS_REGISTRY.md` para orden de ejecución completo y propietario de cada componente.

```
player_system → conveyor_system → drill_system → item_system → physics → render
```

---

## Controles en ejecución
| Tecla | Acción |
|---|---|
| Flechas | Pan de cámara |
| `+` / `-` | Zoom: 0.5× → 1× → 2× → 3× → 4× |
| `0` | Reset zoom |
| `F1` | Toggle debug UI |
| `WASD` | Mueve jugador (smooth, via player_system) |
| `ESC` | Salir |

---

## Pipeline de assets
```
assets/raw/*.png
      ↓
tools/gen_test_sprites.py  → genera PNGs de prueba en assets/sprites/
tools/build_atlas.py       → assets/atlas/atlas.png + atlas.json
cp -r assets/atlas build/assets/
cmake --build build
```
- Atlas: potencia de 2, packing por filas, UV en [0,1] con Y-up (stbi flip)
- Sprite IDs: `snake_case` — nunca renombrar una vez en un save

---

## Sistema de red (ENet)
- `--host [puerto]` → modo servidor (peer_id=0)
- `--join <ip> [puerto]` → modo cliente (peer_id=1)
- Puerto por defecto: 7777

---

## Convenciones activas
- **Y-up**: mayor Y = más arriba en pantalla
- Tile size: 32 px (inmutable — `Renderer::TILE_SIZE_PX`)
- Tick rate: 50 Hz (20 ms/tick)
- Coordenadas: tiles en lógica, píxeles solo en renderer
- Todo el gameplay en `systems/` — sin scripting externo

---

## Próximos pasos
1. **Feedback al recolectar** — animación/sonido cuando item_pickup llega al cofre
2. **Cámara siguiendo al jugador** — camera_system que sigue al PlayerTag
3. **Segunda máquina** — processor: iron_ore → iron_ingot (machine_system activo)
4. **UI in-world** — contador de recursos sobre el cofre
5. **Colocación dinámica** — el jugador puede poner cintas y máquinas
