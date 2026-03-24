#pragma once
#include <entt/entt.hpp>
#include <unordered_map>
#include <cstdint>
#include <cmath>

// PlacementGrid — mapa 2D tile → entidad.
// Coordinadas enteras que coinciden con el sistema de tiles del motor:
// la entidad en (x, y) visualmente ocupa (x±0.5, y±0.5).

class PlacementGrid {
public:
    void place(int x, int y, entt::entity e) { cells_[key(x, y)] = e; }
    void remove(int x, int y)               { cells_.erase(key(x, y)); }
    void clear()                            { cells_.clear(); }

    entt::entity get(int x, int y) const {
        auto it = cells_.find(key(x, y));
        return it != cells_.end() ? it->second : entt::null;
    }

    bool has(int x, int y) const { return cells_.count(key(x, y)) > 0; }

    // Convierte posición mundo (float) a coordenada de tile (round, no floor,
    // porque los tiles están centrados en enteros).
    static int to_tile(float v) { return static_cast<int>(std::round(v)); }

private:
    static uint64_t key(int x, int y) {
        return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32)
             |  static_cast<uint32_t>(y);
    }

    std::unordered_map<uint64_t, entt::entity> cells_;
};
