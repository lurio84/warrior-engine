#pragma once
#include <entt/entt.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <iostream>
#include <map>
#include "components.hpp"
#include "../systems/wave_system.hpp"

// ── save_game ─────────────────────────────────────────────────────────────────
// Persists: inventory, wave number + timer, player position + HP.
// Does NOT save entity layout — that is always re-loaded from the map JSON.
inline void save_game(const std::string& path,
                      const std::map<std::string, int>& inventory,
                      const WaveState& ws,
                      entt::registry& reg) {
    nlohmann::json j;
    j["inventory"]  = inventory;
    j["wave"]       = ws.wave;
    j["wave_timer"] = ws.timer;

    for (auto [e, tf, ptag] :
         reg.view<components::Transform, components::PlayerTag>().each()) {
        j["player_x"] = tf.x;
        j["player_y"] = tf.y;
        if (reg.all_of<components::PlayerHealth>(e)) {
            auto& ph      = reg.get<components::PlayerHealth>(e);
            j["player_hp"]     = ph.hp;
            j["player_max_hp"] = ph.max_hp;
        }
        break;
    }

    std::ofstream f(path);
    if (!f) { std::cerr << "[Save] cannot write: " << path << "\n"; return; }
    f << j.dump(2) << "\n";
    std::cout << "[Save] " << path << "\n";
}

// ── load_game ─────────────────────────────────────────────────────────────────
// Restores: inventory, wave state, player position + HP.
// Returns false if file not found or invalid.
inline bool load_game(const std::string& path,
                      std::map<std::string, int>& inventory,
                      WaveState& ws,
                      entt::registry& reg) {
    std::ifstream f(path);
    if (!f) { std::cerr << "[Load] not found: " << path << "\n"; return false; }

    auto j = nlohmann::json::parse(f, nullptr, false);
    if (j.is_discarded()) { std::cerr << "[Load] invalid JSON\n"; return false; }

    if (j.contains("inventory"))
        inventory = j["inventory"].get<std::map<std::string, int>>();
    if (j.contains("wave"))       ws.wave  = j["wave"].get<int>();
    if (j.contains("wave_timer")) ws.timer = j["wave_timer"].get<float>();

    for (auto [e, tf, ptag] :
         reg.view<components::Transform, components::PlayerTag>().each()) {
        if (j.contains("player_x")) tf.x = j["player_x"].get<float>();
        if (j.contains("player_y")) tf.y = j["player_y"].get<float>();
        if (reg.all_of<components::PlayerHealth>(e)) {
            auto& ph = reg.get<components::PlayerHealth>(e);
            if (j.contains("player_hp"))     ph.hp     = j["player_hp"].get<float>();
            if (j.contains("player_max_hp")) ph.max_hp = j["player_max_hp"].get<float>();
        }
        break;
    }

    std::cout << "[Load] " << path << "\n";
    return true;
}
