#pragma once
#include <SDL2/SDL.h>
#include <entt/entt.hpp>
#include "camera.hpp"
#include "audio.hpp"
#include "network.hpp"
#include "components.hpp"
#include <map>
#include <string>

// Debug UI — Phase 8
// Panel ImGui activado con F1. Muestra entidades, cámara, audio y perf.

class DebugUI {
public:
    enum class GameResult { None, GameOver, Victory };

    void init(SDL_Window* window, SDL_GLContext ctx);
    void shutdown();

    void process_event(const SDL_Event& e);

    void begin_frame();
    void draw(entt::registry& reg, Camera& cam, Audio& audio,
              float fps, double total_time, const NetStats& net);
    void draw_hud(const std::map<std::string, int>& inventory,
                  float hp, float max_hp, int wave, float wave_timer,
                  const components::EquipmentTag* equip = nullptr,
                  bool near_chest = false);
    void draw_end_screen(GameResult result, int waves_survived,
                         int items_produced, double time_played,
                         bool& restart_requested);
    void end_frame();

    bool visible = false;
    void toggle() { visible = !visible; }
};
