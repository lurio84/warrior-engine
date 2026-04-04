#pragma once
#include <entt/entt.hpp>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <glm/glm.hpp>
#include "components.hpp"
#include "placement_grid.hpp"

static constexpr int MAX_WAVES = 10;   // GDD 2.2

// ── WaveState ─────────────────────────────────────────────────────────────────
struct WaveState {
    int   wave       = 0;
    float timer      = 5.f;    // seconds until next wave (first wave at 5s)
    float interval   = 20.f;
    int   base_count = 3;      // enemies in wave 1; +2 per wave
};

// ── Tipos de enemigo (GDD 3.5) ────────────────────────────────────────────────
struct EnemyType {
    const char* sprite_id;
    float       hp;
    float       contact_dmg;
    float       speed;
    glm::vec4   tint;
};

inline EnemyType select_enemy_type(int wave) {
    if (wave >= 7)  // Jefe menor
        return { "enemy", 200.f, 35.f, 1.5f, {1.0f, 0.5f, 0.0f, 1.f} };
    if (wave >= 3)  // Troll
        return { "enemy",  80.f, 20.f, 1.0f, {0.5f, 0.3f, 1.0f, 1.f} };
    // Goblin
    return          { "enemy",  30.f,  8.f, 2.0f, {0.3f, 0.9f, 0.3f, 1.f} };
}

// ── wave_system ───────────────────────────────────────────────────────────────
// Decrements timer each tick. When it reaches 0, spawns a wave of enemies at
// random positions just inside the map border (row/col 1 and map-2).
// Skips tiles that are already occupied in the PlacementGrid.
inline void wave_system(entt::registry& reg, const PlacementGrid& grid,
                        WaveState& ws, float dt,
                        int map_w, int map_h) {
    ws.timer -= dt;
    if (ws.timer > 0.f) return;

    ++ws.wave;
    ws.timer    = ws.interval;
    int count   = ws.base_count + (ws.wave - 1) * 2;

    // Collect border positions (one tile inside the wall)
    std::vector<std::pair<int,int>> candidates;
    candidates.reserve(2 * (map_w + map_h));
    for (int x = 1; x < map_w - 1; ++x) {
        candidates.push_back({x, 1});            // south edge
        candidates.push_back({x, map_h - 2});    // north edge
    }
    for (int y = 2; y < map_h - 2; ++y) {
        candidates.push_back({1, y});            // west edge
        candidates.push_back({map_w - 2, y});    // east edge
    }

    // Partial Fisher-Yates: pick 'count' positions
    int spawned = 0;
    for (int i = 0; i < count && !candidates.empty(); ++i) {
        int idx = std::rand() % static_cast<int>(candidates.size());
        auto [sx, sy] = candidates[idx];
        // Swap-and-pop (O(1) remove without preserving order)
        candidates[idx] = candidates.back();
        candidates.pop_back();

        // Skip occupied tiles (walls, machines, etc.)
        if (grid.get(sx, sy) != entt::null) {
            --i;   // retry slot
            continue;
        }

        auto e     = reg.create();
        auto& tf   = reg.emplace<components::Transform>(e);
        tf.x = static_cast<float>(sx);
        tf.y = static_cast<float>(sy);

        auto etype = select_enemy_type(ws.wave);
        auto& sr   = reg.emplace<components::SpriteRef>(e);
        sr.sprite_id = etype.sprite_id;
        sr.size_w    = 1;
        sr.size_h    = 1;
        sr.layer     = 8;
        sr.tint      = etype.tint;

        reg.emplace<components::Velocity>(e);
        auto& et   = reg.emplace<components::EnemyTag>(e);
        et.hp          = etype.hp;
        et.max_hp      = etype.hp;
        et.speed       = etype.speed;
        et.contact_dmg = etype.contact_dmg;

        ++spawned;
    }

    std::cout << "[Wave " << ws.wave << "] " << spawned << " enemies\n";
}
