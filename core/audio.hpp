#pragma once
#include <miniaudio.h>
#include <string>
#include <unordered_map>
#include <memory>

// Audio system — Phase 7
// Wraps miniaudio: engine init, load WAV/OGG/FLAC, play/stop per ID.

class Audio {
public:
    bool init();
    void shutdown();

    // Carga un archivo de audio y lo asocia a un ID.
    // Devuelve false si el archivo no existe (no-fatal).
    bool load(const std::string& id, const std::string& path);

    void play(const std::string& id);
    void stop(const std::string& id);
    void set_volume(float vol);   // 0..1, master

private:
    ma_engine engine_{};
    bool      engine_ok_ = false;

    // ma_sound no se puede mover/copiar tras init → guardamos por puntero
    std::unordered_map<std::string, std::unique_ptr<ma_sound>> sounds_;
};
