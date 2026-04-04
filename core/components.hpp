#pragma once
#include <glm/glm.hpp>
#include <string>
#include <cstdint>
#include <map>

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
    std::string recipe_id   = "forge";    // qué receta procesa
    int         out_dir     = 0;          // dirección de salida (0=E,1=S,2=W,3=N)
    float       progress    = 0.f;        // ticks acumulados de la receta actual
    bool        processing  = false;      // true mientras está procesando
    bool        output_ready = false;     // true cuando el output está listo para spawnar
    std::map<std::string, int> input_buf; // items acumulados en el buffer de entrada
};

// ── PlayerTag ─────────────────────────────────────────────────────────────────
// Marks the local player entity. Owned by player_system.
struct PlayerTag {
    float speed     = 5.0f;
    float attack_cd = 0.f;   // seconds until next melee swing
};

// ── PlayerHealth ──────────────────────────────────────────────────────────────
// Player hit points. Owned by enemy_system (damage) and combat_system.
struct PlayerHealth {
    float hp      = 100.f;
    float max_hp  = 100.f;
    float inv_t   = 0.f;   // invincibility seconds after being hit
};

// ── EquipmentTag ──────────────────────────────────────────────────────────────
// Attached to the player entity. Tracks currently equipped items.
// Empty string = slot vacío.
// Stats efectivos (GDD 3.5):
//   Ataque base=5, espada_hierro→25
//   Defensa base=0, casco_hierro→+5, pechera_hierro→+10
//   HP base=100, casco_hierro→+20 max_hp, pechera_hierro→+50 max_hp
//   Velocidad base=5, pechera_hierro→4
struct EquipmentTag {
    std::string weapon_id = "";   // "espada_hierro" o ""
    std::string helmet_id = "";   // "casco_hierro"  o ""
    std::string chest_id  = "";   // "pechera_hierro" o ""
};

// ── EnemyTag ──────────────────────────────────────────────────────────────────
// Marks a hostile enemy entity. Owned by enemy_system + combat_system.
struct EnemyTag {
    float hp          = 3.f;
    float max_hp      = 3.f;
    float speed       = 2.5f;
    float contact_dmg = 1.f;
    float dmg_cd      = 0.f;   // cooldown before applying contact damage again
};

// ── BoxTag ────────────────────────────────────────────────────────────────────
// Marks a collection box. Items that reach this entity are collected.
struct BoxTag {};

// ── SolidTag ──────────────────────────────────────────────────────────────────
// Marks an entity as colisionable. El jugador no puede atravesarlo.
// Las cintas NO tienen SolidTag (son caminables).
struct SolidTag {};

} // namespace components
