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
// Marca una entidad como jugador de red. Creada por NetworkManager.
struct NetworkPlayer {
    uint8_t peer_id  = 0;
    bool    is_local = true;
};

// ── ConveyorTag ───────────────────────────────────────────────────────────────
// Marks a belt entity. Owner of Velocity writes is conveyor_system.
struct ConveyorTag {
    float speed     = 1.5f;
    int   direction = 0;    // 0=E, 1=N, 2=W, 3=S
};

// ── DrillTag ──────────────────────────────────────────────────────────────────
// Marks a drill entity. Owned by drill_system (writes SpriteRef, spawns items).
struct DrillTag {
    float anim_t      = 0.f;
    int   frame       = 0;
    float spawn_timer = 1.4f;   // pre-filled: 2.0 * 0.7 (first spawn sooner)
    float dest_x      = 0.f;   // x of the collection box
    float dest_y      = 0.f;   // y of the collection box
    float belt_speed  = 1.5f;
    bool  active      = true;  // false if not connected to a box via belts
};

// ── ItemTag ───────────────────────────────────────────────────────────────────
// Marks an item entity on the belt. Owned by item_system.
struct ItemTag {
    std::string item_type  = "iron_ore";
    int         quantity   = 1;
    float       age        = 0.f;
    bool        popping    = true;
    float       source_x   = 0.f;   // drill x (for layer promotion)
    float       source_y   = 0.f;   // drill y
    float       dest_x     = 0.f;   // box x (collection point)
    float       dest_y     = 0.f;   // box y
    float       belt_speed = 1.5f;
};

// ── MachineTag ────────────────────────────────────────────────────────────────
// Marks a processor machine. Owned by machine_system.
struct MachineTag {
    std::string type;
    float       progress  = 0.f;
    std::string recipe_id;
};

// ── PlayerTag ─────────────────────────────────────────────────────────────────
// Marks the local player entity. Owned by player_system.
struct PlayerTag {
    float speed = 5.0f;
};

// ── BoxTag ────────────────────────────────────────────────────────────────────
// Marks a collection box. Items that reach this entity are collected.
struct BoxTag {};

// ── SolidTag ──────────────────────────────────────────────────────────────────
// Marks an entity as colisionable. El jugador no puede atravesarlo.
// Las cintas NO tienen SolidTag (son caminables).
struct SolidTag {};

} // namespace components
