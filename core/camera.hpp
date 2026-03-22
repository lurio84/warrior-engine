#pragma once

struct Camera {
    float x    = 7.f;
    float y    = 7.f;
    float zoom = 2.f;

    void zoom_in()    {
        if      (zoom < 0.75f) zoom = 1.f;
        else if (zoom < 1.5f)  zoom = 2.f;
        else if (zoom < 3.f)   zoom = 4.f;
    }
    void zoom_out()   {
        if      (zoom > 3.f)   zoom = 2.f;
        else if (zoom > 1.5f)  zoom = 1.f;
        else if (zoom > 0.75f) zoom = 0.5f;
    }
    void zoom_reset() { zoom = 2.f; }
};
