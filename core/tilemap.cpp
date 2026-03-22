#include "tilemap.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

const std::string TileMap::EMPTY_ = "";

// ── load ──────────────────────────────────────────────────────────────────────

void TileMap::load(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        std::cerr << "[TileMap] cannot open: " << path << "\n";
        return;
    }

    nlohmann::json j = nlohmann::json::parse(f, nullptr, false);
    if (j.is_discarded()) {
        std::cerr << "[TileMap] invalid JSON: " << path << "\n";
        return;
    }

    width_  = j.value("width",  0);
    height_ = j.value("height", 0);
    if (width_ <= 0 || height_ <= 0) return;

    tiles_.assign(width_ * height_, "");

    // tile_defs: {"1": "floor_tile", "2": "wall_tile", ...}
    std::unordered_map<int, std::string> defs;
    if (j.contains("tile_defs")) {
        for (auto& [k, v] : j["tile_defs"].items())
            defs[std::stoi(k)] = v.get<std::string>();
    }

    // layers[0]["data"]: 2D array, row 0 = top of screen = world y = height-1
    if (!j.contains("layers") || j["layers"].empty()) return;
    auto& data = j["layers"][0]["data"];

    for (int row = 0; row < height_; ++row) {
        int world_y = height_ - 1 - row;   // flip: JSON top-row → highest Y
        if (row >= (int)data.size()) break;
        for (int col = 0; col < width_; ++col) {
            if (col >= (int)data[row].size()) break;
            int tid = data[row][col].get<int>();
            if (tid != 0) {
                auto it = defs.find(tid);
                if (it != defs.end())
                    tiles_[world_y * width_ + col] = it->second;
            }
        }
    }

    std::cout << "[TileMap] loaded " << path
              << " (" << width_ << "x" << height_ << ")\n";
}

// ── resize / clear ────────────────────────────────────────────────────────────

void TileMap::resize(int w, int h) {
    width_  = w;
    height_ = h;
    tiles_.assign(width_ * height_, "");
}

void TileMap::clear() {
    std::fill(tiles_.begin(), tiles_.end(), "");
}

// ── accessors ─────────────────────────────────────────────────────────────────

const std::string& TileMap::tile_at(int x, int y) const {
    if (!valid(x, y)) return EMPTY_;
    return tiles_[y * width_ + x];
}

void TileMap::set_tile(int x, int y, const std::string& sprite_id) {
    if (!valid(x, y)) return;
    tiles_[y * width_ + x] = sprite_id;
}
