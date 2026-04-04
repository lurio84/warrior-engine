#pragma once
#include <entt/entt.hpp>
#include <string>
#include "components.hpp"

// conveyor_system — anima los sprites de esquina de cinta.
// Las cintas rectas se animan via UV scroll en el renderer (scroll_x).
// Las esquinas no pueden usar UV scroll (son arcos), así que ciclamos
// entre 8 frames pregenerados (conveyor_corner_cw_0 .. _7).
// total_time: mismo reloj que renderer.set_time() → UV scroll y frames en sincronía.
// La cinta recta completa un ciclo UV en 1/scroll_x = 1/1.5 ≈ 0.667s.
// Con N=8 frames y FPS = N·scroll_x = 12 la esquina cicla exactamente igual.
inline void conveyor_system(entt::registry& reg, float total_time) {
    static constexpr int   N          = 8;
    static constexpr float SCROLL_X   = 1.5f;   // igual que ConveyorTag.speed
    // frame = floor(total_time · scroll_x · N) % N  →  sincronizado con UV scroll
    int frame = static_cast<int>(total_time * SCROLL_X * N) % N;

    for (auto e : reg.view<components::ConveyorTag, components::SpriteRef>()) {
        auto& sr = reg.get<components::SpriteRef>(e);
        const std::string& sid = sr.sprite_id;

        // Detectar si es esquina: empieza por "conveyor_corner_"
        // Comprobar ccw primero (contiene "cw", evitar falso positivo)
        std::string base;
        if (sid.find("conveyor_corner_ccw") == 0)     base = "conveyor_corner_ccw";
        else if (sid.find("conveyor_corner_cw") == 0) base = "conveyor_corner_cw";
        else continue;  // cinta recta → UV scroll lo maneja el renderer

        sr.sprite_id = base + "_" + std::to_string(frame);
    }
}
