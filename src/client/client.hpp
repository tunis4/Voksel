#pragma once

#include "window.hpp"
#include "renderer/renderer.hpp"
#include "../world/world.hpp"

namespace client {
    // Singleton
    class Client {
        Window *m_window;
        render::Renderer *m_renderer;
        world::World *m_world;
        Camera *m_camera;
        
        f64 m_delta_time, m_last_frame;

    public:
        Client();
        ~Client();

        static Client* get();

        inline world::World* world() const { return m_world; }

        void loop();
        void process_input();
    };
}
