#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <algorithm>
#include "components.hpp"
#include "input.hpp"

// player_system — moves PlayerTag entities with WASD, smooth per-tick.
// También aplica tint de flash de ataque: amarillo brillante al atacar,
// rojo parpadeante al recibir daño (usando PlayerHealth.inv_t).
inline void player_system(entt::registry& reg, const InputSystem& input) {
    for (auto [entity, vel, player] :
         reg.view<components::Velocity, components::PlayerTag>().each()) {
        vel.vx = 0.f;
        vel.vy = 0.f;
        if (input.down("d")) vel.vx =  player.speed;
        if (input.down("a")) vel.vx = -player.speed;
        if (input.down("w")) vel.vy =  player.speed;
        if (input.down("s")) vel.vy = -player.speed;

        // ── Tint feedback ─────────────────────────────────────────────────
        if (!reg.all_of<components::SpriteRef>(entity)) continue;
        auto& sr = reg.get<components::SpriteRef>(entity);

        // Ataque: flash amarillo mientras attack_cd > 0.3s
        if (player.attack_cd > 0.3f) {
            float t = (player.attack_cd - 0.3f) / 0.1f;  // 0→1
            sr.tint = glm::vec4{1.f, 0.85f + 0.15f*t, 0.f, 1.f};
            continue;
        }
        // Daño recibido: flash rojo mientras inv_t > 0
        if (reg.all_of<components::PlayerHealth>(entity)) {
            auto& ph = reg.get<components::PlayerHealth>(entity);
            if (ph.inv_t > 0.f) {
                float pulse = std::sin(ph.inv_t * 20.f) * 0.5f + 0.5f;
                sr.tint = glm::vec4{1.f, pulse * 0.3f, pulse * 0.3f, 1.f};
                continue;
            }
        }
        sr.tint = glm::vec4{1.f, 1.f, 1.f, 1.f};
    }
}
