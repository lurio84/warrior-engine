#pragma once
#include <glad/glad.h>
#include <string>
#include <unordered_map>

struct SpriteUV {
    float u0, v0, u1, v1;  // normalized, ready for the shader
};

class Atlas {
public:
    // json_path: ruta a atlas.json (atlas.png debe estar en el mismo directorio)
    bool load(const std::string& json_path);
    void shutdown();

    GLuint texture() const { return tex_; }

    // Devuelve false si el ID no existe
    bool get_uv(const std::string& id, SpriteUV& out) const;

private:
    GLuint tex_ = 0;
    std::unordered_map<std::string, SpriteUV> uvs_;
};
