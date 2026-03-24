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
    int           orientation = 0;  // 0=E 1=N 2=W 3=S (para futuras cintas)
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

    ptf.x        = static_cast<float>(tx);
    ptf.y        = static_cast<float>(ty);
    psr.sprite_id = item.sprite;
    psr.size_w   = 1;
    psr.size_h   = 1;
    psr.layer    = 20;  // por encima de todo

    bool occupied = grid.has(tx, ty);
    psr.tint = occupied
        ? glm::vec4{ 1.f, 0.2f, 0.2f, 0.55f }   // rojo: ocupado
        : glm::vec4{ 1.f, 1.f,  1.f,  0.65f };  // blanco semitransparente

    // ── Colocar (clic izquierdo) ──────────────────────────────────────────────
    if (input.mouse_pressed(SDL_BUTTON_LEFT) && !occupied) {
        auto e     = reg.create();
        auto& tf   = reg.emplace<components::Transform>(e);
        tf.x       = static_cast<float>(tx);
        tf.y       = static_cast<float>(ty);

        auto& sr   = reg.emplace<components::SpriteRef>(e);
        sr.sprite_id = item.sprite;
        sr.size_w  = 1;
        sr.size_h  = 1;
        sr.layer   = 2;

        if (item.solid)
            reg.emplace<components::SolidTag>(e);

        if (state.selected == 0)  // cinta
            reg.emplace<components::ConveyorTag>(e);

        grid.place(tx, ty, e);
    }

    // ── Eliminar (clic derecho) ───────────────────────────────────────────────
    if (input.mouse_pressed(SDL_BUTTON_RIGHT)) {
        auto e = grid.get(tx, ty);
        if (e != entt::null) {
            reg.destroy(e);
            grid.remove(tx, ty);
        }
    }
}
