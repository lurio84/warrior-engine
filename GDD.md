# The Warrior's Way — Game Design Document Técnico
> Versión 1.0 — Documento vivo

---

## 1. Visión del juego

The Warrior's Way es un factory game de supervivencia con combate en tiempo real. El jugador extrae recursos del suelo, construye una cadena de producción automatizada y forja armas y armaduras para sobrevivir oleadas de enemigos que atacan periódicamente.

**La tensión central:** la fábrica es tu defensa. Si no produces suficiente equipo antes de la siguiente oleada, mueres. El loop obliga al jugador a equilibrar expansión de la fábrica con preparación para el combate.

**Diferencia clave respecto a Factorio y Mindustry:** el jugador es una entidad activa con stats (vida, ataque, defensa) que puede morir. La optimización de la fábrica no es el fin, es el medio para sobrevivir.

---

## 2. Loop de juego

### 2.1 Ciclo básico

| Fase | Duración aprox. | Objetivo del jugador |
|---|---|---|
| Extracción | 0-2 min | Colocar taladros sobre depósitos de mineral |
| Producción | 2-5 min | Conectar cintas, máquinas y cofres |
| Equipamiento | 1 min | Recoger equipo del cofre y equiparlo |
| Oleada | 1-3 min | Sobrevivir el ataque de enemigos |
| Expansión | Variable | Ampliar la fábrica antes de la siguiente oleada |

### 2.2 Condición de victoria y derrota

- Victoria: sobrevivir N oleadas (configurable en engine_config.json, default 10)
- Derrota: vida del jugador llega a 0
- Game over muestra estadísticas: oleadas sobrevividas, items producidos, tiempo jugado

### 2.3 Escalado de oleadas

| Oleada | Enemigos | Nuevo recurso desbloqueado |
|---|---|---|
| 1 | 3 goblins básicos | Hierro |
| 2 | 5 goblins | Carbón (necesario para forja) |
| 3 | 3 goblins + 1 troll | Cobre |
| 5 | Oleada mixta | Mithril |
| 7 | Jefe menor | Gemas |
| 10 | Jefe final | Victoria |

---

## 3. Sistemas de gameplay

### 3.1 Sistema de extracción

Los taladros (`DrillTag`) extraen mineral del tile donde están colocados. Solo funcionan sobre tiles de tipo recurso definidos en el mapa JSON.

| Parámetro | Valor | Donde se configura |
|---|---|---|
| Frecuencia de spawn | 1 item cada 2 segundos | DrillTag.spawn_timer |
| Animación | 8 frames loop | DrillTag.frame / anim_timer |
| Radio de depósito | Tile ocupado por el taladro | PlacementGrid |
| Activación | Automática al conectar a una cinta | drill_system.hpp |

### 3.2 Sistema de transporte

Las cintas (`ConveyorTag`) mueven items en la dirección configurada. El sprite cambia automáticamente según los vecinos para mostrar rectas y esquinas.

| Dirección | Valor | Sprite |
|---|---|---|
| Este | 0 | conveyor_e |
| Sur | 1 | conveyor_s |
| Oeste | 2 | conveyor_w |
| Norte | 3 | conveyor_n |
| Esquina CW | auto | conveyor_corner_cw + rotación |
| Esquina CCW | auto | conveyor_corner_ccw + rotación |

> **Pendiente crítico:** rediseñar `make_conveyor_corner_cw/ccw` en `tools/gen_test_sprites.py` para que los brazos del sprite alineen con los rieles de la cinta recta (RAIL en y=0..3 y y=28..31).

### 3.3 Sistema de producción (machine_system)

Las máquinas consumen items de su input, aplican una receta y depositan el resultado en su output. Toda la lógica es data-driven — las recetas se definen en JSON.

#### Recetas — fase 1 (MVP)

| Input | Cantidad | Output | Tiempo (ticks a 50Hz) | Máquina |
|---|---|---|---|---|
| hierro_raw | 2 | hierro_lingote | 100 (2s) | forge |
| carbon | 1 | carbon_procesado | 50 (1s) | crusher |
| hierro_lingote + carbon_procesado | 1+1 | espada_hierro | 200 (4s) | anvil |
| hierro_lingote x3 | 3 | casco_hierro | 300 (6s) | anvil |
| hierro_lingote x4 | 4 | pechera_hierro | 400 (8s) | anvil |

#### Formato JSON de receta

```json
{
  "id": "espada_hierro",
  "inputs": [
    { "item": "hierro_lingote", "qty": 1 },
    { "item": "carbon_procesado", "qty": 1 }
  ],
  "output": { "item": "espada_hierro", "qty": 1 },
  "ticks": 200,
  "machine": "anvil"
}
```

### 3.4 Sistema de items

Los items (`ItemTag`) son entidades ECS que se mueven por las cintas. El sistema aplica una animación pop-in al aparecer (escala 0.05 a 1 en 20 ticks) y siguen la cadena de cintas hasta llegar a un `BoxTag` o una máquina con input disponible.

| Item | ID | Sprite | Fase de aparición |
|---|---|---|---|
| Mineral de hierro | hierro_raw | item_hierro_raw | Oleada 1 |
| Carbón | carbon | item_carbon | Oleada 2 |
| Mineral de cobre | cobre_raw | item_cobre_raw | Oleada 3 |
| Lingote de hierro | hierro_lingote | item_hierro_lingote | Producción |
| Espada de hierro | espada_hierro | item_espada_hierro | Producción |
| Casco de hierro | casco_hierro | item_casco_hierro | Producción |
| Pechera de hierro | pechera_hierro | item_pechera_hierro | Producción |

### 3.5 Sistema de combate

Nuevo sistema a implementar. Los enemigos aparecen en los bordes del mapa cada N oleadas y hacen pathfinding directo hacia el jugador. El jugador ataca con el arma equipada actualmente.

#### Stats del jugador

| Stat | Base | Con espada hierro | Con casco hierro | Con pechera hierro |
|---|---|---|---|---|
| Vida | 100 | 100 | 120 | 150 |
| Ataque | 5 | 25 | 5 | 5 |
| Defensa | 0 | 0 | 5 | 10 |
| Velocidad | 3 tiles/s | 3 tiles/s | 3 tiles/s | 2.5 tiles/s |

#### Stats de enemigos

| Enemigo | Vida | Ataque | Velocidad | Aparece en oleada |
|---|---|---|---|---|
| Goblin | 30 | 8 | 2 tiles/s | 1 |
| Troll | 80 | 20 | 1 tile/s | 3 |
| Jefe menor | 200 | 35 | 1.5 tiles/s | 7 |
| Jefe final | 500 | 50 | 2 tiles/s | 10 |

### 3.6 Sistema de oleadas

Nuevo sistema a implementar. Un `WaveManager` singleton controla el timer de oleadas y el spawn de enemigos desde los bordes del mapa.

| Parámetro | Valor | Donde se configura |
|---|---|---|
| Tiempo entre oleadas | 120 segundos (6000 ticks) | engine_config.json wave_interval_ticks |
| Aviso previo a oleada | 30 segundos antes | HUD countdown |
| Punto de spawn | Borde del mapa (random) | wave_system.hpp |
| Pathfinding | Directo al jugador (sin obstáculos en MVP) | enemy_system.hpp |

---

## 4. Arquitectura técnica

### 4.1 Stack

| Librería | Versión | Uso |
|---|---|---|
| C++20 | — | Lenguaje principal |
| EnTT | 3.13.2 | ECS |
| OpenGL 4.5 + GLAD | 0.1.36 | Renderer |
| SDL2 | 2.30.9 | Ventana, input, contexto GL |
| ImGui | 1.91.0 | Debug UI en runtime |
| ENet | 1.3.17 | Multiplayer UDP |
| miniaudio | 0.11.21 | Audio |
| nlohmann/json | 3.11.3 | Parsing de JSON |
| stb_image | master | Carga de atlas PNG |
| efsw | 1.4.0 | Hot-reload de shaders y atlas |

### 4.2 Componentes ECS

```cpp
Transform      { x, y, rot, scale_x, scale_y }
SpriteRef      { sprite_id, layer, scroll_x, scroll_y, tint }
Velocity       { vx, vy }
DrillTag       { active, dest_x, dest_y, belt_speed, spawn_timer, frame, anim_timer }
ItemTag        { item_type, popping, source/dest_x/y, belt_speed }
ConveyorTag    { direction, speed }
BoxTag         { marker — recoge items que llegan }
SolidTag       { marker — bloquea al jugador }
NetworkPlayer  { peer_id, is_local }
EnemyTag       { hp, attack, speed, target_x, target_y }    // PENDIENTE
EquipmentTag   { weapon_id, helmet_id, chest_id }            // PENDIENTE
WaveTag        { wave_number, enemies_remaining }            // PENDIENTE
```

### 4.3 Orden de sistemas por tick (50 Hz)

| Orden | Sistema | Archivo | Lee | Escribe |
|---|---|---|---|---|
| 1 | input_system | core/input.hpp | SDL events | InputState |
| 2 | player_system | systems/player_system.hpp | InputState | Velocity |
| 3 | enemy_system | systems/enemy_system.hpp | Transform (jugador) | Velocity (enemigos) |
| 4 | wave_system | systems/wave_system.hpp | WaveTag | Spawns enemigos |
| 5 | conveyor_system | systems/conveyor_system.hpp | ConveyorTag, ItemTag | Velocity (items) |
| 6 | drill_system | systems/drill_system.hpp | DrillTag | Spawns items |
| 7 | item_system | systems/item_system.hpp | ItemTag, Velocity | Transform (items) |
| 8 | machine_system | systems/machine_system.hpp | MachineTag, ItemTag | MachineTag, ItemTag |
| 9 | physics_system | core/main.cpp | Velocity, SolidTag | Transform |
| 10 | combat_system | systems/combat_system.hpp | EnemyTag, EquipmentTag | HP jugador y enemigos |
| 11 | camera_system | core/camera.hpp | Transform (jugador) | Camera singleton |
| 12 | render_system | core/renderer.hpp | Transform, SpriteRef | — |
| 13 | debug_ui_system | core/debug_ui.hpp | Todo (lectura) | — |

### 4.4 Layers de render

| Layer | Qué contiene |
|---|---|
| 1 | Cintas (ConveyorTag) |
| 2 | Cofres, máquinas, items durante pop-in |
| 3 | Taladros |
| 5 | Items en tránsito (tras pop-in) |
| 8 | Enemigos |
| 10 | Jugador |
| 15 | Proyectiles / efectos de combate |
| 20 | Ghost preview de construcción |
| 50 | HUD / UI en mundo |

### 4.5 Convenciones técnicas inamovibles

- Y crece hacia arriba (math, no screen) — `DIR_DY = {0, -1, 0, 1}`
- Tile size: 32px — inmutable, definido en `Renderer::TILE_SIZE_PX`
- Tick rate: 50 Hz (20ms/tick) — fixed timestep
- `to_tile(float)` usa `round()`, no `floor()` — tiles centrados en su coordenada
- IDs de sprite: `snake_case` — nunca renombrar una vez en un save
- Único escritor por componente — ver `SYSTEMS_REGISTRY.md`
- `dot(u_scroll, u_scroll) > 0` para detectar scroll en shader (no `sum > 0`)

---

## 5. Mapa y niveles

### 5.1 Formato del mapa JSON

```json
{
  "version": 1,
  "width": 32,
  "height": 32,
  "spawn_x": 16,
  "spawn_y": 16,
  "wave_spawns": [
    { "x": 0,  "y": 16 },
    { "x": 31, "y": 16 },
    { "x": 16, "y": 0  },
    { "x": 16, "y": 31 }
  ],
  "tiles": [
    { "x": 5,  "y": 5,  "type": "hierro_deposit", "qty": 50 },
    { "x": 10, "y": 8,  "type": "carbon_deposit",  "qty": 30 }
  ]
}
```

### 5.2 Tipos de tile

| Tipo | Sprite | Interacción |
|---|---|---|
| floor | tile_floor | Transitable, construible |
| wall | tile_wall | SolidTag — bloquea jugador y enemigos |
| hierro_deposit | tile_hierro_deposit | Solo taladros pueden colocarse aquí |
| carbon_deposit | tile_carbon_deposit | Solo taladros pueden colocarse aquí |
| cobre_deposit | tile_cobre_deposit | Solo taladros pueden colocarse aquí |

---

## 6. HUD y UI

### 6.1 HUD permanente

- Barra de vida del jugador (esquina superior izquierda)
- Inventario de equipo activo: arma, casco, pechera (iconos en esquina superior derecha)
- Countdown a siguiente oleada (centro superior cuando quedan menos de 30s)
- Número de oleada actual

### 6.2 UI de construcción

- `1` — Cinta (ConveyorTag)
- `2` — Taladro (DrillTag)
- `3` — Cofre (BoxTag)
- `4` — Forja (MachineTag tipo forge) — PENDIENTE
- `5` — Yunque (MachineTag tipo anvil) — PENDIENTE
- `R` — Rotar pieza seleccionada
- Clic izquierdo — Colocar
- Clic derecho — Demoler
- Ghost preview en layer 20 mostrando la pieza antes de colocar

### 6.3 UI en mundo

- Contador de items sobre cada cofre (PENDIENTE — sonido pickup ya cargado)
- Barra de progreso sobre máquinas en producción
- Indicador de oleada incoming en los bordes del mapa

---

## 7. Audio

| Sonido | ID | Cuándo suena | Estado |
|---|---|---|---|
| Pickup item | pickup | Item entra en BoxTag | Cargado, falta llamarlo |
| Construcción | place | Al colocar pieza | PENDIENTE |
| Demolición | remove | Al demoler pieza | PENDIENTE |
| Oleada incoming | wave_warning | 30s antes de oleada | PENDIENTE |
| Golpe jugador | hit_player | Enemigo golpea al jugador | PENDIENTE |
| Muerte enemigo | enemy_die | Enemigo llega a 0 HP | PENDIENTE |
| Game over | game_over | Vida jugador llega a 0 | PENDIENTE |
| Victoria | victory | Oleada 10 completada | PENDIENTE |

---

## 8. Fases de implementación — prompts autónomos

> Antes de cada fase: pegar ENGINE_STATE.md + SYSTEMS_REGISTRY.md actualizados junto con este GDD.

---

### FASE A — Fix sprites de esquinas de cinta

**Criterio de éxito:** las esquinas de cinta conectan visualmente con los rieles de las cintas rectas sin huecos ni desalineación.

```
Lee GDD.md, ENGINE_STATE.md y SYSTEMS_REGISTRY.md. Tu objetivo es corregir
los sprites de esquinas de cinta en tools/gen_test_sprites.py. El problema:
los brazos del sprite de esquina no alinean con los rieles de la cinta recta
(RAIL en y=0..3 y y=28..31). Regenera el atlas con tools/build_atlas.py tras
cada cambio. Itera hasta que las esquinas conecten visualmente con las cintas
rectas sin huecos ni desalineación. No me pidas confirmación entre pasos.
Actualiza ENGINE_STATE.md al terminar.
```

---

### FASE B — Sonido pickup

**Criterio de éxito:** se escucha el sonido pickup cada vez que un item entra en un BoxTag.

```
Lee GDD.md y ENGINE_STATE.md. El sonido pickup está cargado pero no se llama.
Añade la llamada a audio en item_system cuando un ItemTag llega a un BoxTag.
Itera hasta que compile y el sonido suene al recoger items. No me pidas
confirmación entre pasos. Actualiza ENGINE_STATE.md al terminar.
```

---

### FASE C — machine_system con recetas

**Criterio de éxito:** una forja conectada a cintas de hierro produce lingotes automáticamente.

```
Lee GDD.md, ENGINE_STATE.md y SYSTEMS_REGISTRY.md. Implementa machine_system
completo. Las recetas se cargan desde game/recipes/*.json — crear el directorio
y las recetas de hierro según la tabla del GDD sección 3.3. La máquina consume
inputs, espera los ticks definidos en la receta y deposita el output. MachineTag
necesita: type, progress, recipe_id. Añade forge y anvil al sistema de
construcción (teclas 4 y 5). Itera hasta que compile y una forja produzca
lingotes de hierro visiblemente en pantalla. No me pidas confirmación entre
pasos. Actualiza ENGINE_STATE.md y SYSTEMS_REGISTRY.md al terminar.
```

---

### FASE D — Tilemap desde JSON

**Criterio de éxito:** el mapa se carga desde game/levels/level_01.json y los tiles de depósito se renderizan con sprite distinto al suelo.

```
Lee GDD.md y ENGINE_STATE.md. Implementa la carga de mapas desde JSON según
el schema definido en el GDD sección 5.1. Crea game/levels/level_01.json con
un mapa 32x32 con depósitos de hierro y carbón. El renderer dibuja tiles de
suelo y tiles de depósito con sprites distintos. Los tiles de depósito son los
únicos donde se pueden colocar taladros — placement_system debe validarlo.
Itera hasta que el mapa se cargue y renderice correctamente. No me pidas
confirmación entre pasos. Actualiza ENGINE_STATE.md al terminar.
```

---

### FASE E — Sistema de oleadas

**Criterio de éxito:** enemigos aparecen en los bordes del mapa cada 2 minutos y caminan hacia el jugador.

```
Lee GDD.md, ENGINE_STATE.md y SYSTEMS_REGISTRY.md. Implementa wave_system y
enemy_system. WaveManager singleton: timer configurable en engine_config.json
(wave_interval_ticks, default 6000). Aviso en HUD 30s antes de la oleada.
Enemigos aparecen en los wave_spawns del mapa JSON con EnemyTag: hp, attack,
speed según la tabla del GDD sección 3.5. Pathfinding directo al jugador sin
obstáculos (MVP). HUD muestra número de oleada y countdown. Itera hasta que
los enemigos aparezcan y caminen hacia el jugador visiblemente. No me pidas
confirmación entre pasos. Actualiza ENGINE_STATE.md y SYSTEMS_REGISTRY.md
al terminar.
```

---

### FASE F — Sistema de combate y equipamiento

**Criterio de éxito:** el jugador ataca enemigos con clic, los enemigos tienen HP visible, el jugador puede morir y aparece pantalla de game over.

```
Lee GDD.md, ENGINE_STATE.md y SYSTEMS_REGISTRY.md. Implementa combat_system
y EquipmentTag. Clic izquierdo fuera del modo construcción = ataque en el tile
apuntado (rango 1.5 tiles). Los enemigos mueren al llegar a 0 HP (sonido
enemy_die). El jugador pierde HP al recibir golpes (sonido hit_player). Si HP
jugador llega a 0: pantalla de game over con estadísticas (oleadas sobrevividas,
items producidos, tiempo jugado). El jugador recoge equipo del BoxTag con tecla
E y lo equipa (EquipmentTag). Los stats del jugador cambian según la tabla del
GDD sección 3.5. Itera hasta que el loop completo funcione: fábrica produce
equipo → jugador lo equipa → enemigos llegan → combate resuelve vida. No me
pidas confirmación entre pasos. Actualiza ENGINE_STATE.md y SYSTEMS_REGISTRY.md
al terminar.
```

---

### FASE G — Sprites IA con ComfyUI

**Criterio de éxito:** los sprites procedurales se reemplazan por sprites generados con FLUX.1 dev en la RTX 5070 y el atlas se recarga en caliente.

```
Lee GDD.md y ENGINE_STATE.md. Crea tools/comfyui_pipeline.py que envía prompts
a la API local de ComfyUI (localhost:8188) para generar sprites top-down 32x32
pixel art para cada ID de sprite del atlas. Los PNG se guardan en assets/raw/.
Luego ejecuta tools/build_atlas.py para regenerar el atlas. El File Watcher
recarga en caliente sin reiniciar el motor. Prompts base: taladro = 'top-down
32x32 pixel art drill machine, dark metal, mining, factory game sprite, black
background'. Cinta = 'top-down 32x32 pixel art conveyor belt, metal rails,
factory game sprite, black background'. Itera hasta que el pipeline genere y
cargue todos los sprites sin errores. No me pidas confirmación entre pasos.
Actualiza ENGINE_STATE.md al terminar.
```

---

### FASE H — Save / Load

**Criterio de éxito:** F5 guarda y F9 carga la partida completa sin perder ninguna entidad ni estado.

```
Lee GDD.md y ENGINE_STATE.md. Implementa save/load del registry entt a JSON
en game/saves/. F5 = guardar slot activo. F9 = cargar slot activo. El archivo
save incluye: schema_version, timestamp, lista de entidades con todos sus
componentes serializados, oleada actual, HP del jugador y equipo equipado.
Al cargar, reconstruir el registry completo desde el JSON. Si schema_version
no coincide con la versión actual, ejecutar migrators en cadena antes de cargar.
Itera hasta que guardar y cargar funcione sin perder ninguna entidad, item en
tránsito ni estado de máquina. No me pidas confirmación entre pasos.
Actualiza ENGINE_STATE.md al terminar.
```

---

## 9. Pendientes críticos — orden de prioridad

1. Fix esquinas de cinta (Fase A) — bloqueante visual
2. Sonido pickup (Fase B) — rápido, audio ya cargado
3. machine_system (Fase C) — sin esto no hay loop de producción completo
4. Tilemap JSON (Fase D) — sin esto el mapa es hardcoded
5. Sistema de oleadas (Fase E) — sin esto no hay presión de tiempo
6. Combate (Fase F) — sin esto no hay victoria ni derrota
7. Sprites IA con ComfyUI (Fase G) — puede hacerse en cualquier momento
8. Save/Load (Fase H) — necesario para partidas largas

---

## 10. Reglas de oro para desarrollo con Claude Code

- Siempre pegar `ENGINE_STATE.md` + `SYSTEMS_REGISTRY.md` + `GDD.md` al inicio de cada sesión
- Siempre terminar la sesión pidiendo: *"Actualiza ENGINE_STATE.md y SYSTEMS_REGISTRY.md con lo que hemos hecho"*
- Un sistema = un archivo en `/systems/`. Nunca mezclar lógica de dos sistemas
- Un componente tiene un único dueño (sistema que escribe). Consultarlo en SYSTEMS_REGISTRY.md antes de crear un sistema nuevo
- IDs de sprite son inmutables una vez en un save. Añadir alias, nunca renombrar
- Cualquier cambio de schema JSON lleva su migrator en el mismo commit
- Criterio de éxito concreto en cada prompt. Sin criterio, Claude no sabe cuándo parar
