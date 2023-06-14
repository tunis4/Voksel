#pragma once

#include <cstdint>
#include <cstddef>
#include <utility>
#include <concepts>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <glm/ext/vector_int3_sized.hpp>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using uint = unsigned int;
using usize = size_t;
using uptr = uintptr_t;

using f32 = float;
using f64 = double;

namespace util {
    enum class Direction {
        NORTH, SOUTH, WEST, EAST
    };

    enum class MovementDirection {
        FORWARD, BACKWARD, LEFT, RIGHT
    };

    template<std::signed_integral T>
    inline constexpr std::pair<T, T> signed_int_divide(T x, T divisor) {
        T d = x / divisor;
        T r = x % divisor;
        if (r < 0) {
            d--;
            r = -(d * divisor) + x;
        }
        return std::make_pair(d, r);
    }

    inline constexpr std::pair<glm::i32vec3, glm::i32vec3> signed_i32vec3_divide(glm::i32vec3 v, i32 divisor) {
        auto [xd, xr] = signed_int_divide(v.x, divisor);
        auto [yd, yr] = signed_int_divide(v.y, divisor);
        auto [zd, zr] = signed_int_divide(v.z, divisor);
        return std::make_pair(glm::i32vec3(xd, yd, zd), glm::i32vec3(xr, yr, zr));
    }

    template<usize size, std::integral T>
    inline constexpr T coords_to_index(T x, T y, T z) {
        return y * size * size + z * size + x;
    }

    inline void rprintf(const char *format) {
        std::cout << format;
    }
    
    template<typename T, typename... TArgs>
    inline void rprintf(const char *format, T value, TArgs... f_args) {
        for (; *format; format++) {
            if (*format == '{') {
                format++;
                if (*format == '}') {
                    std::cout << value;
                    rprintf(format + 1, f_args...); // recursive call
                    return;
                }
                std::cout << *(format - 1);
                std::cout << *format;
            }
            std::cout << *format;
        }
    }

    enum LogLevel {
        INFO,
        WARN,
        ERROR
    };

    template<typename... Args>
    void log(LogLevel level, const char *context, const char *format, Args... args) {
        switch (level) {
        case LogLevel::INFO: std::cout << "\033[1;37m"; break;
        case LogLevel::WARN: std::cout << "\033[1;33m"; break;
        case LogLevel::ERROR: std::cout << "\033[1;31m"; break;
        }

        auto t = std::time(nullptr);
        auto tm = *std::localtime(&t);
        std::cout << std::put_time(&tm, "[%H:%M:%S]");

        switch (level) {
        case LogLevel::INFO: std::cout << " [INFO] "; break;
        case LogLevel::WARN: std::cout << " [WARN] "; break;
        case LogLevel::ERROR: std::cout << " [ERROR] "; break;
        }

        std::cout << "[" << context << "] ";
        std::cout << "\033[0;m";
        switch (level) {
        case LogLevel::INFO: std::cout << "\033[37m"; break;
        case LogLevel::WARN: std::cout << "\033[33m"; break;
        case LogLevel::ERROR: std::cout << "\033[31m"; break;
        }
        rprintf(format, args...);
        std::cout << "\033[0;m" << std::endl;
    }
}
