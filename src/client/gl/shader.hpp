#pragma once

#include <string>
#include <filesystem>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include "../../util.hpp"

class Shader {
    uint m_id;

    void check_compile_errors(uint shader, std::string type);

public:
    Shader(const std::filesystem::path &vertex_path, const std::filesystem::path &fragment_path);
    ~Shader();

    void use();

    void set_bool(const std::string &name, bool value) const;
    void set_int(const std::string &name, int value) const;
    void set_float(const std::string &name, f32 value) const;
    void set_vec2(const std::string &name, const glm::vec2 &value) const;
    void set_vec3(const std::string &name, const glm::vec3 &value) const;
    void set_vec4(const std::string &name, const glm::vec4 &value) const;
    void set_mat2(const std::string &name, const glm::mat2 &mat) const;
    void set_mat3(const std::string &name, const glm::mat3 &mat) const;
    void set_mat4(const std::string &name, const glm::mat4 &mat) const;
};
