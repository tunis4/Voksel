#include "script_engine.hpp"

ScriptEngine::ScriptEngine() {
    m_lua.open_libraries();
    m_game_table = m_lua.create_named_table("game");
    m_game_table["listen"] = [&](std::string event_name, sol::function handler) {
        m_events[event_name].push_back(handler);
    };
}

ScriptEngine::~ScriptEngine() {
    m_events.clear();
}

sol::table& ScriptEngine::game_table() {
    return m_game_table;
}

void ScriptEngine::run_script(std::filesystem::path path) {
    m_lua.script_file(path.generic_string());
}
