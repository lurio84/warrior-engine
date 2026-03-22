#pragma once
#include <string>
#include <vector>
#include <unordered_map>

// TileMap — Phase 11: Tilemap system
// Stores a 2D grid of sprite_id strings.
// Tiles are centered at integer world coordinates (same as entities).
// JSON format: { "width", "height", "tile_defs": {"1":"floor_tile",...},
//                "layers": [{"name":"ground","data":[[...],...]}] }
// data[0] = top row in JSON → highest Y in world (Y-up).

class TileMap {
public:
    void load(const std::string& path);
    void resize(int w, int h);   // creates empty map
    void clear();

    const std::string& tile_at(int x, int y) const;
    void set_tile(int x, int y, const std::string& sprite_id);

    int  width()  const { return width_;  }
    int  height() const { return height_; }
    bool valid(int x, int y) const { return x >= 0 && x < width_ && y >= 0 && y < height_; }
    bool empty()  const { return tiles_.empty(); }

private:
    int  width_  = 0;
    int  height_ = 0;
    std::vector<std::string> tiles_;   // row-major: tiles_[y * width_ + x]

    static const std::string EMPTY_;
};
