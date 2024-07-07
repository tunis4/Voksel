#pragma once

#include <cstdint>
#include <cstddef>
#include <utility>
#include <concepts>
#include <iostream>
#include <ctime>
#include <iomanip>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/vector_int3_sized.hpp>
#include <glm/ext/vector_float3.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/io.hpp>

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

using glm::vec2;
using glm::vec3;
using glm::vec4;

using glm::mat3;
using glm::mat4;

using glm::i32vec2;
using glm::i32vec3;
using glm::i32vec4;

namespace moodycamel {}
using namespace moodycamel;

enum class Direction {
    NORTH, SOUTH, EAST, WEST
};

enum class MovementDirection {
    FORWARD, BACKWARD, LEFT, RIGHT
};

template<std::signed_integral T>
inline constexpr std::pair<T, T> signed_int_divide(T dividend, T divisor) {
    T q = dividend / divisor;
    T r = dividend % divisor;
    if (r < 0) {
        q--;
        r = dividend - (q * divisor);
    }
    return std::make_pair(q, r);
}

inline constexpr std::pair<i32vec3, i32vec3> signed_i32vec3_divide(i32vec3 v, i32 divisor) {
    auto [xq, xr] = signed_int_divide(v.x, divisor);
    auto [yq, yr] = signed_int_divide(v.y, divisor);
    auto [zq, zr] = signed_int_divide(v.z, divisor);
    return std::make_pair(i32vec3(xq, yq, zq), i32vec3(xr, yr, zr));
}

inline constexpr i32 i32vec3_distance_squared(i32vec3 a, i32vec3 b) {
    i32vec3 d = a - b;
    return d.x * d.x + d.y * d.y + d.z * d.z;
}

template<usize size, std::integral T>
inline constexpr T coords_to_index(T x, T y, T z) {
    return y * size * size + z * size + x;
}

inline bool aabb_intersect(vec3 min_a, vec3 max_a, vec3 min_b, vec3 max_b) {
    return (min_a.x <= max_b.x && max_a.x >= min_b.x) &&
            (min_a.y <= max_b.y && max_a.y >= min_b.y) &&
            (min_a.z <= max_b.z && max_a.z >= min_b.z);
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
#ifdef __linux__
    switch (level) {
    case LogLevel::INFO: std::cout << "\033[1;37m"; break;
    case LogLevel::WARN: std::cout << "\033[1;33m"; break;
    case LogLevel::ERROR: std::cout << "\033[1;31m"; break;
    }
#endif

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::cout << std::put_time(&tm, "[%H:%M:%S]");

    switch (level) {
    case LogLevel::INFO: std::cout << " [INFO] "; break;
    case LogLevel::WARN: std::cout << " [WARN] "; break;
    case LogLevel::ERROR: std::cout << " [ERROR] "; break;
    }

    std::cout << "[" << context << "] ";

#ifdef __linux__
    std::cout << "\033[0;m";
    switch (level) {
    case LogLevel::INFO: std::cout << "\033[37m"; break;
    case LogLevel::WARN: std::cout << "\033[33m"; break;
    case LogLevel::ERROR: std::cout << "\033[31m"; break;
    }
#endif

    std::cout << std::setprecision(3);
    rprintf(format, args...);

#ifdef __linux__
    std::cout << "\033[0;m";
#endif
    std::cout << std::endl;
}

template<typename T> concept TriviallyCopyable = std::is_trivially_copyable<T>::value;

struct ScopeExitTag {};

template<typename Function>
class ScopeExit final {
    Function function;
public:
    explicit ScopeExit(Function &&fn) : function(std::move(fn)) {}
    ~ScopeExit() {
        function();
    }
};

template<typename Function>
auto operator->*(ScopeExitTag, Function &&function) {
    return ScopeExit<Function>{std::forward<Function>(function)};
}

#define CONCAT(a, b) a ## b
#define CONCAT2(a, b) CONCAT(a, b)

#define defer auto CONCAT2(_defer, __LINE__) = ::ScopeExitTag{}->*[&]()
