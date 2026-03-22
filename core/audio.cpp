#include "audio.hpp"
#include <iostream>

// ── init / shutdown ───────────────────────────────────────────────────────────

bool Audio::init() {
    ma_result r = ma_engine_init(nullptr, &engine_);
    if (r != MA_SUCCESS) {
        std::cerr << "[Audio] ma_engine_init failed: " << r << "\n";
        return false;
    }
    engine_ok_ = true;
    return true;
}

void Audio::shutdown() {
    for (auto& [id, snd] : sounds_)
        ma_sound_uninit(snd.get());
    sounds_.clear();

    if (engine_ok_)
        ma_engine_uninit(&engine_);
}

// ── load ──────────────────────────────────────────────────────────────────────

bool Audio::load(const std::string& id, const std::string& path) {
    if (!engine_ok_) return false;

    auto snd = std::make_unique<ma_sound>();
    ma_result r = ma_sound_init_from_file(
        &engine_, path.c_str(),
        MA_SOUND_FLAG_DECODE,   // decodifica en memoria al cargar
        nullptr, nullptr, snd.get()
    );
    if (r != MA_SUCCESS) {
        std::cerr << "[Audio] No se pudo cargar '" << path << "': " << r << "\n";
        return false;
    }
    sounds_[id] = std::move(snd);
    return true;
}

// ── play / stop ───────────────────────────────────────────────────────────────

void Audio::play(const std::string& id) {
    auto it = sounds_.find(id);
    if (it == sounds_.end()) return;
    ma_sound_seek_to_pcm_frame(it->second.get(), 0);
    ma_sound_start(it->second.get());
}

void Audio::stop(const std::string& id) {
    auto it = sounds_.find(id);
    if (it == sounds_.end()) return;
    ma_sound_stop(it->second.get());
}

void Audio::set_volume(float vol) {
    if (engine_ok_)
        ma_engine_set_volume(&engine_, vol);
}
