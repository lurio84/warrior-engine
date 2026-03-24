#pragma once
#include <entt/entt.hpp>
#include "components.hpp"

// machine_system — owner of MachineTag and ItemTag (output side)
// Processes input items into output items according to recipe_id.
// Currently a stub — no recipes defined yet.
inline void machine_system([[maybe_unused]] entt::registry& reg,
                            [[maybe_unused]] float dt) {
    // Placeholder — processor machine logic (iron_ore → iron_ingot) goes here.
}
