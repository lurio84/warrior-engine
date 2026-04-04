#pragma once
#include <entt/entt.hpp>
#include "components.hpp"
#include "placement_grid.hpp"
#include "audio.hpp"
#include <vector>
#include <cmath>
#include <algorithm>
#include <map>
#include <string>

static constexpr float ITEM_POP_DURATION = 0.25f;

// Direction vectors: 0=E, 1=S, 2=W, 3=N  (horario)
static constexpr float BELT_DVX[] = { 1.f,  0.f, -1.f,  0.f };
static constexpr float BELT_DVY[] = { 0.f, -1.f,  0.f,  1.f };

// item_system — owner of Transform (movement) for ItemTag entities
// Handles pop-in animation, belt travel, and collection detection.
// Accumulates collected items into inventory by type.
inline void item_system(entt::registry& reg, float dt,
                        std::map<std::string, int>& inventory,
                        const PlacementGrid& grid,
                        Audio& audio) {
    std::vector<entt::entity> to_destroy;

    auto view = reg.view<components::Transform,
                         components::SpriteRef,
                         components::Velocity,
                         components::ItemTag>();

    for (auto [entity, tf, sr, vel, item] : view.each()) {
        item.age += dt;

        if (item.popping) {
            // ── Pop-in: scale 0.05 → 1.0 with ease-out ───────────────────
            float t    = std::min(item.age / ITEM_POP_DURATION, 1.f);
            float ease = 1.f - (1.f - t) * (1.f - t);
            float s    = 0.05f + (1.f - 0.05f) * ease;
            tf.scale_x = s;
            tf.scale_y = s;

            if (t >= 1.f) {
                item.popping = false;
                sr.layer = 5;  // encima de cintas al salir del taladro
            }
        } else {
            // ── Belt travel: direction from belt tile under the item ───────
            // Si no hay cinta en el tile actual, busca un tile adelante
            // (cubre la transición drill→cinta donde round() devuelve el drill)
            int tile_x = static_cast<int>(std::round(tf.x));
            int tile_y = static_cast<int>(std::round(tf.y));
            auto tile_e = grid.get(tile_x, tile_y);

            // ── Colección: si estamos en el tile del cofre, recoger ───────
            if (tile_e != entt::null && reg.all_of<components::BoxTag>(tile_e)) {
                to_destroy.push_back(entity);
                inventory[item.item_type] += item.quantity;
                audio.play("item_pickup");
                continue;
            }
            // ── Máquina: depositar en buffer de entrada ───────────────────
            if (tile_e != entt::null && reg.all_of<components::MachineTag>(tile_e)) {
                auto& machine = reg.get<components::MachineTag>(tile_e);
                machine.input_buf[item.item_type] += item.quantity;
                to_destroy.push_back(entity);
                continue;
            }

            auto belt_e = tile_e;
            if (belt_e == entt::null || !reg.all_of<components::ConveyorTag>(belt_e)) {
                // Lookahead solo si hay una entidad bajo el item (ej: taladro).
                // Si el tile está vacío (cinta borrada), el item se para.
                if (tile_e != entt::null) {
                    int ax = tile_x + (vel.vx > 0.f ? 1 : vel.vx < 0.f ? -1 : 0);
                    int ay = tile_y + (vel.vy > 0.f ? 1 : vel.vy < 0.f ? -1 : 0);
                    belt_e = grid.get(ax, ay);
                }
            }
            if (belt_e != entt::null && reg.all_of<components::ConveyorTag>(belt_e)) {
                auto& ctag = reg.get<components::ConveyorTag>(belt_e);
                vel.vx = item.belt_speed * BELT_DVX[ctag.direction];
                vel.vy = item.belt_speed * BELT_DVY[ctag.direction];
            } else {
                vel.vx = 0.f;
                vel.vy = 0.f;
            }

            // ── Colección por proximidad (safety net) ─────────────────────
            float ddx = tf.x - item.dest_x;
            float ddy = tf.y - item.dest_y;
            if (ddx*ddx + ddy*ddy <= 0.25f) {
                to_destroy.push_back(entity);
                inventory[item.item_type] += item.quantity;
                audio.play("item_pickup");
            }
        }
    }

    for (auto e : to_destroy)
        reg.destroy(e);
}
