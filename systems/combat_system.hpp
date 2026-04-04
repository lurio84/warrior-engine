#pragma once
#include <entt/entt.hpp>
#include <cmath>
#include <vector>
#include "components.hpp"
#include "input.hpp"

// ── combat_system ─────────────────────────────────────────────────────────────
// Called once per frame (NOT inside fixed timestep) so pressed() edge is
// consumed exactly once per keypress regardless of how many ticks ran.
//
// Space bar = melee swing:
//   - Hits all EnemyTag entities within ATTACK_RANGE tiles.
//   - Each hit deals ATTACK_DMG.  Enemies at 0 HP are destroyed immediately.
//   - 0.4 s cooldown between swings (tracked in PlayerTag.attack_cd, ticked
//     by enemy_system at 50 Hz so the value is in seconds, not frames).
inline void combat_system(entt::registry& reg, const InputSystem& input) {
    static constexpr float ATTACK_RANGE = 1.5f;
    static constexpr float ATTACK_DMG   = 1.f;
    static constexpr float ATTACK_CD    = 0.4f;

    // ── Locate player ─────────────────────────────────────────────────────────
    entt::entity player_e = entt::null;
    float px = 0.f, py = 0.f;
    for (auto [e, tf, ptag] : reg.view<components::Transform,
                                       components::PlayerTag>().each()) {
        player_e = e;
        px = tf.x;
        py = tf.y;
    }
    if (player_e == entt::null) return;

    auto& ptag = reg.get<components::PlayerTag>(player_e);
    if (!input.pressed("space") || ptag.attack_cd > 0.f) return;

    ptag.attack_cd = ATTACK_CD;

    // ── Collect enemies in range and deal damage ───────────────────────────────
    std::vector<entt::entity> to_destroy;
    for (auto [e, tf, et] : reg.view<components::Transform,
                                     components::EnemyTag>().each()) {
        float dx = tf.x - px;
        float dy = tf.y - py;
        if (dx*dx + dy*dy > ATTACK_RANGE * ATTACK_RANGE) continue;

        et.hp -= ATTACK_DMG;
        if (et.hp <= 0.f)
            to_destroy.push_back(e);
    }

    for (auto e : to_destroy)
        reg.destroy(e);
}
