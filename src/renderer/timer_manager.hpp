#pragma once

#include "context.hpp"

namespace renderer {
    class TimerManager {
        struct Timer {

        };

        struct PerFrame {
            std::unordered_map<std::string, Timer> timers;
        };

        Context &m_context;
        std::array<PerFrame, MAX_FRAMES_IN_FLIGHT> m_per_frame;

        u64 m_timestamp_period;

    public:
        TimerManager(Context &context) : m_context(context) {}

        void init();
        void cleanup();
    };
}
