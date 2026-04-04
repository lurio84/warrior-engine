#pragma once
#include <entt/entt.hpp>
#include "components.hpp"
#include "camera.hpp"
#include "input.hpp"
#include "placement_grid.hpp"
#include <glm/glm.hpp>
#include <string>
#include <cmath>

// ── BuildState ────────────────────────────────────────────────────────────────
struct BuildState {
    struct Item {
        std::string sprite;
        std::string label;
        bool        solid;
    };

    static constexpr int ITEM_COUNT = 3;
    const Item ITEMS[ITEM_COUNT] = {
        { "conveyor_belt_idle", "Cinta [1]",   false },
        { "drill_0",            "Taladro [2]",  true  },
        { "item_box",           "Cofre [3]",    true  },
    };

    int           selected    = 0;
    int           orientation = 0;  // 0=E 1=S 2=W 3=N  (horario)
    entt::entity  preview     = entt::null;
    int           hover_x     = 0;
    int           hover_y     = 0;
};

// ── screen_to_tile ────────────────────────────────────────────────────────────
inline std::pair<int,int> screen_to_tile(int sx, int sy,
                                          int vp_w, int vp_h,
                                          const Camera& cam) {
    constexpr float TILE_PX = 32.f;
    float wx = (sx - vp_w * 0.5f) / (TILE_PX * cam.zoom) + cam.x;
    float wy = cam.y - (sy - vp_h * 0.5f) / (TILE_PX * cam.zoom);
    return { static_cast<int>(std::round(wx)),
             static_cast<int>(std::round(wy)) };
}

// ── Direction helpers ─────────────────────────────────────────────────────────
// 0=E 1=S 2=W 3=N  (horario)
static constexpr int DIR_DX[] = { 1,  0, -1,  0 };
static constexpr int DIR_DY[] = { 0, -1,  0,  1 };

// ── belt_visual_for ───────────────────────────────────────────────────────────
// in_dir  = dirección que trae el item (output del tile alimentador)
// out_dir = dirección de salida de este tile (ConveyorTag.direction)
// Devuelve {sprite_id, rotation} para la entidad de cinta.
//
// Sprites de esquina:
//   conveyor_corner_cw  base (rot=0): N→E  (giro derecha: norte entra, sale al este)
//     rot -π/2 : E→S    rot π : S→W    rot π/2 : W→N
//   conveyor_corner_ccw base (rot=0): N→W  (giro izquierda)
//     rot -π/2 : E→N    rot π : S→E    rot π/2 : W→S
struct BeltVisual { std::string sprite; float rotation; };

inline BeltVisual belt_visual_for(int in_dir, int out_dir) {
    // Rotación del sprite recto según dirección de salida
    // 0=E:0°  1=S:-90°  2=W:180°  3=N:90°
    static constexpr float STRAIGHT_ROT[] = { 0.f, -1.5708f, 3.14159f, 1.5708f };

    if (in_dir == out_dir)
        return { "conveyor_belt_idle", STRAIGHT_ROT[out_dir] };

    struct Corner { const char* sprite; float rot; };
    // Filas = in_dir (0=E,1=S,2=W,3=N), columnas = out_dir
    static const Corner TABLE[4][4] = {
        //         out=E                              out=S                            out=W                               out=N
        /* in=E */ {{},                                {"conveyor_corner_cw_0",-1.5708f}, {},                                  {"conveyor_corner_ccw_0",-1.5708f}},
        /* in=S */ {{"conveyor_corner_ccw_0",3.14159f},{},                              {"conveyor_corner_cw_0", 3.14159f},  {}},
        /* in=W */ {{},                                {"conveyor_corner_ccw_0",1.5708f}, {},                                  {"conveyor_corner_cw_0", 1.5708f}},
        /* in=N */ {{"conveyor_corner_cw_0", 0.f},    {},                              {"conveyor_corner_ccw_0",0.f},        {}},
    };

    const auto& c = TABLE[in_dir][out_dir];
    if (c.sprite) return { c.sprite, c.rot };
    return { "conveyor_belt_idle", STRAIGHT_ROT[out_dir] };  // U-turn (no debería pasar)
}

// ── refresh_belt_sprite ───────────────────────────────────────────────────────
// Actualiza el sprite de la cinta en (x,y) según si hay giro o no.
// Busca en los 4 vecinos cuál apunta HACIA este tile para determinar in_dir.
inline void refresh_belt_sprite(entt::registry& reg,
                                 const PlacementGrid& grid,
                                 int x, int y) {
    auto e = grid.get(x, y);
    if (e == entt::null || !reg.all_of<components::ConveyorTag>(e)) return;

    auto& ctag = reg.get<components::ConveyorTag>(e);
    int   out  = ctag.direction;

    // Buscar vecino cuya dirección apunta exactamente a (x, y)
    int in_dir = out;  // sin alimentador → sprite recto
    for (int d = 0; d < 4; ++d) {
        int nx = x + DIR_DX[d];
        int ny = y + DIR_DY[d];
        auto ne = grid.get(nx, ny);
        if (ne == entt::null || !reg.all_of<components::ConveyorTag>(ne)) continue;
        int d2 = reg.get<components::ConveyorTag>(ne).direction;
        // ¿Apunta hacia (x, y)?
        if (nx + DIR_DX[d2] == x && ny + DIR_DY[d2] == y) {
            in_dir = d2;
            break;
        }
    }

    auto vis = belt_visual_for(in_dir, out);
    auto& sr  = reg.get<components::SpriteRef>(e);
    sr.sprite_id = vis.sprite;
    auto& tf     = reg.get<components::Transform>(e);
    tf.rotation  = vis.rotation;
    sr.scroll_x = (vis.sprite == "conveyor_belt_idle") ? ctag.speed : 0.f;
    sr.scroll_y = 0.f;
}

// ── refresh_belt_area ─────────────────────────────────────────────────────────
// Refresca el tile y sus 4 vecinos (para propagar cambios al colocar/borrar).
inline void refresh_belt_area(entt::registry& reg,
                               const PlacementGrid& grid,
                               int x, int y) {
    refresh_belt_sprite(reg, grid, x, y);
    for (int d = 0; d < 4; ++d)
        refresh_belt_sprite(reg, grid, x + DIR_DX[d], y + DIR_DY[d]);
}

// ── find_chain_dest ───────────────────────────────────────────────────────────
// Starting from (from_x, from_y), steps in dir and follows belt directions
// until a BoxTag is found. Returns {dest_x, dest_y, speed, found}.
struct ChainResult { float dest_x, dest_y, speed; bool found; };

inline ChainResult find_chain_dest(int from_x, int from_y, int dir,
                                    const PlacementGrid& grid,
                                    entt::registry& reg) {
    int   cx    = from_x + DIR_DX[dir];
    int   cy    = from_y + DIR_DY[dir];
    float speed = 1.5f;

    for (int step = 0; step < 64; ++step) {
        auto e = grid.get(cx, cy);
        if (e == entt::null) break;

        if (reg.all_of<components::BoxTag>(e))
            return { (float)cx, (float)cy, speed, true };

        if (reg.all_of<components::ConveyorTag>(e)) {
            auto& ctag = reg.get<components::ConveyorTag>(e);
            speed = ctag.speed;
            cx   += DIR_DX[ctag.direction];
            cy   += DIR_DY[ctag.direction];
        } else {
            break;
        }
    }
    return { 0.f, 0.f, 1.5f, false };
}

// ── reconnect_drills ──────────────────────────────────────────────────────────
// Reescanea todos los taladros del registro para actualizar active/dest.
// Llamar al colocar/borrar cualquier cinta o cofre.
inline void reconnect_drills(entt::registry& reg, const PlacementGrid& grid) {
    auto view = reg.view<components::Transform, components::DrillTag>();
    for (auto [e, tf, dtag] : view.each()) {
        int tx = PlacementGrid::to_tile(tf.x);
        int ty = PlacementGrid::to_tile(tf.y);
        ChainResult best{ 0.f, 0.f, 1.5f, false };
        for (int d = 0; d < 4 && !best.found; ++d)
            best = find_chain_dest(tx, ty, d, grid, reg);
        dtag.active     = best.found;
        dtag.dest_x     = best.dest_x;
        dtag.dest_y     = best.dest_y;
        dtag.belt_speed = best.speed;
    }
}

// ── placement_system ──────────────────────────────────────────────────────────
// Llamar una vez por frame (no en el tick fijo).
inline void placement_system(entt::registry& reg,
                              PlacementGrid&  grid,
                              const Camera&   cam,
                              const InputSystem& input,
                              BuildState&     state,
                              int vp_w, int vp_h) {
    // ── Selector de item ──────────────────────────────────────────────────────
    if (input.pressed("1")) state.selected = 0;
    if (input.pressed("2")) state.selected = 1;
    if (input.pressed("3")) state.selected = 2;
    if (input.pressed("r")) state.orientation = (state.orientation + 1) % 4;

    // ── Tile bajo el cursor ───────────────────────────────────────────────────
    auto [mx, my] = input.mouse_pos();
    auto [tx, ty] = screen_to_tile(mx, my, vp_w, vp_h, cam);
    state.hover_x = tx;
    state.hover_y = ty;

    // ── Preview entity (ghost) ────────────────────────────────────────────────
    if (state.preview == entt::null || !reg.valid(state.preview)) {
        state.preview = reg.create();
        reg.emplace<components::Transform>(state.preview);
        reg.emplace<components::SpriteRef>(state.preview);
    }

    const auto& item = state.ITEMS[state.selected];
    auto& ptf        = reg.get<components::Transform>(state.preview);
    auto& psr        = reg.get<components::SpriteRef>(state.preview);

    ptf.x         = static_cast<float>(tx);
    ptf.y         = static_cast<float>(ty);
    psr.sprite_id = item.sprite;
    psr.size_w    = 1;
    psr.size_h    = 1;
    psr.layer     = 20;
    // Mostrar rotación en el preview solo para cintas
    static constexpr float PREV_ROT[] = { 0.f, -1.5708f, 3.14159f, 1.5708f };
    ptf.rotation = (state.selected == 0) ? PREV_ROT[state.orientation] : 0.f;

    bool occupied = grid.has(tx, ty);
    psr.tint = occupied
        ? glm::vec4{ 1.f, 0.2f, 0.2f, 0.55f }
        : glm::vec4{ 1.f, 1.f,  1.f,  0.65f };

    // ── Colocar (clic izquierdo) ──────────────────────────────────────────────
    if (input.mouse_pressed(SDL_BUTTON_LEFT) && !occupied) {
        auto e   = reg.create();
        auto& tf = reg.emplace<components::Transform>(e);
        tf.x     = static_cast<float>(tx);
        tf.y     = static_cast<float>(ty);

        auto& sr      = reg.emplace<components::SpriteRef>(e);
        sr.sprite_id  = item.sprite;
        sr.size_w     = 1;
        sr.size_h     = 1;
        sr.layer      = 2;

        if (item.solid)
            reg.emplace<components::SolidTag>(e);

        // Layers por tipo: cinta=1, cofre=2, taladro=3
        if (state.selected == 0) sr.layer = 1;
        else if (state.selected == 1) sr.layer = 3;
        else sr.layer = 2;

        if (state.selected == 0) {
            // ── Cinta ─────────────────────────────────────────────────────
            auto& ctag     = reg.emplace<components::ConveyorTag>(e);
            ctag.direction = state.orientation;
            ctag.speed     = 1.5f;
            // Scroll siempre positivo en espacio local + rotación del sprite
            sr.scroll_x = ctag.speed;
            sr.scroll_y = 0.f;
            // 0=E:0  1=S:-π/2  2=W:π  3=N:π/2  (horario)
            static constexpr float ROT[] = { 0.f, -1.5708f, 3.14159f, 1.5708f };
            tf.rotation = ROT[state.orientation];

        } else if (state.selected == 1) {
            // ── Taladro ───────────────────────────────────────────────────
            // Busca un cofre conectado en las 4 direcciones
            ChainResult best{ 0.f, 0.f, 1.5f, false };
            for (int d = 0; d < 4 && !best.found; ++d)
                best = find_chain_dest(tx, ty, d, grid, reg);

            auto& dtag       = reg.emplace<components::DrillTag>(e);
            dtag.active      = best.found;
            dtag.dest_x      = best.dest_x;
            dtag.dest_y      = best.dest_y;
            dtag.belt_speed  = best.speed;
            dtag.spawn_timer = 1.4f;

        } else if (state.selected == 2) {
            // ── Cofre ─────────────────────────────────────────────────────
            reg.emplace<components::BoxTag>(e);
        }

        grid.place(tx, ty, e);
        refresh_belt_area(reg, grid, tx, ty);
        reconnect_drills(reg, grid);
    }

    // ── Eliminar (clic derecho) ───────────────────────────────────────────────
    if (input.mouse_pressed(SDL_BUTTON_RIGHT)) {
        auto e = grid.get(tx, ty);
        if (e != entt::null) {
            reg.destroy(e);
            grid.remove(tx, ty);
            refresh_belt_area(reg, grid, tx, ty);
            reconnect_drills(reg, grid);
        }
    }
}
