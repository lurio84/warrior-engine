#pragma once

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include <entt/entt.hpp>
#include <string>

#include "audio.hpp"
#include "input.hpp"
#include "tilemap.hpp"
#include "camera.hpp"

// Lua VM — Phase 11
// Owns the sol2/LuaJIT state and exposes the engine.* API to scripts.

class LuaVM {
public:
    // Todos son owned por main; LuaVM guarda referencias no-owning.
    void init(entt::registry& reg, Audio& audio, InputSystem& input, TileMap& tilemap, Camera& cam);
    void shutdown();

    // Load and execute a Lua script file.  Returns false on error (non-fatal).
    bool exec_file(const std::string& path);

    // Llamar cada tick — invoca update(dt) si el script lo define.
    void tick(double dt);

    sol::state& state() { return lua_; }

private:
    sol::state      lua_;
    entt::registry* reg_     = nullptr;
    Audio*          audio_   = nullptr;
    InputSystem*    input_   = nullptr;
    TileMap*        tilemap_ = nullptr;
    Camera*         cam_     = nullptr;

    void bind_api();
};
