// The Warrior's Way Engine — Phase 3: Lua VM
// Entities are now created by scripts/main.lua, not hardcoded C++.

#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <entt/entt.hpp>

#include "components.hpp"
#include "renderer.hpp"
#include "lua_vm.hpp"
#include "file_watcher.hpp"
#include "atlas.hpp"
#include "audio.hpp"
#include "camera.hpp"
#include "debug_ui.hpp"
#include "network.hpp"
#include "input.hpp"
#include "tilemap.hpp"

#include <iostream>
#include <cstdlib>
#include <string>

// ── Fixed-timestep constants ──────────────────────────────────────────────────
static constexpr double TICK_HZ      = 50.0;
static constexpr double TICK_SECONDS = 1.0 / TICK_HZ;  // 20 ms

// ── Main ──────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    // ── Argumentos de red ────────────────────────────────────────────────────
    bool        net_host_mode   = false;
    bool        net_join_mode   = false;
    std::string net_join_ip;
    uint16_t    net_port        = 7777;

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

    // Abrir en display 1 si existe, si no en display 0
    // DP-4 está en x=0..3071, HDMI-0 en x=3072..6143 (según xrandr)
    // Posicionamos en DP-4 sin maximize para que el WM no la mueva
    int vp_w = 3072, vp_h = 1728;  // DP-4 resolución completa
    int win_x = 0, win_y = 0;      // esquina superior izquierda de DP-4

    SDL_Window* window = SDL_CreateWindow(
        "The Warrior's Way Engine | Phase 11: Tilemap",
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

    std::cout << "OpenGL " << glGetString(GL_VERSION)
              << "  |  " << glGetString(GL_RENDERER) << "\n"
              << "Controles: flechas=pan  +/-=zoom  0=reset  ESC=salir\n";

    SDL_GL_SetSwapInterval(1);
    glClearColor(0.05f, 0.05f, 0.08f, 1.f);

    // ── Debug UI ──────────────────────────────────────────────────────────────
    DebugUI debug_ui;
    debug_ui.init(window, gl_ctx);

    // ── ECS + renderer + Lua VM ───────────────────────────────────────────────
    entt::registry reg;

    Renderer renderer;
    renderer.init();

    // Assets desde el build, scripts siempre desde el fuente (evita copias rancias)
    char* base_path  = SDL_GetBasePath();
    std::string base_dir    = std::string(base_path);
    std::string scripts_dir = std::string(ENGINE_SOURCE_DIR) + "/scripts/";
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

    InputSystem input;
    TileMap tilemap;
    Camera cam;

    LuaVM lua_vm;
    lua_vm.init(reg, audio, input, tilemap, cam);
    lua_vm.exec_file(scripts_dir + "main.lua");  // populates the registry

    // ── Red ───────────────────────────────────────────────────────────────────
    NetworkManager net;
    net.init();
    if (net_host_mode)       net.host(net_port);
    else if (net_join_mode)  net.join(net_join_ip, net_port);

    // ── File Watcher (hot-reload) ─────────────────────────────────────────────
    std::string src_scripts_dir = std::string(ENGINE_SOURCE_DIR) + "/scripts/";
    FileWatcher fw;
    fw.watch(src_scripts_dir);

    // ── Fixed-timestep loop ───────────────────────────────────────────────────
    uint64_t prev_ticks = SDL_GetTicks64();
    double   accumulator = 0.0;
    double   total_time  = 0.0;
    float    fps         = 0.f;
    bool     running = true;
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
                        case SDLK_ESCAPE: running = false;       break;
                        case SDLK_F1:     debug_ui.toggle();     break;
                        case SDLK_LEFT:   cam.x  -= 1.f;        break;
                        case SDLK_RIGHT:  cam.x  += 1.f;        break;
                        case SDLK_UP:     cam.y  += 1.f;        break;
                        case SDLK_DOWN:   cam.y  -= 1.f;        break;
                        case SDLK_EQUALS:
                        case SDLK_KP_PLUS:  cam.zoom_in();      break;
                        case SDLK_MINUS:
                        case SDLK_KP_MINUS: cam.zoom_out();     break;
                        case SDLK_0:        cam.zoom_reset();   break;
                        // ── WASD: mueve el jugador local ──────────────────
                        case SDLK_w: case SDLK_a: case SDLK_s: case SDLK_d: {
                            entt::entity lp = net.local_player();
                            if (reg.valid(lp)) {
                                auto& tf = reg.get<components::Transform>(lp);
                                if (event.key.keysym.sym == SDLK_w) tf.y += 1.f;
                                if (event.key.keysym.sym == SDLK_s) tf.y -= 1.f;
                                if (event.key.keysym.sym == SDLK_a) tf.x -= 1.f;
                                if (event.key.keysym.sym == SDLK_d) tf.x += 1.f;
                                net.send_move((int16_t)tf.x, (int16_t)tf.y);
                            }
                            break;
                        }
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

        // ── Hot-reload ────────────────────────────────────────────────────────
        fw.poll([&](const std::string& path) {
            if (path.ends_with(".lua")) {
                std::cout << "[Hot-reload] " << path << "\n";
                reg.clear();
                tilemap.clear();
                lua_vm.exec_file(path);
                net.on_registry_cleared(reg);
            }
        });

        // ── Red ───────────────────────────────────────────────────────────────
        net.tick(reg);

        // ── Input snapshot (una vez por frame) ────────────────────────────────
        input.update();

        // ── Fixed-timestep logic at 50 Hz ─────────────────────────────────────
        uint64_t now   = SDL_GetTicks64();
        double   delta = (now - prev_ticks) * 0.001;
        prev_ticks     = now;
        accumulator   += delta;
        fps = (delta > 0.0) ? static_cast<float>(1.0 / delta) : 0.f;

        while (accumulator >= TICK_SECONDS) {
            lua_vm.tick(TICK_SECONDS);
            // ── Physics: aplicar velocidad a posición ─────────────────────────
            for (auto [e, tf, v] : reg.view<components::Transform,
                                            components::Velocity>().each()) {
                tf.x += v.vx * (float)TICK_SECONDS;
                tf.y += v.vy * (float)TICK_SECONDS;
            }
            total_time  += TICK_SECONDS;
            accumulator -= TICK_SECONDS;
        }

        // ── Render ────────────────────────────────────────────────────────────
        renderer.set_time(static_cast<float>(total_time));
        renderer.begin_frame(vp_w, vp_h, cam.x, cam.y, cam.zoom);
        renderer.draw_tilemap(tilemap);   // fondo primero
        renderer.draw_registry(reg);

        debug_ui.begin_frame();
        debug_ui.draw(reg, cam, audio, fps, total_time, net.stats());
        debug_ui.end_frame();

        SDL_GL_SwapWindow(window);
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    net.shutdown();
    debug_ui.shutdown();
    lua_vm.shutdown();
    audio.shutdown();
    atlas.shutdown();
    renderer.shutdown();
    SDL_GL_DeleteContext(gl_ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}
