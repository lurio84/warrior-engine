-- scripts/main.lua — Phase 12: Drill animado
engine.log("main.lua cargado")

-- ── Tilemap ────────────────────────────────────────────────────────────────
engine.tilemap.load("assets/maps/test.json")
local map_w = engine.tilemap.width()
local map_h = engine.tilemap.height()
engine.camera.set(map_w / 2 - 0.5, map_h / 2 - 0.5)
engine.camera.zoom(3.0)

-- ── Helpers ────────────────────────────────────────────────────────────────
local function spawn(x, y, sprite, w, h, layer)
    w = w or 1; h = h or 1; layer = layer or 0
    local id = engine.entity.create()
    engine.entity.set_pos(id, x, y)
    engine.entity.set_size(id, w, h)
    engine.entity.set_sprite(id, sprite)
    engine.entity.set_layer(id, layer)
    return id
end

local function lerp(a, b, t) return a + (b - a) * t end
local function clamp(v, lo, hi) return math.max(lo, math.min(hi, v)) end
local function ease_out(t) return 1 - (1-t)*(1-t) end

-- ── Escena ─────────────────────────────────────────────────────────────────
local BELT_Y     = math.floor(map_h / 2)
local BELT_SPEED = 1.5

-- Drill encima de la cinta (layer 3 > belt layer 1)
local DRILL_X = math.floor(map_w / 2) - 6
local drill = spawn(DRILL_X, BELT_Y, "drill_0", 1, 1, 3)

-- Cinta: empieza bajo el drill, termina justo antes del cofre
local BOX_X  = DRILL_X + 12
-- Cinta: del borde derecho del drill (DRILL_X+0.5) al borde izquierdo del cofre (BOX_X-0.5)
local BELT_W  = BOX_X - DRILL_X - 1
local belt_cx = (DRILL_X + BOX_X) / 2   -- centro exacto entre ambos
local belt = spawn(belt_cx, BELT_Y, "conveyor_belt_idle", BELT_W, 1, 1)
engine.entity.set_scroll(belt, BELT_SPEED, 0.0)

-- Caja receptora — cinta llega hasta su borde izquierdo
local box = spawn(BOX_X, BELT_Y, "item_box", 1, 1, 2)

-- Jugador
local player = spawn(DRILL_X - 3, BELT_Y - 3, "player", 1, 1, 10)
engine.entity.set_velocity(player, 0, 0)

-- ── Estado del drill ───────────────────────────────────────────────────────
local SPAWN_EVERY    = 2.0
local POP_DURATION   = 0.25
local FLASH_DURATION = 0.12

local spawn_timer  = SPAWN_EVERY * 0.7
local drill_flash  = 0.0
local drill_frame  = 0          -- 0 o 1
local drill_anim_t = 0.0        -- acumulador de animación

local items    = {}
local collected = 0

local DRILL_FRAMES = { "drill_0","drill_1","drill_2","drill_3","drill_4","drill_5","drill_6","drill_7" }

local function do_spawn()
    drill_flash = FLASH_DURATION

    local id = engine.entity.create()
    engine.entity.set_pos(id, DRILL_X, BELT_Y)
    engine.entity.set_sprite(id, "item")
    engine.entity.set_layer(id, 2)   -- bajo el drill (layer 3) al salir
    engine.entity.set_scale(id, 0.05, 0.05)
    engine.entity.set_color(id, 1, 1, 1, 1)
    engine.entity.set_velocity(id, BELT_SPEED, 0)
    table.insert(items, { id=id, age=0, popping=true })
end

-- ── Animación del drill ────────────────────────────────────────────────────
-- En reposo: broca lenta (frame cada ~0.4s)
-- Cargando: broca se acelera cuanto más cargado
-- Al disparar: flash blanco y reset

local function update_drill(dt, charge)
    local frame_time = 0.18   -- velocidad constante (~0.7 seg/rotación)
    drill_anim_t = drill_anim_t + dt
    if drill_anim_t >= frame_time then
        drill_anim_t = drill_anim_t - frame_time
        drill_frame = (drill_frame + 1) % #DRILL_FRAMES
        engine.entity.set_sprite(drill, DRILL_FRAMES[drill_frame + 1])
    end

    -- Sin inflate ni wobble — escala siempre 1.0
    engine.entity.set_scale(drill, 1.0, 1.0)
    engine.entity.set_color(drill, 1, 1, 1, 1)
    drill_flash = math.max(0, drill_flash - dt)
end

-- ── Tick ───────────────────────────────────────────────────────────────────
local SPEED = 5.0

function update(dt)
    -- Jugador
    local vx, vy = 0, 0
    if engine.input.down("d") then vx =  SPEED end
    if engine.input.down("a") then vx = -SPEED end
    if engine.input.down("w") then vy =  SPEED end
    if engine.input.down("s") then vy = -SPEED end
    engine.entity.set_velocity(player, vx, vy)

    -- Timer de spawn
    spawn_timer = spawn_timer + dt
    local charge = clamp(spawn_timer / SPAWN_EVERY, 0, 1)

    if spawn_timer >= SPAWN_EVERY then
        spawn_timer = 0
        do_spawn()
    end

    update_drill(dt, charge)

    -- Actualizar items
    local dead = {}
    for i, item in ipairs(items) do
        item.age = item.age + dt

        if item.popping then
            local t = clamp(item.age / POP_DURATION, 0, 1)
            local s = lerp(0.1, 1.0, ease_out(t))
            engine.entity.set_scale(item.id, s, s)

            -- Sube al layer de la cinta cuando sale físicamente del drill
            local ix = engine.entity.get_pos(item.id)
            if ix > DRILL_X + 0.5 then
                engine.entity.set_layer(item.id, 5)
            end

            if t >= 1.0 then
                item.popping = false
            end
        else
            local x, y = engine.entity.get_pos(item.id)

            -- Llegó a la caja → destruir
            if x >= BOX_X - 0.5 then
                engine.entity.destroy(item.id)
                table.insert(dead, i)
                collected = collected + 1
                engine.log("Recogido #" .. collected)
            else
                -- Siempre avanza desde el drill hasta la caja
                engine.entity.set_velocity(item.id, BELT_SPEED, 0)
            end
        end
    end

    for i = #dead, 1, -1 do
        table.remove(items, dead[i])
    end
end
