#pragma once

#include "window.hpp"
#include "renderer.hpp"
#include "../world/world.hpp"

class Client {
    Window *m_window;
    Renderer *m_renderer;
    World *m_world;
    Camera *m_camera;
    
    f64 m_delta_time, m_last_frame;

public:
    Client();
    ~Client();

    void loop();
    void process_input();
};
