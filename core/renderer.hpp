#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <entt/entt.hpp>

#include "atlas.hpp"
#include "tilemap.hpp"

// 2D tile-space renderer — Phase 5
// Dibuja sprites del atlas con UV. Fallback a quad blanco × tint si no hay sprite_id.

class Renderer {
public:
    static constexpr int TILE_SIZE_PX = 32;  // debe coincidir con engine_config.json

    void init();
    void shutdown();

    // Asignar atlas antes del primer draw_registry
    void set_atlas(Atlas* atlas) { atlas_ = atlas; }

    // Llamar una vez por frame antes de draw_registry
    void begin_frame(int vp_w, int vp_h, float cam_x, float cam_y, float zoom);

    void set_time(float t) { time_ = t; }
    // Tilemap se dibuja ANTES de las entidades (fondo)
    void draw_tilemap(const TileMap& map);
    void draw_registry(entt::registry& reg);

private:
    GLuint prog_      = 0;
    GLuint vao_       = 0;
    GLuint vbo_       = 0;
    GLuint ibo_       = 0;
    GLuint white_tex_ = 0;  // textura 1×1 blanca para fallback

    GLint u_mvp_     = -1;
    GLint u_color_   = -1;
    GLint u_uv_rect_ = -1;
    GLint u_texture_ = -1;
    GLint u_scroll_  = -1;
    GLint u_repeat_  = -1;
    GLint u_time_u_  = -1;

    glm::mat4 view_proj_{1.f};
    Atlas*    atlas_ = nullptr;
    float     time_  = 0.f;
    float     zoom_  = 1.f;    // guardado desde begin_frame para pixel-snap
    int       vp_w_  = 0;
    int       vp_h_  = 0;

    static GLuint compile_shader(GLenum type, const char* src);
    static GLuint link_program(GLuint vert, GLuint frag);
};
