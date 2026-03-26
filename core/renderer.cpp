#include "renderer.hpp"
#include "components.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

// ── Shaders ───────────────────────────────────────────────────────────────────

static constexpr const char* VERT_SRC = R"glsl(
#version 450 core
layout(location = 0) in vec2 a_pos;   // unit quad [-0.5, 0.5]
layout(location = 1) in vec2 a_uv;    // local UV  [0, 1]

uniform mat4 u_mvp;

out vec2 v_local_uv;   // interpolado correctamente antes de scroll/fract

void main() {
    gl_Position  = u_mvp * vec4(a_pos, 0.0, 1.0);
    v_local_uv   = a_uv;
}
)glsl";

static constexpr const char* FRAG_SRC = R"glsl(
#version 450 core
uniform sampler2D u_texture;
uniform vec4      u_color;
uniform vec4      u_uv_rect;   // (u0, v0, u1, v1) en atlas
uniform vec2      u_scroll;    // UV cycles/sec (x, y)
uniform vec2      u_repeat;    // tile repeat (size_w, size_h) para sprites multi-tile
uniform float     u_time;

in  vec2 v_local_uv;
out vec4 frag_color;

void main() {
    vec2 uv_raw   = v_local_uv * u_repeat - u_scroll * u_time;
    // fract solo cuando hay scroll activo; evita discontinuidad en bordes de tiles
    bool scrolling = dot(u_scroll, u_scroll) > 0.0;
    vec2 scrolled  = scrolling ? fract(uv_raw) : clamp(uv_raw, 0.0, 1.0);
    vec2 atlas_uv  = mix(u_uv_rect.xy, u_uv_rect.zw, scrolled);
    frag_color     = texture(u_texture, atlas_uv) * u_color;
}
)glsl";

// ── Quad geometry (pos + uv, immutable DSA) ───────────────────────────────────
//
//  v0(-0.5, 0.5) uv(0,1) ─── v1(0.5, 0.5) uv(1,1)
//       │              ╲              │
//  v2(-0.5,-0.5) uv(0,0) ─── v3(0.5,-0.5) uv(1,0)

static constexpr float QUAD_VERTS[] = {
    // pos          uv
    -0.5f,  0.5f,   0.f, 1.f,   // v0 top-left
     0.5f,  0.5f,   1.f, 1.f,   // v1 top-right
    -0.5f, -0.5f,   0.f, 0.f,   // v2 bot-left
     0.5f, -0.5f,   1.f, 0.f,   // v3 bot-right
};
static constexpr unsigned short QUAD_IDX[] = { 0, 2, 1,  1, 2, 3 };

// ── init / shutdown ───────────────────────────────────────────────────────────

void Renderer::init() {
    GLuint vert = compile_shader(GL_VERTEX_SHADER,   VERT_SRC);
    GLuint frag = compile_shader(GL_FRAGMENT_SHADER, FRAG_SRC);
    prog_ = link_program(vert, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    u_mvp_     = glGetUniformLocation(prog_, "u_mvp");
    u_color_   = glGetUniformLocation(prog_, "u_color");
    u_uv_rect_ = glGetUniformLocation(prog_, "u_uv_rect");
    u_texture_ = glGetUniformLocation(prog_, "u_texture");
    u_scroll_  = glGetUniformLocation(prog_, "u_scroll");
    u_repeat_  = glGetUniformLocation(prog_, "u_repeat");
    u_time_u_  = glGetUniformLocation(prog_, "u_time");

    // DSA buffers + VAO
    glCreateVertexArrays(1, &vao_);
    glCreateBuffers(1, &vbo_);
    glCreateBuffers(1, &ibo_);

    glNamedBufferStorage(vbo_, sizeof(QUAD_VERTS), QUAD_VERTS, 0);
    glNamedBufferStorage(ibo_, sizeof(QUAD_IDX),   QUAD_IDX,   0);

    // stride = 4 floats (pos xy + uv xy)
    glVertexArrayVertexBuffer(vao_,  0, vbo_, 0, 4 * sizeof(float));
    glVertexArrayElementBuffer(vao_, ibo_);

    // attrib 0 — posición (2 floats, offset 0)
    glEnableVertexArrayAttrib(vao_, 0);
    glVertexArrayAttribFormat(vao_,  0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao_, 0, 0);

    // attrib 1 — UV (2 floats, offset 8 bytes)
    glEnableVertexArrayAttrib(vao_, 1);
    glVertexArrayAttribFormat(vao_,  1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
    glVertexArrayAttribBinding(vao_, 1, 0);

    // Textura blanca 1×1 para entidades sin sprite_id
    static const uint8_t white[4] = {255, 255, 255, 255};
    glCreateTextures(GL_TEXTURE_2D, 1, &white_tex_);
    glTextureStorage2D(white_tex_, 1, GL_RGBA8, 1, 1);
    glTextureSubImage2D(white_tex_, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTextureParameteri(white_tex_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(white_tex_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void Renderer::shutdown() {
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
    glDeleteBuffers(1, &ibo_);
    glDeleteTextures(1, &white_tex_);
    glDeleteProgram(prog_);
}

// ── begin_frame ───────────────────────────────────────────────────────────────

void Renderer::begin_frame(int vp_w, int vp_h,
                            float cam_x, float cam_y,
                            float zoom) {
    glViewport(0, 0, vp_w, vp_h);
    glClear(GL_COLOR_BUFFER_BIT);

    zoom_ = zoom;
    vp_w_ = vp_w;
    vp_h_ = vp_h;

    float half_w = (vp_w * 0.5f) / (TILE_SIZE_PX * zoom);
    float half_h = (vp_h * 0.5f) / (TILE_SIZE_PX * zoom);

    view_proj_ = glm::ortho(
        cam_x - half_w,  cam_x + half_w,
        cam_y - half_h,  cam_y + half_h,
        -1.f, 1.f
    );
}

// ── draw_tilemap ──────────────────────────────────────────────────────────────
// Dibuja todas las celdas no vacías del mapa antes de las entidades.
// Tile en (x,y) → quad centrado en (x, y) igual que una entidad 1×1.

void Renderer::draw_tilemap(const TileMap& map) {
    if (map.empty()) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(prog_);
    glBindVertexArray(vao_);
    glUniform1i(u_texture_, 0);
    glUniform1f(u_time_u_, 0.f);
    glUniform2f(u_scroll_, 0.f, 0.f);
    glUniform2f(u_repeat_, 1.f, 1.f);

    static const glm::vec4 WHITE{1, 1, 1, 1};
    glUniform4fv(u_color_, 1, glm::value_ptr(WHITE));

    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const auto& sid = map.tile_at(x, y);
            if (sid.empty()) continue;

            GLuint tex = white_tex_;
            SpriteUV uv{0.f, 0.f, 1.f, 1.f};
            if (atlas_) {
                SpriteUV found;
                if (atlas_->get_uv(sid, found)) {
                    tex = atlas_->texture();
                    uv  = found;
                }
            }

            glBindTextureUnit(0, tex);
            glUniform4f(u_uv_rect_, uv.u0, uv.v0, uv.u1, uv.v1);

            glm::mat4 model{1.f};
            model = glm::translate(model, glm::vec3((float)x, (float)y, 0.f));
            // escala 1×1 → el quad [-0.5,0.5] cubre [x-0.5, x+0.5]
            glm::mat4 mvp = view_proj_ * model;
            glUniformMatrix4fv(u_mvp_, 1, GL_FALSE, glm::value_ptr(mvp));
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
        }
    }
}

// ── draw_registry ─────────────────────────────────────────────────────────────

void Renderer::draw_registry(entt::registry& reg) {
    using namespace components;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(prog_);
    glBindVertexArray(vao_);
    glUniform1i(u_texture_, 0);   // texture unit 0
    glUniform1f(u_time_u_, time_);

    // Ordenar entidades por layer para render correcto
    std::vector<entt::entity> order;
    order.reserve(reg.view<Transform, SpriteRef>().size_hint());
    for (auto e : reg.view<Transform, SpriteRef>())
        order.push_back(e);
    std::stable_sort(order.begin(), order.end(), [&](entt::entity a, entt::entity b) {
        return reg.get<SpriteRef>(a).layer < reg.get<SpriteRef>(b).layer;
    });

    for (auto entity : order) {
        auto& tf = reg.get<Transform>(entity);
        auto& sp = reg.get<SpriteRef>(entity);

        // ── Seleccionar textura y UV rect ─────────────────────────────────────
        GLuint tex = white_tex_;
        SpriteUV uv{0.f, 0.f, 1.f, 1.f};

        if (atlas_ && !sp.sprite_id.empty()) {
            SpriteUV found;
            if (atlas_->get_uv(sp.sprite_id, found)) {
                tex = atlas_->texture();
                uv  = found;
            }
        }

        glBindTextureUnit(0, tex);
        glUniform4f(u_uv_rect_, uv.u0, uv.v0, uv.u1, uv.v1);
        glUniform2f(u_scroll_, sp.scroll_x, sp.scroll_y);
        glUniform2f(u_repeat_, (float)sp.size_w, (float)sp.size_h);

        // ── Model matrix ──────────────────────────────────────────────────────
        // Snap posición al grid de píxeles para evitar jitter sub-pixel
        float px_scale = TILE_SIZE_PX * zoom_;
        float snap_x   = std::round(tf.x * px_scale) / px_scale;
        float snap_y   = std::round(tf.y * px_scale) / px_scale;

        glm::mat4 model{1.f};
        model = glm::translate(model, glm::vec3(snap_x, snap_y, 0.f));
        model = glm::rotate(model, tf.rotation, glm::vec3(0.f, 0.f, 1.f));
        model = glm::scale(model, glm::vec3(
            sp.size_w * tf.scale_x,
            sp.size_h * tf.scale_y,
            1.f
        ));

        glm::mat4 mvp = view_proj_ * model;
        glUniformMatrix4fv(u_mvp_,  1, GL_FALSE, glm::value_ptr(mvp));
        glUniform4fv(u_color_, 1, glm::value_ptr(sp.tint));
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    }
}

// ── Shader helpers ────────────────────────────────────────────────────────────

GLuint Renderer::compile_shader(GLenum type, const char* src) {
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    GLint ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(id, sizeof(log), nullptr, log);
        std::cerr << "[Renderer] shader error: " << log << "\n";
    }
    return id;
}

GLuint Renderer::link_program(GLuint vert, GLuint frag) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::cerr << "[Renderer] link error: " << log << "\n";
    }
    return prog;
}
