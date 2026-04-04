# Engine State — The Warrior's Way Engine

## Fase actual
**Phase 15 — Factory-defender completo, jugable end-to-end**

Loop completo implementado: drill → cinta → forge → cinta → cofre → crafteo de equipo → combate vs oleadas.

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
  /core
    main.cpp          — game loop (50 Hz fixed timestep), eventos SDL, GameState
    game_scene.hpp    — init_scene: spawna entidades desde assets/maps/test.json
    components.hpp    — todos los componentes ECS
    renderer.hpp/.cpp — draw calls OpenGL DSA, UV atlas, UV scroll animado
    atlas.hpp/.cpp    — carga atlas.png + atlas.json, provee UV rects
    audio.hpp/.cpp    — miniaudio: load/play/stop/volume
    camera.hpp        — struct Camera (x, y, zoom)
    input.hpp         — InputSystem: snapshot teclado+mouse por frame
    debug_ui.hpp/.cpp — HUD (HP, oleada, equipo, recursos) + panel F1
    network.hpp/.cpp  — NetworkManager ENet: host/join, sync jugadores
    tilemap.hpp/.cpp  — carga y dibuja tilemap JSON
    placement_grid.hpp — mapa 2D (int x, int y) → entt::entity
    save_load.hpp     — F5/F9: guarda/carga inventory + wave + EquipmentTag
    file_watcher.hpp/.cpp — efsw, hot-reload de shaders
    stb_impl.cpp      — implementación única de stb_image
    audio_impl.cpp    — implementación única de miniaudio
  /systems
    player_system.hpp    — input → velocity, tint feedback
    enemy_system.hpp     — steering, daño contacto, cooldowns
    wave_system.hpp      — timer + spawn oleadas en borde
    conveyor_system.hpp  — actualiza sprite de esquinas
    drill_system.hpp     — animación + spawn items
    item_system.hpp      — pop-in + viaje + colección/depósito
    machine_system.hpp   — recetas (forge: 2 iron_ore → 1 iron_ingot)
    camera_system.hpp    — lerp hacia jugador
    placement_system.hpp — construcción/borrado de piezas
    combat_system.hpp    — ataque melee + crafteo equipo + EffectTag ring
  /assets
    /raw          — sprites fuente PNG (generados por gen_test_sprites.py)
    /atlas        — atlas.png + atlas.json (NO en git — regenerar con build_atlas.py)
    /shaders      — GLSL (hot-reload por efsw)
    /sounds       — click.wav, belt_loop.wav, item_pickup.wav
    /maps         — test.json (tilemap + objetos de escena)
  /tools
    gen_test_sprites.py  — genera 35 sprites procedurales en assets/raw/
    build_atlas.py       — empaqueta assets/raw/*.png en assets/atlas/
  CMakeLists.txt
  CLAUDE.md             — contexto completo para Claude Code
  ENGINE_STATE.md       — este archivo
  SYSTEMS_REGISTRY.md   — tabla de propietario de cada componente
  GDD.md                — Game Design Document (fuente de verdad de gameplay)
```

---

## Componentes ECS actuales (core/components.hpp)
```cpp
Transform     { x, y, rotation, scale_x, scale_y }
SpriteRef     { sprite_id, tint, size_w, size_h, scroll_x, scroll_y, layer }
Velocity      { vx, vy }
NetworkPlayer { peer_id, is_local }
ConveyorTag   { speed, direction }
DrillTag      { anim_t, frame, spawn_timer, dest_x, dest_y, belt_speed, active }
ItemTag       { item_type, quantity, age, popping, source_x, source_y, dest_x, dest_y, belt_speed }
MachineTag    { recipe_id, out_dir, progress, processing, output_ready, input_buf }
PlayerTag     { speed, attack_cd }
PlayerHealth  { hp, max_hp, inv_t }
EquipmentTag  { weapon_id, helmet_id, chest_id }
EnemyTag      { hp, max_hp, speed, contact_dmg, dmg_cd }
BoxTag        {}  // marker — colección de items
SolidTag      {}  // marker — colisión jugador/enemigos
EffectTag     { ttl }  // entidad temporal (attack_ring, etc.)
```

---

## Sistemas (ver SYSTEMS_REGISTRY.md para detalle completo)
```
Fixed timestep 50 Hz:
  player_system → enemy_system → wave_system → conveyor_system
  → drill_system → item_system → machine_system → physics
  → EffectTag tick → camera_system

Por frame:
  placement_system · combat_system
```

---

## Controles
| Tecla | Acción |
|---|---|
| `WASD` | Mover jugador |
| `Space` | Ataque melee (radio 1.5 t, CD 0.4 s) |
| `E` | Craftear equipo en cofre adyacente |
| Flechas | Pan cámara |
| `+/-` | Zoom |
| `0` | Reset zoom |
| `1/2/3` | Seleccionar cinta/taladro/cofre |
| `R` | Rotar pieza |
| Clic L/D | Colocar / Borrar |
| `F1` | Debug UI |
| `F5`/`F9` | Guardar / Cargar |
| `ESC` | Salir |
| `R` (Game Over/Victory) | Reiniciar |

---

## Pipeline de assets
```bash
python3 tools/gen_test_sprites.py   # → assets/raw/*.png  (35 sprites)
python3 tools/build_atlas.py        # → assets/atlas/atlas.png + atlas.json
cp -r assets/atlas build/assets/    # copiar al directorio de build
cmake --build build                 # compilar
```
- Atlas: 256×256 px, packing por filas, UV en [0,1] con Y-up
- Sprite IDs: `snake_case` — no renombrar una vez usados en save files
- assets/atlas y assets/raw están en .gitignore (se regeneran)

---

## Sistema de red
- `--host [puerto]` → modo servidor (peer_id=0), puerto por defecto 7777
- `--join <ip> [puerto]` → modo cliente
- Sincroniza posición de jugadores, NO entidades de gameplay

---

## Convenciones
- **Y-up**: mayor Y = más arriba en pantalla
- Tile size: 32 px (constante `Renderer::TILE_SIZE_PX`)
- Tick rate: 50 Hz (20 ms/tick)
- Coordenadas en tiles en lógica, píxeles solo en renderer
- Todo el gameplay en `systems/` como funciones `inline` en headers
- EnTT structured bindings: los componentes vacíos (BoxTag, SolidTag) NO aparecen en el tuple

---

## Historial resumido de fases

| Fase | Contenido |
|---|---|
| 1-12 | Lua scripting, prototipo, renderer básico |
| 13 | Reescritura a C++ puro, ECS EnTT, renderer OpenGL DSA |
| 14 | Oleadas, combate, save/load, escena desde JSON, esquinas animadas |
| 15 | UX: ataque visual (attack_ring), crafteo de equipo, prompt cofre, oleadas más lentas, forge sprite propio, volumen inicial moderado |

---

## Pendientes (por orden de prioridad)
1. **Contador visual en cofre** — nº de iron_ingot sobre el tile en world-space
2. **Más recetas / máquina anvil** — ampliar machine_system con más tipos (GDD 3.3)
3. **Sonidos** — place, remove, enemy_die, wave_warning
4. **Tiles de recurso en mapa** — depósitos de hierro/carbón/cobre (GDD 5.2)
5. **Sprites IA** — ComfyUI + Flux en RTX 5070
6. **Multiplayer sync** — entidades de juego (actualmente solo jugadores)
7. **Save/Load completo** — persistir entidades del mapa construidas en runtime

## Bugs conocidos
- Enemigos no colisionan entre sí (se apilan)
- save_load no persiste entidades del mapa colocadas en runtime
