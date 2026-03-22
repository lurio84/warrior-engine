#include "atlas.hpp"

#include <stb_image.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>

using json = nlohmann::json;

bool Atlas::load(const std::string& json_path) {
    // ── Leer JSON ─────────────────────────────────────────────────────────────
    std::ifstream f(json_path);
    if (!f) {
        std::cerr << "[Atlas] No se puede abrir: " << json_path << "\n";
        return false;
    }
    json data;
    try { data = json::parse(f); }
    catch (const std::exception& e) {
        std::cerr << "[Atlas] JSON inválido: " << e.what() << "\n";
        return false;
    }

    // ── Cargar textura ────────────────────────────────────────────────────────
    std::string dir = json_path.substr(0, json_path.rfind('/') + 1);
    std::string png_path = dir + data.value("texture", "atlas.png");

    stbi_set_flip_vertically_on_load(true);  // OpenGL UV: v=0 en abajo
    int w, h, ch;
    unsigned char* pixels = stbi_load(png_path.c_str(), &w, &h, &ch, 4);
    if (!pixels) {
        std::cerr << "[Atlas] No se puede cargar imagen: " << png_path << "\n";
        return false;
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &tex_);
    glTextureStorage2D(tex_, 1, GL_RGBA8, w, h);
    glTextureSubImage2D(tex_, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTextureParameteri(tex_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(tex_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(tex_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(tex_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    stbi_image_free(pixels);

    // ── Calcular UVs ─────────────────────────────────────────────────────────
    float fw = static_cast<float>(w);
    float fh = static_cast<float>(h);

    for (auto& [id, sp] : data["sprites"].items()) {
        int sx = sp["x"], sy = sp["y"], sw = sp["w"], sh = sp["h"];
        SpriteUV uv;
        uv.u0 = sx / fw;
        uv.u1 = (sx + sw) / fw;
        // imagen cargada con flip: fila 0 original queda en v=1
        uv.v0 = (fh - (sy + sh)) / fh;
        uv.v1 = (fh - sy)        / fh;
        uvs_[id] = uv;
    }

    std::cout << "[Atlas] Cargado: " << png_path
              << "  (" << uvs_.size() << " sprites)\n";
    return true;
}

void Atlas::shutdown() {
    if (tex_) { glDeleteTextures(1, &tex_); tex_ = 0; }
}

bool Atlas::get_uv(const std::string& id, SpriteUV& out) const {
    auto it = uvs_.find(id);
    if (it == uvs_.end()) return false;
    out = it->second;
    return true;
}
