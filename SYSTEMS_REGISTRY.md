# SYSTEMS_REGISTRY — The Warrior's Way Engine

Registro canónico de qué sistema **escribe** cada componente.
Regla: un componente tiene un único dueño. Leer es libre; escribir solo el dueño.

---

## Componentes y propietarios

| Componente | Propietario (escribe) | Lectores frecuentes |
|---|---|---|
| `Transform` | `physics` (main.cpp), `item_system` (items), `combat_system` (ring spawn) | renderer, enemy_system, camera_system |
| `Velocity` | `player_system`, `enemy_system`, `item_system` | physics |
| `SpriteRef` | `drill_system` (frames), `conveyor_system` (corner frames), `placement_system` (ghost), `player_system` (tint) | renderer |
| `DrillTag` | `drill_system` | placement_system, game_scene |
| `ItemTag` | `item_system` | drill_system (spawn), machine_system (spawn) |
| `ConveyorTag` | `placement_system` (colocación/borrado) | item_system, conveyor_system |
| `BoxTag` | `placement_system`, `game_scene` | item_system, combat_system |
| `SolidTag` | `placement_system`, `game_scene` | physics (colisión) |
| `MachineTag` | `machine_system` (progress, output_ready), `item_system` (input_buf) | placement_system, game_scene |
| `PlayerTag` | `player_system` (speed), `enemy_system` (attack_cd) | combat_system |
| `PlayerHealth` | `enemy_system` (daño contacto, inv_t) | combat_system, debug_ui (HUD) |
| `EquipmentTag` | `combat_system` (pickup/crafteo), `save_load` | enemy_system (defensa), debug_ui |
| `EnemyTag` | `enemy_system` (dmg_cd, steering), `combat_system` (hp daño) | wave_system (spawn), physics |
| `EffectTag` | `combat_system` (spawn ring), main.cpp (tick TTL, destroy) | renderer |
| `NetworkPlayer` | `network` | — |
| `Camera` | `camera_system` | renderer |

---

## Orden de sistemas por tick (50 Hz fixed timestep)

| # | Sistema | Archivo | Escribe |
|---|---|---|---|
| 1 | `player_system` | `systems/player_system.hpp` | `Velocity` (jugador), `SpriteRef.tint` (feedback visual) |
| 2 | `enemy_system` | `systems/enemy_system.hpp` | `Velocity` (enemigos), `PlayerHealth` (daño), `PlayerTag.attack_cd` |
| 3 | `wave_system` | `systems/wave_system.hpp` | Spawna entidades con `Transform`, `SpriteRef`, `Velocity`, `EnemyTag` |
| 4 | `conveyor_system` | `systems/conveyor_system.hpp` | `SpriteRef.sprite_id` (corners frame 0-7) |
| 5 | `drill_system` | `systems/drill_system.hpp` | `DrillTag.frame/anim_t/spawn_timer`, spawna `ItemTag` |
| 6 | `item_system` | `systems/item_system.hpp` | `Transform` + `Velocity` (items), `MachineTag.input_buf`, destruye items |
| 7 | `machine_system` | `systems/machine_system.hpp` | `MachineTag.progress/output_ready`, spawna `ItemTag` |
| 8 | `physics` | `core/main.cpp` | `Transform` (todos con `Velocity`), colisión eje-separado |
| 9 | `EffectTag tick` | `core/main.cpp` | Decrementa `EffectTag.ttl`, destruye entidades expiradas |
| 10 | `camera_system` | `systems/camera_system.hpp` | `Camera` (lerp hacia jugador) |

## Sistemas por frame (fuera del timestep fijo)

| Sistema | Archivo | Escribe |
|---|---|---|
| `placement_system` | `systems/placement_system.hpp` | `PlacementGrid`, entidades nuevas/borradas, `DrillTag.active/dest` |
| `combat_system` | `systems/combat_system.hpp` | `EnemyTag.hp`, `EquipmentTag`, spawna `EffectTag` ring, destruye enemigos |

---

## PlacementGrid

Estructura de datos compartida (no un sistema ECS). Vive en `main.cpp`.
- **Escribe**: `placement_system` (colocar/borrar), `game_scene` (init)
- **Lee**: `item_system`, physics (colisión), `wave_system` (skip tiles ocupados)
- Los enemigos NO van al grid — se mueven libremente.

---

## Lógica de `find_chain_dest` (placement_system)

Recorre desde el taladro en la dirección de cada cinta:
- `ConveyorTag` → avanza en `ctag.direction`
- `MachineTag` → avanza en `mtag.out_dir` (atraviesa la máquina)
- `BoxTag` → destino encontrado, activa el taladro
- Tile vacío o entidad desconocida → cadena rota, taladro inactivo

---

## Recetas (machine_system)

| Receta ID | Input | Output | Ticks (50 Hz) |
|---|---|---|---|
| `forge` | 2× iron_ore | 1× iron_ingot | 100 (= 2 s) |

---

## Crafteo en cofre (combat_system, tecla E)

Requiere estar a ≤1.5 tiles de un `BoxTag`. Consume `iron_ingot` del inventario:

| Resultado | Coste |
|---|---|
| `espada_hierro` | 5 iron_ingot |
| `casco_hierro` | 3 iron_ingot |
| `pechera_hierro` | 8 iron_ingot |

Prioridad: arma > casco > pechera (primer slot vacío que se pueda costear).
Llama `apply_equipment_effects` al equipar (recalcula max_hp y speed).

---

## Pendientes del GDD

| Pendiente | GDD sección | Prioridad |
|---|---|---|
| Contador de items sobre el cofre (world-space UI) | — | Alta |
| Más recetas + máquina anvil | 3.3 | Alta |
| Sonidos: place, remove, enemy_die, wave_warning | 7 | Media |
| Tiles de depósito en mapa (hierro/carbón/cobre) | 5.2 | Media |
| Sprites IA ComfyUI + Flux | Fase G | Baja |
| Multiplayer sync de entidades de juego | — | Baja |
| Save/Load completo (entidades del mapa en runtime) | — | Baja |

## Bugs conocidos

| Bug | Síntoma | Causa probable |
|---|---|---|
| Enemigos se apilan | Varios enemigos en el mismo tile | No hay colisión enemy↔enemy |
| save_load parcial | Al cargar, el mapa vuelve al JSON inicial | Solo se serializa inventory/wave/equip |
