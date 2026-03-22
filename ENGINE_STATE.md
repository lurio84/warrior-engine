# Engine state — The Warrior's Way

## Fase actual
**Fase 10 completa — Motor base terminado**

---

## Stack de dependencias (FetchContent)
| Lib | Versión | Uso |
|---|---|---|
| SDL2 | 2.30.9 | Ventana, input, contexto GL |
| EnTT | 3.13.2 | ECS |
| GLM | 1.0.1 | Matemáticas |
| GLAD | 0.1.36 | OpenGL 4.5 core loader |
| stb_image | master | Carga de PNG para el atlas |
| sol2 | 3.3.0 | Binding C++ → LuaJIT |
| LuaJIT | sistema | VM de scripts |
| efsw | 1.4.0 | File watcher (hot-reload) |
| nlohmann/json | 3.11.3 | Parsing de atlas.json |
| miniaudio | 0.11.21 | Audio (WAV/OGG/FLAC) |
| Dear ImGui | 1.91.0 | Debug UI en runtime |
| ENet | 1.3.17 | Multiplayer UDP fiable |

---

## Archivos del core C++
| Archivo | Responsabilidad |
|---|---|
| `main.cpp` | Loop principal, eventos SDL, timestep fijo 50 Hz |
| `renderer.hpp/.cpp` | Draw calls OpenGL DSA, atlas UV, UV scroll animado |
| `lua_vm.hpp/.cpp` | VM LuaJIT + todos los bindings `engine.*` |
| `file_watcher.hpp/.cpp` | efsw, detecta cambios en `scripts/` |
| `atlas.hpp/.cpp` | Carga atlas.png + atlas.json, provee UV rects |
| `stb_impl.cpp` | Implementación única de stb_image |
| `audio.hpp/.cpp` | miniaudio: load/play/stop/volume |
| `audio_impl.cpp` | Implementación única de miniaudio |
| `camera.hpp` | Struct Camera (x, y, zoom) |
| `input.hpp` | InputSystem: snapshot teclado por frame |
| `debug_ui.hpp/.cpp` | Panels ImGui: perf, cámara, audio, entidades, red |
| `network.hpp/.cpp` | NetworkManager ENet: host/join, sync jugadores |
| `components.hpp` | Transform, SpriteRef, Velocity, NetworkPlayer |

---

## Componentes ECS
```cpp
Transform    { x, y, rotation, scale_x, scale_y }
SpriteRef    { sprite_id, tint, size_w, size_h, scroll_x, scroll_y }
Velocity     { vx, vy }          // tiles/segundo, aplicado por C++ cada tick
NetworkPlayer{ peer_id, is_local }
```

---

## API Lua completa
```lua
-- Constantes
engine.TILE_SIZE                              -- 32 px

-- Log
engine.log(msg)

-- Entidades
engine.entity.create()           -> id
engine.entity.destroy(id)
engine.entity.set_pos(id, x, y)
engine.entity.get_pos(id)        -> x, y
engine.entity.set_rotation(id, radians)
engine.entity.set_color(id, r, g, b, a)
engine.entity.set_size(id, w, h)
engine.entity.set_sprite(id, sprite_id)
engine.entity.set_scroll(id, sx, sy)          -- UV scroll cycles/sec
engine.entity.set_velocity(id, vx, vy)        -- tiles/segundo
engine.entity.get_velocity(id)   -> vx, vy
engine.entity.stop(id)                        -- vx = vy = 0

-- Input (snapshot por frame)
engine.input.down("w")           -> bool      -- tecla mantenida
engine.input.pressed("space")    -> bool      -- flanco de subida

-- Audio
engine.audio.play(id)
engine.audio.stop(id)
engine.audio.volume(0..1)                     -- master volume

-- Hook de juego (definir en el script)
function update(dt) end                       -- llamado 50 veces/seg
```

---

## Controles en ejecución
| Tecla | Acción |
|---|---|
| Flechas | Pan de cámara |
| `+` / `-` | Zoom: 0.5× → 1× → 2× |
| `0` | Reset zoom |
| `F1` | Toggle debug UI |
| `ESC` | Salir |
| `WASD` | Mueve jugador de red (NetworkManager) |

---

## Pipeline de assets
```
assets/raw/*.png
      ↓
tools/build_atlas.py       → assets/atlas/atlas.png + atlas.json
tools/gen_test_sprites.py  → genera PNGs de prueba
tools/gen_test_sounds.py   → genera WAVs de prueba
```
- Atlas: potencia de 2, packing por filas, UV en [0,1] con Y-up (stbi flip)
- Sprite IDs: `snake_case` — nunca renombrar una vez en un save

---

## Sistema de red (ENet)
- `--host [puerto]` → modo servidor (peer_id=0, spawn x=4,y=7)
- `--join <ip> [puerto]` → modo cliente (peer_id=1, spawn x=10,y=7)
- Puerto por defecto: 7777
- Jugador local: tint blanco. Jugador remoto: tint naranja
- Protocolo: PKT_WELCOME / PKT_JOIN / PKT_LEAVE / PKT_MOVE (todos reliable)

---

## Convenciones activas
- **Y-up**: mayor Y = más arriba en pantalla
- Tile size: 32 px (inmutable — `Renderer::TILE_SIZE_PX`)
- Tick rate: 50 Hz (20 ms/tick)
- Coordenadas: tiles en lógica, píxeles solo en renderer
- Todo el gameplay se escribe en `scripts/` — cero C++ para mecánicas

---

## Próximos pasos (gameplay factory)
1. **Tilemap desde archivo** — cargar mapa JSON/Tiled
2. **Items en cinta** — entidades con Velocity que siguen la dirección de la cinta
3. **Máquinas** — splitter, merger, crafter (lógica 100% Lua)
4. **Pipeline AI sprites** — ComfyUI + FLUX.1 dev → `assets/raw/` automático
5. **Save/load** — serializar registry a JSON
