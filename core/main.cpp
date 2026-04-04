// The Warrior's Way Engine — Phase 14

#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <entt/entt.hpp>

#include "components.hpp"
#include "renderer.hpp"
#include "file_watcher.hpp"
#include "atlas.hpp"
#include "audio.hpp"
#include "camera.hpp"
#include "debug_ui.hpp"
#include "network.hpp"
#include "input.hpp"
#include "tilemap.hpp"
#include "placement_grid.hpp"
#include "game_scene.hpp"
#include "save_load.hpp"

#include "drill_system.hpp"
#include "item_system.hpp"
#include "conveyor_system.hpp"
#include "machine_system.hpp"
#include "player_system.hpp"
#include "camera_system.hpp"
#include "placement_system.hpp"
#include "wave_system.hpp"
#include "enemy_system.hpp"
#include "combat_system.hpp"

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <string>

// ── Fixed-timestep constants ──────────────────────────────────────────────────
static constexpr double TICK_HZ      = 50.0;
static constexpr double TICK_SECONDS = 1.0 / TICK_HZ;  // 20 ms

// ── Main ──────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    // ── Argumentos de red ─────────────────────────────────────────────────────
    bool        net_host_mode = false;
    bool        net_join_mode = false;
    std::string net_join_ip;
    uint16_t    net_port      = 7777;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--host") {
            net_host_mode = true;
            if (i + 1 < argc && argv[i+1][0] != '-')
                net_port = (uint16_t)std::stoi(argv[++i]);
        } else if (arg == "--join" && i + 1 < argc) {
            net_join_mode = true;
            net_join_ip   = argv[++i];
            if (i + 1 < argc && argv[i+1][0] != '-')
                net_port = (uint16_t)std::stoi(argv[++i]);
        }
    }

    // ── SDL init ──────────────────────────────────────────────────────────────
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init: " << SDL_GetError() << "\n";
        return EXIT_FAILURE;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   0);

    int vp_w = 3072, vp_h = 1728;
    int win_x = 0,   win_y = 0;

    SDL_Window* window = SDL_CreateWindow(
        "The Warrior's Way Engine | Phase 13: Pure C++",
        win_x, win_y, vp_w, vp_h,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    if (!window) {
        std::cerr << "SDL_CreateWindow: " << SDL_GetError() << "\n";
        SDL_Quit(); return EXIT_FAILURE;
    }

    SDL_GLContext gl_ctx = SDL_GL_CreateContext(window);
    if (!gl_ctx) {
        std::cerr << "SDL_GL_CreateContext: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window); SDL_Quit(); return EXIT_FAILURE;
    }

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
        std::cerr << "GLAD: failed to load OpenGL\n";
        return EXIT_FAILURE;
    }

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    std::cout << "OpenGL " << glGetString(GL_VERSION)
              << "  |  " << glGetString(GL_RENDERER) << "\n"
              << "Controles: flechas=pan  +/-=zoom  0=reset  F1=debug  ESC=salir\n"
              << "           WASD=mover  Space=atacar  F5=guardar  F9=cargar\n";

    SDL_GL_SetSwapInterval(1);
    glClearColor(0.05f, 0.05f, 0.08f, 1.f);

    // ── Debug UI ──────────────────────────────────────────────────────────────
    DebugUI debug_ui;
    debug_ui.init(window, gl_ctx);

    // ── ECS + renderer ────────────────────────────────────────────────────────
    entt::registry reg;

    Renderer renderer;
    renderer.init();

    char* base_path = SDL_GetBasePath();
    std::string base_dir = std::string(base_path);
    SDL_free(base_path);

    // ── Atlas ─────────────────────────────────────────────────────────────────
    Atlas atlas;
    if (atlas.load(base_dir + "assets/atlas/atlas.json"))
        renderer.set_atlas(&atlas);

    // ── Audio ─────────────────────────────────────────────────────────────────
    Audio audio;
    audio.init();
    std::string sounds_dir = base_dir + "assets/sounds/";
    audio.load("click",       sounds_dir + "click.wav");
    audio.load("belt_loop",   sounds_dir + "belt_loop.wav");
    audio.load("item_pickup", sounds_dir + "item_pickup.wav");
    audio.set_volume(0.25f);   // volumen inicial moderado

    InputSystem input;
    TileMap tilemap;
    Camera cam;
    PlacementGrid grid;
    BuildState build;
    WaveState ws;

    // ── Scene init (replaces main.lua) ────────────────────────────────────────
    SceneState scene = init_scene(reg, tilemap, cam, grid, base_dir);

    // Refrescar sprites de cintas + conectar taladros
    {
        auto view = reg.view<components::ConveyorTag, components::Transform>();
        for (auto [e, ct, tf] : view.each())
            refresh_belt_area(reg, grid,
                              PlacementGrid::to_tile(tf.x),
                              PlacementGrid::to_tile(tf.y));
    }
    reconnect_drills(reg, grid);

    // ── Red ───────────────────────────────────────────────────────────────────
    NetworkManager net;
    net.init();
    if (net_host_mode)      net.host(net_port);
    else if (net_join_mode) net.join(net_join_ip, net_port);

    // ── File Watcher (hot-reload de shaders/atlas) ────────────────────────────
    std::string shaders_dir = std::string(ENGINE_SOURCE_DIR) + "/assets/shaders/";
    FileWatcher fw;
    fw.watch(shaders_dir);

    // ── Game state ────────────────────────────────────────────────────────────
    enum class GameState { Playing, GameOver, Victory };
    GameState game_state    = GameState::Playing;
    double    time_at_end   = 0.0;
    bool      restart_req   = false;

    // Helper: full scene reset (called on restart)
    auto reset_scene = [&]() {
        reg.clear();
        grid             = PlacementGrid{};
        build.selected   = 0;
        build.orientation= 0;
        build.preview    = entt::null;
        ws    = WaveState{};
        scene = init_scene(reg, tilemap, cam, grid, base_dir);
        {
            auto view = reg.view<components::ConveyorTag, components::Transform>();
            for (auto [e, ct, tf] : view.each())
                refresh_belt_area(reg, grid,
                                  PlacementGrid::to_tile(tf.x),
                                  PlacementGrid::to_tile(tf.y));
        }
        game_state  = GameState::Playing;
        restart_req = false;
    };

    // ── Fixed-timestep loop ───────────────────────────────────────────────────
    uint64_t prev_ticks = SDL_GetTicks64();
    double   accumulator = 0.0;
    double   total_time  = 0.0;
    float    fps         = 0.f;
    bool     running     = true;
    SDL_Event event;

    while (running) {
        // ── Events ────────────────────────────────────────────────────────────
        while (SDL_PollEvent(&event)) {
            debug_ui.process_event(event);
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE: running = false;         break;
                        case SDLK_r:
                            if (game_state != GameState::Playing)
                                restart_req = true;
                            break;
                        case SDLK_F1:     debug_ui.toggle();     break;
                        case SDLK_F5:
                            save_game(base_dir + "savegame.json",
                                      scene.inventory, ws, reg);
                            break;
                        case SDLK_F9:
                            load_game(base_dir + "savegame.json",
                                      scene.inventory, ws, reg);
                            break;
                        case SDLK_LEFT:   cam.x  -= 1.f;        break;
                        case SDLK_RIGHT:  cam.x  += 1.f;        break;
                        case SDLK_UP:     cam.y  += 1.f;        break;
                        case SDLK_DOWN:   cam.y  -= 1.f;        break;
                        case SDLK_EQUALS:
                        case SDLK_KP_PLUS:  cam.zoom_in();      break;
                        case SDLK_MINUS:
                        case SDLK_KP_MINUS: cam.zoom_out();     break;
                        case SDLK_0:        cam.zoom_reset();   break;
                    }
                    break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        vp_w = event.window.data1;
                        vp_h = event.window.data2;
                    }
                    break;
            }
        }

        // ── Hot-reload (shaders/atlas) ────────────────────────────────────────
        fw.poll([&](const std::string& path) {
            std::cout << "[Hot-reload] " << path << "\n";
        });

        // ── Red ───────────────────────────────────────────────────────────────
        net.tick(reg);

        // ── Input snapshot ────────────────────────────────────────────────────
        input.update();

        // ── Restart ───────────────────────────────────────────────────────────
        if (restart_req) { reset_scene(); }

        // ── Per-frame systems (input-driven, outside fixed timestep) ──────────
        if (game_state == GameState::Playing) {
            placement_system(reg, grid, cam, input, build, vp_w, vp_h);
            combat_system(reg, input, scene.inventory);
        }

        // ── Fixed-timestep logic at 50 Hz ─────────────────────────────────────
        uint64_t now   = SDL_GetTicks64();
        double   delta = (now - prev_ticks) * 0.001;
        prev_ticks     = now;
        accumulator   += delta;
        fps = (delta > 0.0) ? static_cast<float>(1.0 / delta) : 0.f;

        while (game_state == GameState::Playing && accumulator >= TICK_SECONDS) {
            float dt = static_cast<float>(TICK_SECONDS);

            // ── Gameplay systems (order matters) ──────────────────────────────
            player_system(reg, input);
            enemy_system(reg, dt);
            wave_system(reg, grid, ws, dt,
                        tilemap.width(), tilemap.height());
            conveyor_system(reg, static_cast<float>(total_time));
            drill_system(reg, dt);
            item_system(reg, dt, scene.inventory, scene.total_items_produced, grid, audio);
            machine_system(reg, dt, grid);

            // ── Physics: apply velocity → position ───────────────────────────
            for (auto [e, tf, v] : reg.view<components::Transform,
                                            components::Velocity>().each()) {
                // Player and enemies use axis-separated solid-tile collision.
                // Other moving entities (items) pass through freely.
                bool needs_collision = reg.all_of<components::PlayerTag>(e)
                                    || reg.all_of<components::EnemyTag>(e);
                if (!needs_collision) {
                    tf.x += v.vx * dt;
                    tf.y += v.vy * dt;
                } else {
                    // Colisión por eje (permite deslizamiento en esquinas)
                    constexpr float HALF = 0.42f;
                    auto solid = [&](float px, float py) {
                        int corners[4][2] = {
                            { PlacementGrid::to_tile(px - HALF), PlacementGrid::to_tile(py - HALF) },
                            { PlacementGrid::to_tile(px + HALF), PlacementGrid::to_tile(py - HALF) },
                            { PlacementGrid::to_tile(px - HALF), PlacementGrid::to_tile(py + HALF) },
                            { PlacementGrid::to_tile(px + HALF), PlacementGrid::to_tile(py + HALF) },
                        };
                        for (auto& c : corners) {
                            auto ent = grid.get(c[0], c[1]);
                            if (ent != entt::null && reg.all_of<components::SolidTag>(ent))
                                return true;
                        }
                        return false;
                    };
                    float nx = tf.x + v.vx * dt;
                    float ny = tf.y + v.vy * dt;
                    if (!solid(nx, tf.y)) tf.x = nx;
                    if (!solid(tf.x, ny)) tf.y = ny;
                }
            }

            // ── EffectTag: tick TTL y destruir expirados ──────────────────────
            {
                std::vector<entt::entity> dead_effects;
                for (auto [e, ef] : reg.view<components::EffectTag>().each()) {
                    ef.ttl -= dt;
                    if (ef.ttl <= 0.f) dead_effects.push_back(e);
                }
                for (auto e : dead_effects) reg.destroy(e);
            }

            // ── Camera sigue al jugador ───────────────────────────────────────
            camera_system(reg, cam, dt);

            // ── Condición de derrota ──────────────────────────────────────────
            for (auto [e, ph] : reg.view<components::PlayerHealth>().each()) {
                if (ph.hp <= 0.f) {
                    game_state  = GameState::GameOver;
                    time_at_end = total_time;
                }
                break;
            }

            // ── Condición de victoria: oleada 10 completada sin enemigos ─────
            if (ws.wave >= MAX_WAVES) {
                int enemy_count = 0;
                reg.view<components::EnemyTag>().each([&](auto){ ++enemy_count; });
                if (enemy_count == 0) {
                    game_state  = GameState::Victory;
                    time_at_end = total_time;
                }
            }

            total_time  += TICK_SECONDS;
            accumulator -= TICK_SECONDS;
        }

        // ── Render ────────────────────────────────────────────────────────────
        renderer.set_time(static_cast<float>(total_time));
        renderer.begin_frame(vp_w, vp_h, cam.x, cam.y, cam.zoom);
        renderer.draw_tilemap(tilemap);
        renderer.draw_registry(reg);

        // Collect player HP + equipment for HUD
        float hud_hp = 100.f, hud_max_hp = 100.f;
        const components::EquipmentTag* equip_ptr = nullptr;
        for (auto [e, ph] : reg.view<components::PlayerHealth>().each()) {
            hud_hp = ph.hp; hud_max_hp = ph.max_hp;
            if (reg.all_of<components::EquipmentTag>(e))
                equip_ptr = &reg.get<components::EquipmentTag>(e);
            break;
        }

        // ── Near-chest detection for HUD prompt ───────────────────────────────
        bool near_chest = false;
        {
            float player_x = 0.f, player_y = 0.f;
            for (auto [e, tf, pt] : reg.view<components::Transform,
                                            components::PlayerTag>().each()) {
                player_x = tf.x; player_y = tf.y; break;
            }
            for (auto [e, tf] : reg.view<components::Transform,
                                         components::BoxTag>().each()) {
                float dx = tf.x - player_x, dy = tf.y - player_y;
                if (dx*dx + dy*dy <= 1.5f * 1.5f) { near_chest = true; break; }
            }
        }

        debug_ui.begin_frame();
        debug_ui.draw(reg, cam, audio, fps, total_time, net.stats());
        debug_ui.draw_hud(scene.inventory, hud_hp, hud_max_hp,
                          ws.wave, ws.timer, equip_ptr, near_chest);

        if (game_state != GameState::Playing) {
            debug_ui.draw_end_screen(
                game_state == GameState::Victory
                    ? DebugUI::GameResult::Victory
                    : DebugUI::GameResult::GameOver,
                ws.wave,
                scene.total_items_produced,
                time_at_end,
                restart_req);
        }

        debug_ui.end_frame();

        SDL_GL_SwapWindow(window);
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    net.shutdown();
    debug_ui.shutdown();
    audio.shutdown();
    atlas.shutdown();
    renderer.shutdown();
    SDL_GL_DeleteContext(gl_ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}
