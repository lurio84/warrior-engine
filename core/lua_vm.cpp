#include "lua_vm.hpp"
#include "components.hpp"

#include <iostream>

// ── init / shutdown ───────────────────────────────────────────────────────────

void LuaVM::init(entt::registry& reg, Audio& audio, InputSystem& input, TileMap& tilemap, Camera& cam) {
    reg_     = &reg;
    audio_   = &audio;
    input_   = &input;
    tilemap_ = &tilemap;
    cam_     = &cam;

    lua_.open_libraries(
        sol::lib::base,
        sol::lib::math,
        sol::lib::table,
        sol::lib::string,
        sol::lib::io
    );

    bind_api();
}

void LuaVM::shutdown() {
    // sol::state destructor cleans up the Lua VM
}

// ── exec_file ─────────────────────────────────────────────────────────────────

bool LuaVM::exec_file(const std::string& path) {
    auto result = lua_.safe_script_file(path, sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error err = result;
        std::cerr << "[Lua] Error in '" << path << "':\n  " << err.what() << "\n";
        return false;
    }
    return true;
}

// ── tick ──────────────────────────────────────────────────────────────────────

void LuaVM::tick(double dt) {
    sol::protected_function upd = lua_["update"];
    if (upd.valid()) {
        auto res = upd(dt);
        if (!res.valid()) {
            sol::error err = res;
            std::cerr << "[Lua] update(): " << err.what() << "\n";
        }
    }
}

// ── bind_api ──────────────────────────────────────────────────────────────────
// Exposed to Lua:
//
//   engine.TILE_SIZE              → 32 (read-only convention)
//   engine.log(msg)               → prints "[Lua] msg"
//
//   engine.entity.create()        → uint  (entity ID)
//   engine.entity.destroy(id)
//   engine.entity.set_pos(id, x, y)
//   engine.entity.get_pos(id)     → x, y
//   engine.entity.set_rotation(id, radians)
//   engine.entity.set_color(id, r, g, b, a)
//   engine.entity.set_size(id, w, h)
//   engine.entity.set_sprite(id, sprite_id)

void LuaVM::bind_api() {
    // ── engine table ──────────────────────────────────────────────────────────
    sol::table eng = lua_.create_named_table("engine");
    eng["TILE_SIZE"] = 32;

    eng["log"] = [](const std::string& msg) {
        std::cout << "[Lua] " << msg << "\n";
    };

    // ── engine.entity table ───────────────────────────────────────────────────
    sol::table entity = lua_.create_table();
    eng["entity"] = entity;

    entity["create"] = [this]() -> uint32_t {
        auto e = reg_->create();
        reg_->emplace<components::Transform>(e);
        reg_->emplace<components::SpriteRef>(e);
        return static_cast<uint32_t>(entt::to_integral(e));
    };

    entity["destroy"] = [this](uint32_t id) {
        auto e = static_cast<entt::entity>(id);
        if (reg_->valid(e)) reg_->destroy(e);
    };

    entity["set_pos"] = [this](uint32_t id, float x, float y) {
        auto e = static_cast<entt::entity>(id);
        if (auto* tf = reg_->try_get<components::Transform>(e)) {
            tf->x = x;
            tf->y = y;
        }
    };

    entity["get_pos"] = [this](uint32_t id) -> std::tuple<float, float> {
        auto e = static_cast<entt::entity>(id);
        if (auto* tf = reg_->try_get<components::Transform>(e))
            return { tf->x, tf->y };
        return { 0.f, 0.f };
    };

    entity["set_rotation"] = [this](uint32_t id, float radians) {
        auto e = static_cast<entt::entity>(id);
        if (auto* tf = reg_->try_get<components::Transform>(e))
            tf->rotation = radians;
    };

    entity["set_color"] = [this](uint32_t id, float r, float g, float b, float a) {
        auto e = static_cast<entt::entity>(id);
        if (auto* sp = reg_->try_get<components::SpriteRef>(e))
            sp->tint = { r, g, b, a };
    };

    entity["set_size"] = [this](uint32_t id, int w, int h) {
        auto e = static_cast<entt::entity>(id);
        if (auto* sp = reg_->try_get<components::SpriteRef>(e)) {
            sp->size_w = w;
            sp->size_h = h;
        }
    };

    entity["set_scale"] = [this](uint32_t id, float sx, float sy) {
        auto e = static_cast<entt::entity>(id);
        if (auto* tf = reg_->try_get<components::Transform>(e)) {
            tf->scale_x = sx;
            tf->scale_y = sy;
        }
    };

    entity["set_layer"] = [this](uint32_t id, int layer) {
        auto e = static_cast<entt::entity>(id);
        if (auto* sp = reg_->try_get<components::SpriteRef>(e))
            sp->layer = layer;
    };

    entity["set_scroll"] = [this](uint32_t id, float sx, float sy) {
        auto e = static_cast<entt::entity>(id);
        if (auto* sp = reg_->try_get<components::SpriteRef>(e)) {
            sp->scroll_x = sx;
            sp->scroll_y = sy;
        }
    };

    entity["set_sprite"] = [this](uint32_t id, const std::string& sprite_id) {
        auto e = static_cast<entt::entity>(id);
        if (auto* sp = reg_->try_get<components::SpriteRef>(e))
            sp->sprite_id = sprite_id;
    };

    // ── engine.entity — velocity ──────────────────────────────────────────────
    entity["set_velocity"] = [this](uint32_t id, float vx, float vy) {
        auto e = static_cast<entt::entity>(id);
        if (!reg_->valid(e)) return;
        auto& v = reg_->get_or_emplace<components::Velocity>(e);
        v.vx = vx; v.vy = vy;
    };

    entity["get_velocity"] = [this](uint32_t id) -> std::tuple<float, float> {
        auto e = static_cast<entt::entity>(id);
        if (auto* v = reg_->try_get<components::Velocity>(e))
            return { v->vx, v->vy };
        return { 0.f, 0.f };
    };

    entity["stop"] = [this](uint32_t id) {
        auto e = static_cast<entt::entity>(id);
        if (auto* v = reg_->try_get<components::Velocity>(e))
            v->vx = v->vy = 0.f;
    };

    // ── engine.input table ────────────────────────────────────────────────────
    sol::table input_tbl = lua_.create_table();
    eng["input"] = input_tbl;

    input_tbl["down"] = [this](const std::string& name) -> bool {
        return input_->down(name);
    };
    input_tbl["pressed"] = [this](const std::string& name) -> bool {
        return input_->pressed(name);
    };

    // ── engine.audio table ────────────────────────────────────────────────────
    sol::table audio_tbl = lua_.create_table();
    eng["audio"] = audio_tbl;

    audio_tbl["play"] = [this](const std::string& id) {
        audio_->play(id);
    };
    audio_tbl["stop"] = [this](const std::string& id) {
        audio_->stop(id);
    };
    audio_tbl["volume"] = [this](float vol) {
        audio_->set_volume(vol);
    };

    // ── engine.tilemap table ──────────────────────────────────────────────────
    //   engine.tilemap.load(path)          → carga mapa desde JSON
    //   engine.tilemap.new(w, h)           → crea mapa vacío w×h
    //   engine.tilemap.set(x, y, sprite)   → coloca tile (sprite="" para borrar)
    //   engine.tilemap.get(x, y)           → retorna sprite_id o ""
    //   engine.tilemap.clear()             → vacía todas las celdas
    //   engine.tilemap.width()             → ancho en tiles
    //   engine.tilemap.height()            → alto en tiles
    sol::table tm_tbl = lua_.create_table();
    eng["tilemap"] = tm_tbl;

    tm_tbl["load"] = [this](const std::string& path) {
        tilemap_->load(path);
    };
    tm_tbl["new"] = [this](int w, int h) {
        tilemap_->resize(w, h);
    };
    tm_tbl["set"] = [this](int x, int y, const std::string& sprite_id) {
        tilemap_->set_tile(x, y, sprite_id);
    };
    tm_tbl["get"] = [this](int x, int y) -> std::string {
        return tilemap_->tile_at(x, y);
    };
    tm_tbl["clear"] = [this]() {
        tilemap_->clear();
    };
    tm_tbl["width"] = [this]() -> int {
        return tilemap_->width();
    };
    tm_tbl["height"] = [this]() -> int {
        return tilemap_->height();
    };

    // ── engine.camera table ───────────────────────────────────────────────────
    //   engine.camera.set(x, y)   → mueve la cámara
    //   engine.camera.get()       → retorna x, y
    //   engine.camera.zoom(z)     → establece zoom (0.5 | 1.0 | 2.0)
    sol::table cam_tbl = lua_.create_table();
    eng["camera"] = cam_tbl;

    cam_tbl["set"] = [this](float x, float y) {
        cam_->x = x;
        cam_->y = y;
    };
    cam_tbl["get"] = [this]() -> std::tuple<float, float> {
        return { cam_->x, cam_->y };
    };
    cam_tbl["zoom"] = [this](float z) {
        cam_->zoom = z;
    };
}
