#pragma once
#include <entt/entt.hpp>
#include <cmath>
#include <algorithm>
#include "components.hpp"

// ── enemy_system ──────────────────────────────────────────────────────────────
// Per fixed-tick (dt = TICK_SECONDS):
//   1. Steers each enemy toward the player by setting its Velocity.
//   2. Ticks player attack cooldown (PlayerTag.attack_cd).
//   3. Applies contact damage to player when an enemy is within 0.7 tiles.
//      Contact damage has its own per-enemy cooldown (EnemyTag.dmg_cd).
//      Player has an invincibility window (PlayerHealth.inv_t) after each hit.
//
// Actual movement (velocity → transform) is applied in main.cpp's physics loop,
// which also does axis-separated solid-tile collision for EnemyTag entities.
inline void enemy_system(entt::registry& reg, float dt) {
    // ── Locate player ─────────────────────────────────────────────────────────
    entt::entity player_e = entt::null;
    float px = 0.f, py = 0.f;

    for (auto [e, tf, ptag] : reg.view<components::Transform,
                                       components::PlayerTag>().each()) {
        player_e  = e;
        px        = tf.x;
        py        = tf.y;
        ptag.attack_cd = std::max(0.f, ptag.attack_cd - dt);
    }
    if (player_e == entt::null) return;

    // ── Tick player invincibility & get HP ptr ────────────────────────────────
    components::PlayerHealth* player_hp = nullptr;
    if (reg.all_of<components::PlayerHealth>(player_e)) {
        player_hp = &reg.get<components::PlayerHealth>(player_e);
        player_hp->inv_t = std::max(0.f, player_hp->inv_t - dt);
    }

    // ── Update each enemy ─────────────────────────────────────────────────────
    for (auto [e, tf, vel, et] : reg.view<components::Transform,
                                          components::Velocity,
                                          components::EnemyTag>().each()) {
        float dx   = px - tf.x;
        float dy   = py - tf.y;
        float dist = std::hypot(dx, dy);

        // Steer toward player
        if (dist > 0.05f) {
            vel.vx = et.speed * dx / dist;
            vel.vy = et.speed * dy / dist;
        } else {
            vel.vx = vel.vy = 0.f;
        }

        // Contact damage
        et.dmg_cd = std::max(0.f, et.dmg_cd - dt);
        if (dist < 0.7f && et.dmg_cd <= 0.f && player_hp && player_hp->inv_t <= 0.f) {
            player_hp->hp   -= et.contact_dmg;
            player_hp->inv_t = 0.8f;   // 0.8 s invincibility after hit
            et.dmg_cd        = 0.5f;
        }
    }
}
