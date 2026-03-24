#pragma once
#include <entt/entt.hpp>
#include "components.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

static constexpr float ITEM_POP_DURATION = 0.25f;

// item_system — owner of Transform (movement) for ItemTag entities
// Handles pop-in animation, belt travel, and collection detection.
// Returns the number of items collected this tick.
inline int item_system(entt::registry& reg, float dt) {
    int collected = 0;
    std::vector<entt::entity> to_destroy;

    auto view = reg.view<components::Transform,
                         components::SpriteRef,
                         components::Velocity,
                         components::ItemTag>();

    for (auto [entity, tf, sr, vel, item] : view.each()) {
        item.age += dt;

        if (item.popping) {
            // ── Pop-in: scale 0.05 → 1.0 with ease-out ───────────────────
            float t    = std::min(item.age / ITEM_POP_DURATION, 1.f);
            float ease = 1.f - (1.f - t) * (1.f - t);
            float s    = 0.05f + (1.f - 0.05f) * ease;
            tf.scale_x = s;
            tf.scale_y = s;

            // Promote layer when item physically exits the drill
            if (tf.x > item.source_x + 0.5f)
                sr.layer = 5;

            if (t >= 1.f)
                item.popping = false;
        } else {
            // ── Belt travel ───────────────────────────────────────────────
            vel.vx = item.belt_speed;
            vel.vy = 0.f;

            if (tf.x >= item.dest_x - 0.5f) {
                to_destroy.push_back(entity);
                ++collected;
            }
        }
    }

    for (auto e : to_destroy)
        reg.destroy(e);

    return collected;
}
