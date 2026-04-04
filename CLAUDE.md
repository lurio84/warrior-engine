# The Warrior's Way Engine — Contexto para Claude

Motor de juego 2D top-down factory-defender en C++20/OpenGL 4.5. Actualmente en **Phase 15**.

## Build y ejecución
```bash
# Assets (solo si cambias sprites o gen_test_sprites.py)
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
core/           — engine: main.cpp, renderer, atlas, audio, camera, debug_ui, input,
                           tilemap, placement_grid, game_scene, save_load
systems/        — gameplay: player, camera, drill, item, conveyor, placement,
                            machine, wave, enemy, combat
tools/          — gen_test_sprites.py, build_atlas.py
assets/raw/     — PNGs fuente (generados por gen_test_sprites.py)
assets/atlas/   — atlas.png + atlas.json (generados por build_atlas.py)
assets/maps/    — test.json (tilemap + lista de objetos de la escena inicial)
assets/sounds/  — click.wav, belt_loop.wav, item_pickup.wav
```

## Sistema de coordenadas
- Y aumenta hacia **arriba** (convención matemática, no de pantalla)
- Direcciones horarias: **0=E (+x), 1=S (-y), 2=W (-x), 3=N (+y)**
- Tecla **R** rota en sentido horario al construir

## Controles del juego
| Tecla | Acción |
|---|---|
| `WASD` | Mover jugador |
| `Space` | Ataque melee (radio 1.5 tiles, cooldown 0.4 s) |
| `E` | Craftear equipo en cofre adyacente (≤1.5 tiles) |
| Flechas | Pan de cámara |
| `+` / `-` | Zoom |
| `0` | Reset zoom |
| `1` / `2` / `3` | Seleccionar: cinta / taladro / cofre |
| `R` | Rotar pieza a colocar |
| Clic izq | Colocar pieza |
| Clic der | Borrar pieza |
| `F1` | Toggle debug UI |
| `F5` | Guardar partida |
| `F9` | Cargar partida |
| `ESC` | Salir |

## Loop de juego completo
```
Taladro → cinta → Forge (2× iron_ore → 1× iron_ingot, 2 s) → cinta → Cofre
                                                                         ↓
                                                         [E] Craftear equipo:
                                                           5 lingotes → espada_hierro
                                                           3 lingotes → casco_hierro
                                                           8 lingotes → pechera_hierro
```

## Stats de equipamiento (GDD 3.5)
| Slot | ID | Efecto |
|---|---|---|
| Arma | `espada_hierro` | Daño ataque 5 → 25 |
| Casco | `casco_hierro` | +20 HP máx, +5 defensa |
| Pechera | `pechera_hierro` | +50 HP máx, +10 defensa, velocidad 5→4 |

## Construcción (placement_system)
- Al colocar/borrar cualquier pieza: `reconnect_drills` reescanea todos los taladros
- `find_chain_dest` atraviesa cinta + `MachineTag` para encontrar el cofre destino del taladro
- Esquinas automáticas: `refresh_belt_sprite` detecta qué vecino apunta hacia el tile actual

## Sistemas ECS — orden en main.cpp (50 Hz fixed timestep)
1. `player_system` — input → velocidad + tint feedback (amarillo=ataque, rojo=daño recibido)
2. `enemy_system` — steering hacia jugador, daño contacto (reducido por defensa), tick cooldowns
3. `wave_system` — decrementa timer; spawna oleada en borde del mapa al llegar a 0
4. `conveyor_system` — actualiza sprite de esquinas animadas (frames 0-7)
5. `drill_system` — animación 8 frames + spawn items si `DrillTag.active`
6. `item_system` — pop-in + viaje por cinta + colección en BoxTag + depósito en MachineTag
7. `machine_system` — avanza recetas, spawna output si hay cinta en tile de salida
8. Physics — apply velocity → transform (colisión eje-separado para player y enemies)
9. **EffectTag tick** — decrementa TTL, destruye entidades expiradas (ej: `attack_ring`)
10. `camera_system` — lerp hacia el jugador

## Sistemas por frame (fuera del timestep fijo)
- `placement_system` — detección de clic + construcción/borrado de piezas
- `combat_system` — Space→ataque + spawn `EffectTag` ring + E→crafteo equipo desde cofre

## Sistema de oleadas (wave_system)
- 60 s hasta primera oleada · 30 s entre oleadas · 10 oleadas para victoria
- Goblin (oleada 1-2): 30 HP, 8 dmg, vel 2.0, tint verde
- Troll (oleada 3-6): 80 HP, 20 dmg, vel 1.0, tint morado
- Jefe menor (oleada 7+): 200 HP, 35 dmg, vel 1.5, tint naranja
- Borde del mapa con skip de tiles ocupados en PlacementGrid

## Layers de render
| Layer | Entidad |
|-------|---------|
| 1 | Cintas |
| 2 | Cofres |
| 3 | Taladros, Forge |
| 5 | Items en tránsito |
| 8 | Enemigos |
| 10 | Jugador |
| 15 | EffectTag (attack_ring) |
| 20 | Ghost preview (construcción) |

## Sprites en el atlas (35 sprites)
`attack_ring`, `conveyor_belt_idle`, `conveyor_corner_cw/ccw_0..7`,
`drill_0..7`, `enemy`, `floor_tile`, `forge`, `iron_ingot`, `item`,
`item_box`, `player`, `wall_tile`

## Componentes relevantes (core/components.hpp)
- `DrillTag`: `active`, `dest_x/y`, `belt_speed`, `spawn_timer`, `frame/anim_t`
- `ItemTag`: `item_type`, `quantity`, `popping`, `source_x/y`, `dest_x/y`, `belt_speed`
- `ConveyorTag`: `speed`, `direction` (0-3)
- `BoxTag`: marker — items que llegan a este tile se recogen en inventory
- `MachineTag`: `recipe_id`, `out_dir`, `progress`, `processing`, `output_ready`, `input_buf`
- `SolidTag`: jugador y enemigos no pueden atravesar el tile
- `PlayerTag`: `speed`, `attack_cd`
- `PlayerHealth`: `hp`, `max_hp`, `inv_t` (ventana de invencibilidad post-golpe = 0.8 s)
- `EquipmentTag`: `weapon_id`, `helmet_id`, `chest_id` (strings vacíos = slot libre)
- `EnemyTag`: `hp`, `max_hp`, `speed`, `contact_dmg`, `dmg_cd`
- `EffectTag`: `ttl` — entidad temporal destruida cuando ttl llega a 0
- `PlacementGrid`: estructura de datos `(int x, int y) → entt::entity`

## Escena inicial (assets/maps/test.json)
```
Mapa: 60×34 tiles, borde de muros
x=24,y=17 → drill (dest=36,17)
x=25..29  → conveyors dirección E
x=30,y=17 → forge (recipe=forge, out_dir=E)
x=31..35  → conveyors dirección E
x=36,y=17 → box (cofre — destino final)
x=21,y=14 → player
```

## Preferencias visuales
- Sin glow, wobble ni inflate en sprites
- Sprites full-tile (ocupan el tile completo)
- Colores diferenciados y legibles a zoom bajo

## Pendientes (por orden de prioridad)
1. **Contador visual en cofre** — mostrar nº de iron_ingot acumulados flotando sobre el tile
2. **Más recetas / máquina anvil** — expandir `machine_system` con más tipos de máquina y recetas (GDD 3.3)
3. **Sonidos** — place, remove, enemy_die, wave_warning (infraestructura de audio ya existe)
4. **Tiles de recurso en mapa** — depósitos de hierro/carbón/cobre según GDD 5.2
5. **Sprites de IA** — máquinas y enemigos con ComfyUI + Flux en RTX 5070
6. **Multiplayer sync** — NetworkManager existe pero no sincroniza entidades de juego
7. **Save/Load completo** — actualmente guarda inventory + wave + equip, no el mapa construido

## Bugs conocidos
- Enemigos no colisionan entre sí — se apilan en el mismo tile
- `save_load` no persiste entidades del mapa (cintas, taladros, forge colocados en runtime)
- `conveyor_system` es stub — la animación real la hace el renderer con `scroll_x`
