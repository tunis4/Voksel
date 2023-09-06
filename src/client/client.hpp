#pragma once

#include "window.hpp"
#include "renderer/renderer.hpp"
#include "renderer/chunk_renderer.hpp"
#include "../world/world.hpp"

namespace client {
    // Singleton
    class Client {
        Window *m_window;
        Camera *m_camera;
        world::World *m_world;
        
        f32 m_delta_time;

    public:
        Client();
        ~Client();

        static Client* get();

        inline world::World* world() const { return m_world; }

        void loop();
        void process_input();

        glm::vec3 resolve_movement(glm::vec3 old_pos, glm::vec3 new_pos, glm::vec3 min_bounds, glm::vec3 max_bounds);
    };
}
