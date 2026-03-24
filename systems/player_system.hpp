#pragma once
#include <entt/entt.hpp>
#include "components.hpp"
#include "input.hpp"

// player_system — moves PlayerTag entities with WASD, smooth per-tick
inline void player_system(entt::registry& reg, const InputSystem& input) {
    auto view = reg.view<components::Velocity, components::PlayerTag>();
    for (auto [entity, vel, player] : view.each()) {
        vel.vx = 0.f;
        vel.vy = 0.f;
        if (input.down("d")) vel.vx =  player.speed;
        if (input.down("a")) vel.vx = -player.speed;
        if (input.down("w")) vel.vy =  player.speed;
        if (input.down("s")) vel.vy = -player.speed;
    }
}
