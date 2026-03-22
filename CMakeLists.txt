cmake_minimum_required(VERSION 3.20)
project(warrior_engine VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# ── SDL2 ──────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.30.9
    GIT_SHALLOW    TRUE
)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL2)

# ── EnTT ──────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        v3.13.2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(entt)

# ── GLM ───────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)
set(GLM_ENABLE_CXX_20 ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glm)

# ── stb ───────────────────────────────────────────────────────────────────────
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(stb)

# stb is header-only — wrap it in an INTERFACE target
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${stb_SOURCE_DIR})

# ── GLAD (OpenGL 4.5 core) ────────────────────────────────────────────────────
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v0.1.36
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(glad)
if(NOT glad_POPULATED)
    FetchContent_Populate(glad)
    set(GLAD_PROFILE  "core" CACHE STRING "OpenGL profile" FORCE)
    set(GLAD_API      "gl=4.5" CACHE STRING "OpenGL version" FORCE)
    set(GLAD_GENERATOR "c"    CACHE STRING "Language to generate" FORCE)
    set(GLAD_EXTENSIONS ""    CACHE STRING "Extensions to enable" FORCE)
    add_subdirectory(${glad_SOURCE_DIR} ${glad_BINARY_DIR})
endif()

# ── nlohmann/json (header-only, para atlas loader) ────────────────────────────
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# ── efsw (file system watcher) ────────────────────────────────────────────────
FetchContent_Declare(
    efsw
    GIT_REPOSITORY https://github.com/SpartanJ/efsw.git
    GIT_TAG        1.4.0
    GIT_SHALLOW    TRUE
)
set(EFSW_BUILD_TEST_APP OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(efsw)

# ── LuaJIT (system package: sudo apt install libluajit-5.1-dev) ───────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(LUAJIT REQUIRED luajit)

# ── sol2 (header-only C++ → Lua binding) ─────────────────────────────────────
FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG        v3.3.0
    GIT_SHALLOW    TRUE
)
set(SOL2_BUILD_LUA    FALSE CACHE BOOL "" FORCE)
set(SOL2_ENABLE_INSTALL FALSE CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(sol2)

# ── miniaudio (header-only) ───────────────────────────────────────────────────
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG        0.11.21
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(miniaudio)
add_library(miniaudio_iface INTERFACE)
target_include_directories(miniaudio_iface INTERFACE ${miniaudio_SOURCE_DIR})

# ── ENet (reliable UDP) ───────────────────────────────────────────────────────
FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG        v1.3.17
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(enet)
add_library(enet_lib STATIC
    ${enet_SOURCE_DIR}/callbacks.c
    ${enet_SOURCE_DIR}/compress.c
    ${enet_SOURCE_DIR}/host.c
    ${enet_SOURCE_DIR}/list.c
    ${enet_SOURCE_DIR}/packet.c
    ${enet_SOURCE_DIR}/peer.c
    ${enet_SOURCE_DIR}/protocol.c
    ${enet_SOURCE_DIR}/unix.c
)
target_include_directories(enet_lib PUBLIC ${enet_SOURCE_DIR}/include)
target_compile_definitions(enet_lib PRIVATE HAS_SOCKLEN_T)

# ── Dear ImGui ────────────────────────────────────────────────────────────────
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC SDL2::SDL2-static)

# ── Engine executable ─────────────────────────────────────────────────────────
add_executable(warrior_engine
    core/main.cpp
    core/renderer.cpp
    core/lua_vm.cpp
    core/file_watcher.cpp
    core/atlas.cpp
    core/stb_impl.cpp
    core/audio.cpp
    core/audio_impl.cpp
    core/debug_ui.cpp
    core/network.cpp
    core/tilemap.cpp
)

target_include_directories(warrior_engine PRIVATE
    core
    ${LUAJIT_INCLUDE_DIRS}
)

target_link_libraries(warrior_engine PRIVATE
    SDL2::SDL2-static
    EnTT::EnTT
    glm::glm
    stb_image
    glad
    sol2
    efsw
    nlohmann_json::nlohmann_json
    miniaudio_iface
    imgui_lib
    enet_lib
    ${LUAJIT_LIBRARIES}
)

# Embed source dir so the FileWatcher can watch the real scripts/ folder
target_compile_definitions(warrior_engine PRIVATE
    ENGINE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

# Link platform libs needed by SDL2 static on Linux
if(UNIX AND NOT APPLE)
    target_link_libraries(warrior_engine PRIVATE dl pthread m)
endif()

# ── Post-build: copy assets next to the binary ────────────────────────────────
add_custom_command(TARGET warrior_engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:warrior_engine>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/scripts $<TARGET_FILE_DIR:warrior_engine>/scripts
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/game $<TARGET_FILE_DIR:warrior_engine>/game
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/engine_config.json $<TARGET_FILE_DIR:warrior_engine>/engine_config.json
    COMMENT "Copying runtime data to build directory"
)
