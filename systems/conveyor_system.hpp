#pragma once
#include <entt/entt.hpp>
#include "components.hpp"

// conveyor_system — owner of Velocity on ConveyorTag entities
// Currently a stub: belt visual scroll is set at spawn (SpriteRef::scroll_x).
// Future: detect items overlapping the belt zone and drive their velocity.
inline void conveyor_system([[maybe_unused]] entt::registry& reg,
                             [[maybe_unused]] float dt) {
    // Placeholder — belt speed is driven per-item in item_system.
    // This system will own belt→item velocity transfer once belts are
    // placed dynamically.
}
