#pragma once

#include <thread>
#include <vector>
#include <functional>
#include <semaphore>
#include "util.hpp"

class ParallelExecutor {
    std::vector<std::thread> m_workers;
    std::counting_semaphore<> m_main_to_threads_semaphore;
    std::counting_semaphore<> m_threads_to_main_semaphore;
    std::function<void(u8)> m_task;
    std::atomic_flag m_running = ATOMIC_FLAG_INIT;

public:
    u8 num_workers;

    ParallelExecutor() : m_main_to_threads_semaphore(0), m_threads_to_main_semaphore(0) {
        num_workers = std::thread::hardware_concurrency() - 2;
        // num_workers = 0;
        m_running.test_and_set();
        for (u8 i = 0; i < num_workers; ++i) {
            m_workers.emplace_back([this, i] {
                while (true) {
                    m_main_to_threads_semaphore.acquire();
                    if (!m_running.test())
                        break;
                    m_task(i + 1);
                    m_threads_to_main_semaphore.release();
                }
            });
        }
    }

    ~ParallelExecutor() {
        m_running.clear();
        m_main_to_threads_semaphore.release(num_workers);
        for (auto &thread : m_workers)
            thread.join();
    }

    template<typename F>
    void run(F &&task) {
        m_task = task;
        m_main_to_threads_semaphore.release(num_workers);
        m_task(0);
        for (u8 i = 0; i < num_workers; i++)
            m_threads_to_main_semaphore.acquire();
    }
};
