#pragma once

#include "window.hpp"
#include "gl/shader.hpp"
#include "../world/world.hpp"
#include "../scripting/script_engine.hpp"

class Client {
    Window *m_window;
    Shader *m_shader;
    ScriptEngine *m_script_engine;
    World *m_world;

public:
    Client();
    ~Client();

    void loop();
    void process_input();
};
