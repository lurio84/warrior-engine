#pragma once
#include <entt/entt.hpp>
#include "components.hpp"
#include <array>
#include <string_view>

static constexpr std::array<std::string_view, 8> DRILL_FRAME_NAMES = {
    "drill_0","drill_1","drill_2","drill_3",
    "drill_4","drill_5","drill_6","drill_7"
};
static constexpr float DRILL_FRAME_TIME  = 0.18f;   // seconds per frame (~0.7s/rotation)
static constexpr float DRILL_SPAWN_EVERY = 2.0f;    // seconds between items

// drill_system — owner of DrillTag
// Animates the drill sprite and spawns ItemTag entities onto the belt.
inline void drill_system(entt::registry& reg, float dt) {
    auto view = reg.view<components::Transform,
                         components::SpriteRef,
                         components::DrillTag>();

    for (auto [entity, tf, sr, drill] : view.each()) {
        if (!drill.active) continue;

        // ── Frame animation ───────────────────────────────────────────────
        drill.anim_t += dt;
        if (drill.anim_t >= DRILL_FRAME_TIME) {
            drill.anim_t -= DRILL_FRAME_TIME;
            drill.frame   = (drill.frame + 1) % 8;
            sr.sprite_id  = DRILL_FRAME_NAMES[drill.frame];
        }

        // ── Item spawn ────────────────────────────────────────────────────
        drill.spawn_timer += dt;
        if (drill.spawn_timer >= DRILL_SPAWN_EVERY) {
            drill.spawn_timer = 0.f;

            auto item = reg.create();

            auto& item_tf    = reg.emplace<components::Transform>(item);
            item_tf.x        = tf.x;
            item_tf.y        = tf.y;
            item_tf.scale_x  = 0.05f;
            item_tf.scale_y  = 0.05f;

            auto& item_sr    = reg.emplace<components::SpriteRef>(item);
            item_sr.sprite_id = "item";
            item_sr.layer    = 2;   // under drill (layer 3) while popping
            item_sr.size_w   = 1;
            item_sr.size_h   = 1;

            auto& vel        = reg.emplace<components::Velocity>(item);
            vel.vx           = drill.belt_speed;
            vel.vy           = 0.f;

            auto& itag       = reg.emplace<components::ItemTag>(item);
            itag.source_x    = tf.x;
            itag.source_y    = tf.y;
            itag.dest_x      = drill.dest_x;
            itag.dest_y      = drill.dest_y;
            itag.belt_speed  = drill.belt_speed;
        }
    }
}
