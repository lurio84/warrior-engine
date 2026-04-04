# SYSTEMS_REGISTRY — The Warrior's Way Engine

Registro canónico de qué sistema **escribe** cada componente.
Regla: un componente tiene un único dueño. Leer es libre; escribir solo el dueño.

---

## Componentes y propietarios

| Componente | Propietario (escribe) | Lectores frecuentes |
|---|---|---|
| `Transform` | `physics` (main.cpp) | renderer, item_system, enemy_system, camera_system |
| `Velocity` | `player_system`, `enemy_system`, `item_system`, `conveyor_system` | physics |
| `SpriteRef` | `drill_system` (frames), `conveyor_system` (corner frames), `placement_system` (ghost) | renderer |
| `DrillTag` | `drill_system` | placement_system, game_scene |
| `ItemTag` | `item_system` | drill_system (spawn), machine_system (spawn) |
| `ConveyorTag` | `placement_system` (colocación/borrado) | item_system, conveyor_system |
| `BoxTag` | `placement_system` | item_system |
| `SolidTag` | `placement_system`, `game_scene` | physics (colisión) |
| `MachineTag` | `machine_system` | item_system (depositar input), game_scene |
| `PlayerTag` | `player_system` (speed), `enemy_system` (attack_cd) | combat_system, physics |
| `PlayerHealth` | `enemy_system` (daño contacto) | combat_system, debug_ui (HUD) |
| `EnemyTag` | `enemy_system` (dmg_cd, steering), `combat_system` (hp) | wave_system (spawn), physics |
| `NetworkPlayer` | `network` | — |
| `Camera` | `camera_system` | renderer |

---

## Orden de sistemas por tick (50 Hz fixed timestep)

| # | Sistema | Archivo | Escribe |
|---|---|---|---|
| 1 | `player_system` | `systems/player_system.hpp` | `Velocity` (jugador) |
| 2 | `enemy_system` | `systems/enemy_system.hpp` | `Velocity` (enemigos), `PlayerHealth`, `PlayerTag.attack_cd` |
| 3 | `wave_system` | `systems/wave_system.hpp` | Spawna entidades con `EnemyTag` |
| 4 | `conveyor_system` | `systems/conveyor_system.hpp` | `SpriteRef.sprite_id` (corners) |
| 5 | `drill_system` | `systems/drill_system.hpp` | `DrillTag`, spawna `ItemTag` |
| 6 | `item_system` | `systems/item_system.hpp` | `Transform` (items), `MachineTag.input_buf`, destruye items |
| 7 | `machine_system` | `systems/machine_system.hpp` | `MachineTag`, spawna `ItemTag` |
| 8 | `physics` | `core/main.cpp` | `Transform` (todos con `Velocity`) |
| 9 | `camera_system` | `systems/camera_system.hpp` | `Camera` |

## Sistemas por frame (fuera del timestep fijo)

| Sistema | Archivo | Escribe |
|---|---|---|
| `placement_system` | `systems/placement_system.hpp` | `PlacementGrid`, entidades nuevas/borradas |
| `combat_system` | `systems/combat_system.hpp` | `EnemyTag.hp`, `PlayerTag.attack_cd`, destruye enemigos |

---

## PlacementGrid

Estructura de datos compartida (no un sistema ECS). Vive en `main.cpp`.
- **Escribe**: `placement_system` (colocar/borrar), `game_scene` (init)
- **Lee**: `item_system`, physics (colisión indirecta), `drill_system` (reconnect)
- Los enemigos NO van al grid — se mueven libremente.

---

## Pendientes del GDD

| Pendiente | GDD sección | Prioridad |
|---|---|---|
| `EquipmentTag` — arma/casco/pechera, recoger con E, stats | 3.5, 6.2 | Alta |
| Game Over + pantalla stats / Victoria oleada 10 | 2.2 | Alta |
| Countdown 30s antes de oleada en HUD | 3.6 | Media |
| Tipos de enemigo (goblin/troll/jefe con stats del GDD) | 3.5 | Media |
| Tiles de depósito en mapa (hierro/carbón/cobre) | 5.2 | Media |
| Recetas desde JSON + máquina anvil | 3.3 | Media |
| Sonidos: place, remove, wave_warning, hit_player, enemy_die | 7 | Baja |
| Sprites IA ComfyUI + Flux | Fase G | Baja |
| Save/Load completo (todas las entidades) | Fase H | Baja |
