#pragma once
#include <glm/glm.hpp>
#include <string>
#include <cstdint>

// ECS component definitions — The Warrior's Way Engine
// Coordinates: tile integers (world), pixels only in renderer

namespace components {

// ── Transform ─────────────────────────────────────────────────────────────────
// World-space position in tile units (floats allow sub-tile interpolation
// during render, but logic snaps to integers — enforced at the Lua layer)
struct Transform {
    float x        = 0.f;  // tile column
    float y        = 0.f;  // tile row
    float rotation = 0.f;  // radians, CCW
    float scale_x  = 1.f;  // tile multiplier
    float scale_y  = 1.f;
};

// ── SpriteRef ─────────────────────────────────────────────────────────────────
// Atlas key + visual properties. sprite_id is resolved to a UV rect
// in Phase 5 (atlas loader). Until then, the renderer uses tint only.
struct SpriteRef {
    std::string sprite_id;              // e.g. "conveyor_belt_idle"
    glm::vec4   tint{1.f, 1.f, 1.f, 1.f};  // RGBA multiplier / solid color
    int         size_w   = 1;          // sprite width  in tiles
    int         size_h   = 1;          // sprite height in tiles
    float       scroll_x = 0.f;        // UV cycles/second (horizontal)
    float       scroll_y = 0.f;        // UV cycles/second (vertical)
    int         layer    = 0;          // render order: menor = más atrás
};

// ── Velocity ──────────────────────────────────────────────────────────────────
// Sistema de movimiento: C++ aplica vx/vy a Transform cada tick.
// Lua setea la velocidad en update(dt) según input.
struct Velocity {
    float vx = 0.f;   // tiles/segundo
    float vy = 0.f;
};

// ── NetworkPlayer ─────────────────────────────────────────────────────────────
// Marca una entidad como jugador de red. Creada por NetworkManager, no por Lua.
struct NetworkPlayer {
    uint8_t peer_id  = 0;
    bool    is_local = true;
};

} // namespace components
