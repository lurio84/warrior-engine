# Systems Registry — The Warrior's Way

## Orden de ejecución en el game loop (50 Hz)

| Orden | Sistema | Archivo | Lee | Escribe |
|---|---|---|---|---|
| 1 | input snapshot | core/input.hpp | SDL keyboard | InputSystem (singleton) |
| 2 | player_system | systems/player_system.hpp | InputSystem, PlayerTag | Velocity |
| 3 | conveyor_system | systems/conveyor_system.hpp | ConveyorTag | — (stub) |
| 4 | drill_system | systems/drill_system.hpp | Transform, DrillTag | SpriteRef, crea ItemTag |
| 5 | item_system | systems/item_system.hpp | Transform, ItemTag, Velocity | Transform (scale), Velocity, destruye entidades |
| 6 | machine_system | systems/machine_system.hpp | MachineTag, ItemTag | MachineTag, ItemTag (stub) |
| 7 | physics | core/main.cpp | Transform, Velocity | Transform |
| 8 | network_system | core/network.hpp | Transform, NetworkPlayer | Transform (remoto) |
| 9 | render_system | core/renderer.hpp | Transform, SpriteRef, Camera | — |
| 10 | debug_ui_system | core/debug_ui.hpp | todo (solo lectura) | — |

## Reglas

- Ningún sistema escribe un componente que otro sistema también escribe en el mismo tick
- Physics (paso 7) es siempre posterior a todos los sistemas de gameplay
- Render y debug son siempre los últimos
- network_system solo escribe Transform de entidades remotas (`NetworkPlayer.is_local == false`)
- Si añades un sistema nuevo, añade una fila aquí antes de escribir el código

## Componentes y su dueño (quién escribe en runtime)

| Componente | Dueño |
|---|---|
| Transform.scale_x/y | item_system (pop-in) |
| Transform.x/y | physics (aplicando Velocity) / network_system (remotos) |
| Velocity | player_system, conveyor_system (futuro), item_system |
| SpriteRef.sprite_id | drill_system (frames de animación) |
| SpriteRef.layer | item_system (promoción de layer al salir del drill) |
| DrillTag | drill_system |
| ItemTag | item_system |
| MachineTag | machine_system |
| PlayerTag | solo inicialización |
| ConveyorTag | solo inicialización |
| NetworkPlayer | NetworkManager (inicialización y remoto) |
| SpriteRef.tint/scroll | solo inicialización |

## Notas de diseño

- Los sistemas de gameplay son header-only (`inline`) — sin compilación separada
- `item_system` retorna `int` (items colectados ese tick) — el caller acumula el total
- `drill_system` crea entidades directamente en el registry — no hay factory externa
- `game_scene.hpp` es el único punto de creación de entidades de escena — init idempotente si se llama con registry limpio
