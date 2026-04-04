#pragma once
#include <entt/entt.hpp>
#include <cmath>
#include <vector>
#include <string>
#include <map>
#include "components.hpp"
#include "input.hpp"

// ── apply_equipment_effects ───────────────────────────────────────────────────
// Recalcula max_hp y velocidad del jugador según el equipo actual.
// Llamar siempre que cambie EquipmentTag (pickup o load).
// HP actual se recalcula proporcionalmente para no hacer heal/kill gratis.
inline void apply_equipment_effects(entt::registry& reg, entt::entity player_e) {
    if (!reg.all_of<components::EquipmentTag,
                    components::PlayerHealth,
                    components::PlayerTag>(player_e))
        return;

    auto& eq = reg.get<components::EquipmentTag>(player_e);
    auto& ph = reg.get<components::PlayerHealth>(player_e);
    auto& pt = reg.get<components::PlayerTag>(player_e);

    float old_max = ph.max_hp;
    float new_max = 100.f;
    if (eq.helmet_id == "casco_hierro")   new_max += 20.f;
    if (eq.chest_id  == "pechera_hierro") new_max += 50.f;

    // Escalar HP actual proporcionalmente
    if (old_max > 0.f)
        ph.hp = ph.hp * (new_max / old_max);
    ph.hp     = std::min(ph.hp, new_max);
    ph.max_hp = new_max;

    // Velocidad
    pt.speed = (eq.chest_id == "pechera_hierro") ? 4.f : 5.f;
}

// ── combat_system ─────────────────────────────────────────────────────────────
// Llamado una vez por frame (fuera del fixed timestep).
//
// Tecla E — pickup de equipo:
//   Si hay un BoxTag con items de equipo en inventario dentro de 1.5 tiles,
//   equipa el primer slot vacío que encuentre (arma > casco > pechera).
//
// Space — ataque melee:
//   Golpea enemigos en radio ATTACK_RANGE con el ataque efectivo del jugador.
//   Cooldown en PlayerTag.attack_cd (ticked por enemy_system).
inline void combat_system(entt::registry& reg,
                          const InputSystem& input,
                          std::map<std::string, int>& inventory) {
    static constexpr float ATTACK_RANGE = 1.5f;
    static constexpr float ATTACK_CD    = 0.4f;
    static constexpr float PICKUP_RANGE = 1.5f;

    // ── Locate player ─────────────────────────────────────────────────────────
    entt::entity player_e = entt::null;
    float px = 0.f, py = 0.f;
    for (auto [e, tf, ptag] : reg.view<components::Transform,
                                       components::PlayerTag>().each()) {
        player_e = e;
        px = tf.x;
        py = tf.y;
    }
    if (player_e == entt::null) return;

    auto& ptag = reg.get<components::PlayerTag>(player_e);

    // ── E: craftear y equipar desde cofre adyacente ──────────────────────────
    // El cofre actúa como estación de crafting:
    //   5× iron_ingot → espada_hierro
    //   3× iron_ingot → casco_hierro
    //   8× iron_ingot → pechera_hierro
    if (input.pressed("e")) {
        // Buscar cofre (BoxTag) en rango — BoxTag es empty, no aparece en structured binding
        entt::entity box_e = entt::null;
        for (auto [e, tf] : reg.view<components::Transform,
                                     components::BoxTag>().each()) {
            float dx = tf.x - px, dy = tf.y - py;
            if (dx*dx + dy*dy <= PICKUP_RANGE * PICKUP_RANGE) {
                box_e = e;
                break;
            }
        }

        if (box_e != entt::null &&
            reg.all_of<components::EquipmentTag>(player_e)) {
            auto& eq      = reg.get<components::EquipmentTag>(player_e);
            int&  ingots  = inventory["iron_ingot"];

            // Recetas: prioridad arma > casco > pechera
            struct CraftRecipe { const char* item; std::string* slot; int cost; };
            CraftRecipe recipes[] = {
                { "espada_hierro",  &eq.weapon_id,  5 },
                { "casco_hierro",   &eq.helmet_id,  3 },
                { "pechera_hierro", &eq.chest_id,   8 },
            };
            for (auto& r : recipes) {
                if (!r.slot->empty()) continue;         // ya equipado
                if (ingots < r.cost) continue;          // no hay suficientes
                *r.slot = r.item;
                ingots -= r.cost;
                if (ingots <= 0) inventory.erase("iron_ingot");
                apply_equipment_effects(reg, player_e);
                std::cout << "[Craft+Equip] " << r.item
                          << " (coste: " << r.cost << " iron_ingot)\n";
                break;
            }
        }
    }

    // ── Space: ataque melee ───────────────────────────────────────────────────
    if (!input.pressed("space") || ptag.attack_cd > 0.f) return;
    ptag.attack_cd = ATTACK_CD;

    // Ataque efectivo según arma
    float effective_atk = 5.f;  // base GDD
    if (reg.all_of<components::EquipmentTag>(player_e)) {
        auto& eq = reg.get<components::EquipmentTag>(player_e);
        if (eq.weapon_id == "espada_hierro") effective_atk = 25.f;
    }

    std::vector<entt::entity> to_destroy;
    for (auto [e, tf, et] : reg.view<components::Transform,
                                     components::EnemyTag>().each()) {
        float dx = tf.x - px, dy = tf.y - py;
        if (dx*dx + dy*dy > ATTACK_RANGE * ATTACK_RANGE) continue;
        et.hp -= effective_atk;
        if (et.hp <= 0.f)
            to_destroy.push_back(e);
    }
    for (auto e : to_destroy)
        reg.destroy(e);

    // ── Anillo visual de ataque (EffectTag) ───────────────────────────────────
    auto ring_e = reg.create();
    auto& rtf   = reg.emplace<components::Transform>(ring_e);
    rtf.x = px; rtf.y = py;
    rtf.scale_x = ATTACK_RANGE * 2.f;
    rtf.scale_y = ATTACK_RANGE * 2.f;
    auto& rsr       = reg.emplace<components::SpriteRef>(ring_e);
    rsr.sprite_id   = "attack_ring";
    rsr.layer       = 15;
    rsr.tint        = glm::vec4{1.f, 0.9f, 0.1f, 0.85f};
    rsr.size_w      = 1;
    rsr.size_h      = 1;
    reg.emplace<components::EffectTag>(ring_e);
}
