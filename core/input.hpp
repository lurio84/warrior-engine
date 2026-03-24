#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <cstring>

// InputSystem — Phase 10
// Snapshot del teclado por frame. Llamar update() una vez por frame
// antes del tick de Lua. Expuesto a Lua via engine.input.*.

class InputSystem {
public:
    void update() {
        // Teclado
        std::memcpy(prev_, cur_, SDL_NUM_SCANCODES);
        int n = 0;
        const uint8_t* s = SDL_GetKeyboardState(&n);
        std::memcpy(cur_, s, n);

        // Ratón
        mouse_prev_ = mouse_cur_;
        mouse_cur_  = SDL_GetMouseState(&mouse_x_, &mouse_y_);
    }

    // Tecla mantenida este frame
    bool down(const std::string& name) const {
        SDL_Scancode sc = SDL_GetScancodeFromName(name.c_str());
        if (sc == SDL_SCANCODE_UNKNOWN) sc = alias(name);
        return sc != SDL_SCANCODE_UNKNOWN && cur_[sc];
    }

    // Tecla recién pulsada (flanco de subida)
    bool pressed(const std::string& name) const {
        SDL_Scancode sc = SDL_GetScancodeFromName(name.c_str());
        if (sc == SDL_SCANCODE_UNKNOWN) sc = alias(name);
        return sc != SDL_SCANCODE_UNKNOWN && cur_[sc] && !prev_[sc];
    }

    // ── Ratón ─────────────────────────────────────────────────────────────────
    std::pair<int,int> mouse_pos() const { return { mouse_x_, mouse_y_ }; }

    // btn: SDL_BUTTON_LEFT (1), SDL_BUTTON_RIGHT (3), SDL_BUTTON_MIDDLE (2)
    bool mouse_down(int btn)    const { return (mouse_cur_  & SDL_BUTTON(btn)) != 0; }
    bool mouse_pressed(int btn) const {
        return (mouse_cur_ & SDL_BUTTON(btn)) != 0
            && (mouse_prev_ & SDL_BUTTON(btn)) == 0;
    }

private:
    uint8_t  cur_ [SDL_NUM_SCANCODES]{};
    uint8_t  prev_[SDL_NUM_SCANCODES]{};

    uint32_t mouse_cur_  = 0;
    uint32_t mouse_prev_ = 0;
    int      mouse_x_    = 0;
    int      mouse_y_    = 0;

    // Alias cortos que SDL no reconoce por nombre
    static SDL_Scancode alias(const std::string& n) {
        if (n == "up")     return SDL_SCANCODE_UP;
        if (n == "down")   return SDL_SCANCODE_DOWN;
        if (n == "left")   return SDL_SCANCODE_LEFT;
        if (n == "right")  return SDL_SCANCODE_RIGHT;
        if (n == "space")  return SDL_SCANCODE_SPACE;
        if (n == "enter")  return SDL_SCANCODE_RETURN;
        if (n == "escape") return SDL_SCANCODE_ESCAPE;
        if (n == "shift")  return SDL_SCANCODE_LSHIFT;
        if (n == "ctrl")   return SDL_SCANCODE_LCTRL;
        if (n == "w")      return SDL_SCANCODE_W;
        if (n == "a")      return SDL_SCANCODE_A;
        if (n == "s")      return SDL_SCANCODE_S;
        if (n == "d")      return SDL_SCANCODE_D;
        if (n == "e")      return SDL_SCANCODE_E;
        if (n == "q")      return SDL_SCANCODE_Q;
        if (n == "f")      return SDL_SCANCODE_F;
        return SDL_SCANCODE_UNKNOWN;
    }
};
