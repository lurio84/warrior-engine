#pragma once
#include <entt/entt.hpp>
#include "components.hpp"
#include "tilemap.hpp"
#include "camera.hpp"
#include "placement_grid.hpp"
#include <string>
#include <iostream>
#include <map>

// ── Scene state (owned by main) ───────────────────────────────────────────────
struct SceneState {
    float belt_y     = 0.f;
    float drill_x    = 0.f;
    float box_x      = 0.f;
    float belt_speed = 1.5f;
    std::map<std::string, int> inventory;
};

// ── Helper: create a sprite entity ───────────────────────────────────────────
inline entt::entity spawn_sprite(entt::registry& reg,
                                  float x, float y,
                                  const std::string& sprite,
                                  int w, int h, int layer) {
    auto e = reg.create();
    auto& tf     = reg.emplace<components::Transform>(e);
    tf.x = x; tf.y = y;

    auto& sr     = reg.emplace<components::SpriteRef>(e);
    sr.sprite_id = sprite;
    sr.size_w    = w;
    sr.size_h    = h;
    sr.layer     = layer;
    return e;
}

// ── init_scene ────────────────────────────────────────────────────────────────
// Mirrors the Phase 12 main.lua setup: drill + belt + box + player.
// base_dir must end with '/'.
inline SceneState init_scene(entt::registry& reg,
                              TileMap&        tilemap,
                              Camera&         cam,
                              PlacementGrid&  grid,
                              const std::string& base_dir) {
    // ── Tilemap ───────────────────────────────────────────────────────────────
    tilemap.load(base_dir + "assets/maps/test.json");
    int map_w = tilemap.width();
    int map_h = tilemap.height();

    cam.x    = map_w / 2.f - 0.5f;
    cam.y    = map_h / 2.f - 0.5f;
    cam.zoom = 3.0f;

    // ── Layout constants ──────────────────────────────────────────────────────
    SceneState sc;
    sc.belt_speed = 1.5f;
    sc.belt_y     = static_cast<float>(map_h / 2);
    sc.drill_x    = static_cast<float>(map_w / 2 - 6);
    sc.box_x      = sc.drill_x + 12.f;

    // ── Drill (layer 3, animated by drill_system) ─────────────────────────────
    auto drill_e = spawn_sprite(reg, sc.drill_x, sc.belt_y, "drill_0", 1, 1, 3);
    auto& dtag   = reg.emplace<components::DrillTag>(drill_e);
    dtag.dest_x      = sc.box_x;
    dtag.dest_y      = sc.belt_y;
    dtag.belt_speed  = sc.belt_speed;
    dtag.spawn_timer = 1.4f;
    reg.emplace<components::SolidTag>(drill_e);
    grid.place(static_cast<int>(sc.drill_x), static_cast<int>(sc.belt_y), drill_e);

    // ── Belt: un tile por entidad (para que se puedan borrar individualmente) ─
    for (int bx = static_cast<int>(sc.drill_x) + 1; bx < static_cast<int>(sc.box_x); ++bx) {
        auto be = spawn_sprite(reg, static_cast<float>(bx), sc.belt_y,
                               "conveyor_belt_idle", 1, 1, 1);
        auto& sr_be   = reg.get<components::SpriteRef>(be);
        sr_be.scroll_x = sc.belt_speed;
        auto& ct      = reg.emplace<components::ConveyorTag>(be);
        ct.speed      = sc.belt_speed;
        ct.direction  = 0;  // Este
        grid.place(bx, static_cast<int>(sc.belt_y), be);
    }

    // ── Box (layer 2) ─────────────────────────────────────────────────────────
    auto box_e = spawn_sprite(reg, sc.box_x, sc.belt_y, "item_box", 1, 1, 2);
    reg.emplace<components::SolidTag>(box_e);
    reg.emplace<components::BoxTag>(box_e);
    grid.place(static_cast<int>(sc.box_x), static_cast<int>(sc.belt_y), box_e);

    // ── Player (layer 10) ─────────────────────────────────────────────────────
    auto player_e = spawn_sprite(reg,
                                  sc.drill_x - 3.f, sc.belt_y - 3.f,
                                  "player", 1, 1, 10);
    reg.emplace<components::Velocity>(player_e);
    reg.emplace<components::PlayerTag>(player_e);

    std::cout << "[Scene] init — drill=" << sc.drill_x
              << " box=" << sc.box_x
              << " belt_y=" << sc.belt_y << "\n";

    return sc;
}
