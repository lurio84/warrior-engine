# The Warrior's Way Engine — Contexto para Claude

Motor de juego 2D top-down factory en C++20/OpenGL 4.5. Actualmente en **Phase 14**.

## Build y ejecución
```bash
# Assets (solo si cambias sprites)
python3 tools/gen_test_sprites.py && python3 tools/build_atlas.py && cp -r assets/atlas build/assets/

# Compilar y ejecutar
cmake --build build && ./build/warrior_engine
```
> **Regla**: compilar siempre después de cambiar C++ y verificar que no hay errores antes de reportar resultados.

## Stack
- **ECS**: EnTT — componentes en `core/components.hpp`
- **Render**: SDL2 + OpenGL 4.5 (GLAD) — renderer en `core/renderer.cpp`
- **UI**: ImGui — debug panel (F1) + HUD inventario (siempre visible)
- **Red**: ENet (`--host` / `--join`)

## Estructura clave
```
core/           — engine: main.cpp, renderer, atlas, audio, camera, debug_ui, input, tilemap, placement_grid, game_scene
systems/        — gameplay: player, camera, drill, item, conveyor, placement, machine(stub)
tools/          — gen_test_sprites.py, build_atlas.py
assets/raw/     — PNGs fuente (generados por gen_test_sprites.py)
assets/atlas/   — atlas.png + atlas.json (generados por build_atlas.py)
assets/maps/    — test.json (tilemap Tiled)
```

## Sistema de coordenadas
- Y aumenta hacia **arriba** (convención matemática, no de pantalla)
- Direcciones horarias: **0=E (+x), 1=S (-y), 2=W (-x), 3=N (+y)**
- Tecla **R** rota en sentido horario al construir

## Construcción (placement_system)
- **1** cinta · **2** taladro · **3** cofre · **R** rotar · clic izq colocar · clic der borrar
- Al colocar/borrar cualquier pieza: `reconnect_drills` reescanea todos los taladros
- Esquinas automáticas: `refresh_belt_sprite` detecta qué vecino apunta hacia el tile actual

## Sistemas ECS — orden en main.cpp (50 Hz fixed timestep)
1. `player_system` — input → velocidad
2. `conveyor_system` — stub
3. `drill_system` — animación 8 frames + spawn items si `DrillTag.active`
4. `item_system` — pop-in + viaje por cinta + colección
5. Física: apply velocity → transform (con colisión sub-tile para el jugador)
6. `camera_system` — lerp hacia el jugador

## Inventario
- `SceneState::inventory` = `map<string,int>` acumulado por `item.item_type`
- Items actuales: `"iron_ore"` (naranja-herrumbre)
- HUD: `debug_ui.draw_hud()` siempre visible esquina superior derecha

## Layers de render
| Layer | Entidad |
|-------|---------|
| 1 | Cintas |
| 2 | Cofres |
| 3 | Taladros |
| 5 | Items en tránsito |
| 10 | Jugador |
| 20 | Ghost preview (construcción) |

## Componentes relevantes (core/components.hpp)
- `DrillTag`: `active`, `dest_x/y`, `belt_speed`, `spawn_timer`, `frame/anim_t`
- `ItemTag`: `item_type`, `quantity`, `popping`, `source_x/y`, `dest_x/y`, `belt_speed`
- `ConveyorTag`: `speed`, `direction` (0-3)
- `BoxTag`: marker, items que llegan a este tile se recogen
- `SolidTag`: el jugador no puede atravesar el tile
- `PlacementGrid`: mapa `(int x, int y) → entt::entity`

## Preferencias visuales
- Sin glow, wobble ni inflate en sprites
- Sprites full-tile (ocupan el tile completo)
- Colores diferenciados y legibles a zoom bajo

## Pendientes (por orden de prioridad)
1. **Máquina procesadora** — `machine_system` real con recetas (hierro → lingotes)
2. **Feedback pickup** — audio `item_pickup` ya cargado en `audio`
3. **UI in-world** — contador sobre el cofre
4. **Sprites IA** — máquinas estáticas generadas con ComfyUI + Flux en RTX 5070
5. **Animación esquinas** — actualmente estáticas (scroll=0), mejorar si se añaden frames
