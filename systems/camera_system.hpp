#pragma once
#include <entt/entt.hpp>
#include "components.hpp"
#include "camera.hpp"

// camera_system — interpola la cámara hacia el PlayerTag activo.
inline void camera_system(entt::registry& reg, Camera& cam, float dt) {
    auto view = reg.view<components::Transform, components::PlayerTag>();
    for (auto [entity, tf, player] : view.each()) {
        constexpr float LERP = 6.f;
        cam.x += (tf.x - cam.x) * LERP * dt;
        cam.y += (tf.y - cam.y) * LERP * dt;
    }
}
