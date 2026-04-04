#include "debug_ui.hpp"
#include "components.hpp"

#include <imgui.h>
#include <map>
#include <string>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>

// ── init / shutdown ───────────────────────────────────────────────────────────

void DebugUI::init(SDL_Window* window, SDL_GLContext ctx) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // Escalar UI — tamaño base legible en cualquier monitor
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;   // no persistir posiciones entre sesiones
    io.FontGlobalScale = 2.0f;
    ImGui::GetStyle().ScaleAllSizes(2.0f);
    ImGui_ImplSDL2_InitForOpenGL(window, ctx);
    ImGui_ImplOpenGL3_Init("#version 450");
}

void DebugUI::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

// ── event passthrough ─────────────────────────────────────────────────────────

void DebugUI::process_event(const SDL_Event& e) {
    ImGui_ImplSDL2_ProcessEvent(&e);
}

// ── frame lifecycle ───────────────────────────────────────────────────────────

void DebugUI::begin_frame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void DebugUI::end_frame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// ── draw ──────────────────────────────────────────────────────────────────────

void DebugUI::draw(entt::registry& reg, Camera& cam,
                   Audio& audio, float fps, double total_time,
                   const NetStats& net) {
    using namespace components;

    if (!visible) return;

    // Panel único en la esquina superior izquierda
    ImGui::SetNextWindowPos({10, 10}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({500, 900}, ImGuiCond_Always);
    ImGui::Begin("Debug", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse);

    // ── Performance ───────────────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("FPS       : %.1f", fps);
        ImGui::Text("Tiempo    : %.1f s", total_time);
        ImGui::Text("Entidades : %d", (int)(reg.storage<entt::entity>().size()
                                           - reg.storage<entt::entity>().free_list()));
    }

    // ── Cámara ────────────────────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Camara", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat("X",    &cam.x,    0.f, 60.f);
        ImGui::SliderFloat("Y",    &cam.y,    0.f, 34.f);
        ImGui::SliderFloat("Zoom", &cam.zoom, 0.25f, 4.f);
        if (ImGui::Button("Reset zoom")) { cam.zoom_reset(); }
    }

    // ── Audio ─────────────────────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Audio")) {
        static float master = 1.f;
        if (ImGui::SliderFloat("Volumen", &master, 0.f, 1.f))
            audio.set_volume(master);
        if (ImGui::Button("click"))       audio.play("click");
        ImGui::SameLine();
        if (ImGui::Button("belt_loop"))   audio.play("belt_loop");
        ImGui::SameLine();
        if (ImGui::Button("item_pickup")) audio.play("item_pickup");
    }

    // ── Red ───────────────────────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Red")) {
        const char* mode_str =
            net.mode == NetStats::Mode::Host   ? "HOST" :
            net.mode == NetStats::Mode::Client ? "CLIENT" : "OFFLINE";
        ImGui::Text("Modo      : %s", mode_str);
        ImGui::Text("Peers     : %d", net.peer_count);
        ImGui::Text("Conectado : %s", net.connected ? "SI" : "NO");
        if (net.latency_ms > 0)
            ImGui::Text("Latencia  : %u ms", net.latency_ms);
        else
            ImGui::Text("Latencia  : --");
    }

    // ── Entidades ─────────────────────────────────────────────────────────────
    if (ImGui::CollapsingHeader("Entidades", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginTable("ents", 6,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
            {0, 200});
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("x");
        ImGui::TableSetupColumn("y");
        ImGui::TableSetupColumn("sprite");
        ImGui::TableSetupColumn("size");
        ImGui::TableSetupColumn("scroll");
        ImGui::TableHeadersRow();

        for (auto [e, tf, sp] : reg.view<Transform, SpriteRef>().each()) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%u", (uint32_t)entt::to_integral(e));
            ImGui::TableNextColumn(); ImGui::Text("%.1f", tf.x);
            ImGui::TableNextColumn(); ImGui::Text("%.1f", tf.y);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(sp.sprite_id.empty() ? "-" : sp.sprite_id.c_str());
            ImGui::TableNextColumn(); ImGui::Text("%dx%d", sp.size_w, sp.size_h);
            ImGui::TableNextColumn(); ImGui::Text("%.2f,%.2f", sp.scroll_x, sp.scroll_y);
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

void DebugUI::draw_hud(const std::map<std::string, int>& inventory,
                       float hp, float max_hp, int wave, float wave_timer,
                       const components::EquipmentTag* equip) {
    ImGuiIO& io = ImGui::GetIO();

    // ── Countdown centrado (solo cuando queden <30 s para la siguiente oleada) ─
    if (wave_timer < 30.f) {
        ImGui::SetNextWindowPos({io.DisplaySize.x * 0.5f, 10.f},
                                 ImGuiCond_Always, {0.5f, 0.f});
        ImGui::SetNextWindowBgAlpha(0.75f);
        ImGui::Begin("##countdown", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
            ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoMove);
        ImGui::TextColored({1.f, 0.3f, 0.1f, 1.f},
                           "Oleada %d en %.0f s", wave + 1, wave_timer);
        ImGui::End();
    }

    // ── Panel principal (esquina superior derecha) ────────────────────────────
    ImGui::SetNextWindowPos({io.DisplaySize.x - 10.f, 10.f},
                             ImGuiCond_Always, {1.f, 0.f});
    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::Begin("##hud", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoMove);

    // ── Vida ──────────────────────────────────────────────────────────────────
    ImGui::TextColored({0.9f, 0.3f, 0.3f, 1.f}, "HP");
    ImGui::SameLine();
    float frac = (max_hp > 0.f) ? (hp / max_hp) : 0.f;
    ImVec4 bar_col = (frac > 0.5f)  ? ImVec4{0.2f,0.8f,0.2f,1.f}
                   : (frac > 0.25f) ? ImVec4{0.9f,0.7f,0.1f,1.f}
                   :                  ImVec4{0.9f,0.2f,0.2f,1.f};
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, bar_col);
    char hp_label[32];
    std::snprintf(hp_label, sizeof(hp_label), "%.0f/%.0f", hp, max_hp);
    ImGui::ProgressBar(frac, {160.f, 0.f}, hp_label);
    ImGui::PopStyleColor();

    // ── Oleada ────────────────────────────────────────────────────────────────
    if (wave > 0)
        ImGui::TextColored({1.f, 0.5f, 0.2f, 1.f}, "Oleada %d / %d", wave, 10);
    else
        ImGui::TextDisabled("Primera oleada en %.0f s", wave_timer);

    // ── Equipo ────────────────────────────────────────────────────────────────
    ImGui::Separator();
    ImGui::TextColored({0.7f, 0.85f, 1.f, 1.f}, "Equipo");
    if (equip) {
        ImGui::Text("Arma    : %s", equip->weapon_id.empty()  ? "-" : equip->weapon_id.c_str());
        ImGui::Text("Casco   : %s", equip->helmet_id.empty()  ? "-" : equip->helmet_id.c_str());
        ImGui::Text("Pechera : %s", equip->chest_id.empty()   ? "-" : equip->chest_id.c_str());
        ImGui::TextDisabled("[E] equipar desde cofre");
    }

    // ── Recursos ─────────────────────────────────────────────────────────────
    ImGui::Separator();
    ImGui::TextColored({1.f, 0.85f, 0.1f, 1.f}, "Recursos");
    if (inventory.empty()) {
        ImGui::TextDisabled("(ninguno)");
    } else {
        for (const auto& [type, count] : inventory)
            ImGui::Text("%-16s %d", type.c_str(), count);
    }
    ImGui::End();
}

void DebugUI::draw_end_screen(GameResult result, int waves_survived,
                               int items_produced, double time_played,
                               bool& restart_requested) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f},
                             ImGuiCond_Always, {0.5f, 0.5f});
    ImGui::SetNextWindowSize({500.f, 0.f}, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.92f);
    ImGui::Begin("##endscreen", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    if (result == GameResult::Victory) {
        ImGui::TextColored({1.f, 0.9f, 0.1f, 1.f}, "*** VICTORIA ***");
        ImGui::TextColored({0.8f, 0.8f, 0.8f, 1.f},
                           "Sobreviviste las 10 oleadas.");
    } else {
        ImGui::TextColored({0.9f, 0.2f, 0.2f, 1.f}, "*** GAME OVER ***");
        ImGui::TextColored({0.8f, 0.8f, 0.8f, 1.f},
                           "El jugador ha caido en combate.");
    }

    ImGui::Separator();
    int minutes = static_cast<int>(time_played) / 60;
    int seconds = static_cast<int>(time_played) % 60;
    ImGui::Text("Oleadas sobrevividas : %d / 10", waves_survived);
    ImGui::Text("Items producidos     : %d",   items_produced);
    ImGui::Text("Tiempo jugado        : %02d:%02d", minutes, seconds);
    ImGui::Separator();

    ImGui::TextColored({0.6f,0.9f,0.6f,1.f}, "[R] Reiniciar");
    ImGui::SameLine();
    ImGui::TextColored({0.7f,0.7f,0.7f,1.f}, "  [ESC] Salir");

    if (ImGui::Button("Reiniciar", {200.f, 0.f}))
        restart_requested = true;

    ImGui::End();
}
