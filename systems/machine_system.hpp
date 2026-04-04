#pragma once
#include <entt/entt.hpp>
#include <string>
#include <vector>
#include <map>
#include "components.hpp"
#include "placement_grid.hpp"

// ── Recetas ───────────────────────────────────────────────────────────────────
struct Recipe {
    std::vector<std::pair<std::string,int>> inputs;
    std::string output_item;
    int         output_qty  = 1;
    int         ticks       = 100;   // a 50 Hz
};

inline const std::map<std::string, Recipe>& recipes() {
    static const std::map<std::string, Recipe> TABLE = {
        // forge: 2× iron_ore → 1× iron_ingot  (2 s)
        { "forge", { {{"iron_ore", 2}}, "iron_ingot", 1, 100 } },
    };
    return TABLE;
}

// ── Dirección de spawn del output ─────────────────────────────────────────────
static constexpr int MDVX[] = { 1, 0, -1, 0 };
static constexpr int MDVY[] = { 0,-1,  0, 1 };

// ── machine_system ────────────────────────────────────────────────────────────
// Orden de operaciones por tick:
//   1. Si output_ready → intentar spawnar item en el tile de salida
//   2. Si processing   → avanzar progress; al completar → output_ready = true
//   3. Si idle         → si buffer tiene los inputs suficientes → iniciar proceso
inline void machine_system(entt::registry& reg, float dt,
                            const PlacementGrid& grid) {
    static constexpr float TICK_S = 1.f / 50.f;   // 50 Hz

    for (auto [entity, tf, machine] :
         reg.view<components::Transform, components::MachineTag>().each())
    {
        auto it = recipes().find(machine.recipe_id);
        if (it == recipes().end()) continue;
        const Recipe& rec = it->second;

        // 1. Output listo → intentar spawnar
        if (machine.output_ready) {
            int ox = static_cast<int>(std::round(tf.x)) + MDVX[machine.out_dir];
            int oy = static_cast<int>(std::round(tf.y)) + MDVY[machine.out_dir];
            auto tile_e = grid.get(ox, oy);
            // Solo spawnar si hay una cinta en el tile de salida
            if (tile_e != entt::null && reg.all_of<components::ConveyorTag>(tile_e)) {
                auto item_e = reg.create();
                reg.emplace<components::Transform>(item_e,
                    components::Transform{ (float)ox, (float)oy, 0.f, 0.4f, 0.4f });
                reg.emplace<components::SpriteRef>(item_e,
                    components::SpriteRef{ rec.output_item, {1,1,1,1}, 1, 1, 0.f, 0.f, 5 });
                auto& ct = reg.get<components::ConveyorTag>(tile_e);
                components::ItemTag itag;
                itag.item_type  = rec.output_item;
                itag.quantity   = rec.output_qty;
                itag.belt_speed = ct.speed;
                itag.popping    = true;
                itag.source_x   = (float)ox;
                itag.source_y   = (float)oy;
                itag.dest_x     = (float)ox;   // item_system actualizará dest
                itag.dest_y     = (float)oy;
                reg.emplace<components::ItemTag>(item_e, itag);
                reg.emplace<components::Velocity>(item_e);
                machine.output_ready = false;
            }
            continue;
        }

        // 2. Procesando → avanzar
        if (machine.processing) {
            machine.progress += 1.f;
            if (machine.progress >= static_cast<float>(rec.ticks)) {
                machine.progress   = 0.f;
                machine.processing = false;
                machine.output_ready = true;
            }
            continue;
        }

        // 3. Idle → comprobar buffer
        bool has_inputs = true;
        for (auto& [item_id, qty] : rec.inputs) {
            auto buf_it = machine.input_buf.find(item_id);
            if (buf_it == machine.input_buf.end() || buf_it->second < qty) {
                has_inputs = false;
                break;
            }
        }
        if (has_inputs) {
            for (auto& [item_id, qty] : rec.inputs)
                machine.input_buf[item_id] -= qty;
            machine.processing = true;
            machine.progress   = 0.f;
        }
    }
}
