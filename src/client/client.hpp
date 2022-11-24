#pragma once

#include "window.hpp"
#include "renderer.hpp"
#include "../world/world.hpp"

class Client {
    Window *m_window;
    Renderer *m_renderer;
    Camera *m_camera;
    World *m_world;
    
    f64 m_delta_time, m_last_frame;

public:
    Client();
    ~Client();

    void loop();
    void process_input();
};
