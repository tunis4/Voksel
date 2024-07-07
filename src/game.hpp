#pragma once

#include <entt/entt.hpp>

#include "window.hpp"
#include "camera.hpp"
#include "world/world.hpp"

// Singleton
class Game {
    Window *m_window;
    Camera *m_camera;
    World *m_world;
    bool m_running = false;
    bool m_in_game = false;
    u64 m_tick_count = 0;
    f32 m_tick_delta_time;
    u64 m_frame_count = 0;
    f32 m_render_delta_time;

    entt::registry m_registry;
    entt::entity m_local_player;

public:
    std::thread m_main_thread;

    Game(bool in_game);
    ~Game();

    static Game* get();
    inline bool is_in_game() const { return m_in_game; }
    inline Window* window() const { return m_window; }
    inline Camera* camera() const { return m_camera; }
    inline World* world() const { return m_world; }

    void enter_singleplayer();
    void exit_singleplayer();

    void heartbeat();
    void tick();
    void render();
    void process_input();
};
