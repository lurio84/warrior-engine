#pragma once
#include <SDL2/SDL.h>
#include <entt/entt.hpp>
#include "camera.hpp"
#include "audio.hpp"
#include "network.hpp"

// Debug UI — Phase 8
// Panel ImGui activado con F1. Muestra entidades, cámara, audio y perf.

class DebugUI {
public:
    void init(SDL_Window* window, SDL_GLContext ctx);
    void shutdown();

    // Pasar cada evento SDL antes de procesar el input del juego
    void process_event(const SDL_Event& e);

    void begin_frame();
    void draw(entt::registry& reg, Camera& cam, Audio& audio,
              float fps, double total_time, const NetStats& net);
    void end_frame();

    bool visible = false;
    void toggle() { visible = !visible; }
};
