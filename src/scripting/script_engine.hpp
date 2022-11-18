#pragma once

#include <list>
#include <filesystem>
#include <sol/sol.hpp>

class ScriptEngine {
    sol::state m_lua;
    sol::table m_game_table;
    std::unordered_map<std::string, std::list<sol::function>> m_events;

public:
    ScriptEngine();
    ~ScriptEngine();
    ScriptEngine(const ScriptEngine&) = delete;
    void operator =(const ScriptEngine&) = delete;

    sol::table& game_table();
    void run_script(std::filesystem::path path);

    template<typename... Args>
    void fire_event(std::string event_name, Args... args) {
        if (m_events.contains(event_name)) {
            for (auto &handler : m_events[event_name]) {
                handler.call<void>(args...);
            }
        }
    }
};
