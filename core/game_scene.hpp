#pragma once
#include <entt/entt.hpp>
#include "components.hpp"
#include "tilemap.hpp"
#include "camera.hpp"
#include "placement_grid.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <iostream>
#include <map>

// ── Scene state (owned by main) ───────────────────────────────────────────────
struct SceneState {
    float belt_speed         = 1.5f;
    int   total_items_produced = 0;   // acumulador para pantalla de game over
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
// Loads entity layout from the "objects" array in the map JSON.
// base_dir must end with '/'.
inline SceneState init_scene(entt::registry& reg,
                              TileMap&        tilemap,
                              Camera&         cam,
                              PlacementGrid&  grid,
                              const std::string& base_dir) {
    std::string map_path = base_dir + "assets/maps/test.json";

    // ── Tilemap (tile rendering layer) ────────────────────────────────────────
    tilemap.load(map_path);
    int map_w = tilemap.width();
    int map_h = tilemap.height();

    cam.x    = map_w / 2.f - 0.5f;
    cam.y    = map_h / 2.f - 0.5f;
    cam.zoom = 3.0f;

    SceneState sc;

    // ── Parse objects from JSON ───────────────────────────────────────────────
    std::ifstream f(map_path);
    if (!f) {
        std::cerr << "[Scene] cannot open map: " << map_path << "\n";
        return sc;
    }
    auto j = nlohmann::json::parse(f, nullptr, false);
    if (j.is_discarded() || !j.contains("objects")) {
        std::cerr << "[Scene] no 'objects' key in map JSON\n";
        return sc;
    }

    int spawned = 0;
    for (auto& obj : j["objects"]) {
        std::string type = obj.value("type", "");
        float x = static_cast<float>(obj.value("x", 0));
        float y = static_cast<float>(obj.value("y", 0));
        int   xi = static_cast<int>(x);
        int   yi = static_cast<int>(y);

        if (type == "drill") {
            float dest_x     = static_cast<float>(obj.value("dest_x", 0));
            float dest_y     = static_cast<float>(obj.value("dest_y", 0));
            float belt_speed = static_cast<float>(obj.value("belt_speed", 1.5));
            sc.belt_speed = belt_speed;

            auto e    = spawn_sprite(reg, x, y, "drill_0", 1, 1, 3);
            auto& dt  = reg.emplace<components::DrillTag>(e);
            dt.dest_x      = dest_x;
            dt.dest_y      = dest_y;
            dt.belt_speed  = belt_speed;
            dt.spawn_timer = 1.4f;
            reg.emplace<components::SolidTag>(e);
            grid.place(xi, yi, e);

        } else if (type == "conveyor") {
            int   direction = obj.value("direction", 0);
            float speed     = static_cast<float>(obj.value("speed", 1.5));

            auto e     = spawn_sprite(reg, x, y, "conveyor_belt_idle", 1, 1, 1);
            auto& sr   = reg.get<components::SpriteRef>(e);
            sr.scroll_x = speed;
            auto& ct   = reg.emplace<components::ConveyorTag>(e);
            ct.speed     = speed;
            ct.direction = direction;
            grid.place(xi, yi, e);

        } else if (type == "forge") {
            std::string recipe  = obj.value("recipe", "forge");
            int         out_dir = obj.value("out_dir", 0);

            auto e    = spawn_sprite(reg, x, y, "forge", 1, 1, 3);
            auto& mt  = reg.emplace<components::MachineTag>(e);
            mt.recipe_id = recipe;
            mt.out_dir   = out_dir;
            reg.emplace<components::SolidTag>(e);
            grid.place(xi, yi, e);

        } else if (type == "box") {
            auto e = spawn_sprite(reg, x, y, "item_box", 1, 1, 2);
            reg.emplace<components::SolidTag>(e);
            reg.emplace<components::BoxTag>(e);
            grid.place(xi, yi, e);

        } else if (type == "player") {
            auto e = spawn_sprite(reg, x, y, "player", 1, 1, 10);
            reg.emplace<components::Velocity>(e);
            reg.emplace<components::PlayerTag>(e);
            reg.emplace<components::PlayerHealth>(e);
            reg.emplace<components::EquipmentTag>(e);

        } else {
            std::cerr << "[Scene] unknown object type: " << type << "\n";
            continue;
        }
        ++spawned;
    }

    std::cout << "[Scene] loaded " << spawned << " objects from " << map_path << "\n";
    return sc;
}
