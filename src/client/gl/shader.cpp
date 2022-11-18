#include "shader.hpp"

#include <fstream>
#include <sstream>
#include <iostream> 

Shader::Shader(const std::filesystem::path &vertex_path, const std::filesystem::path &fragment_path) {
    std::string vertex_code;
    std::string fragment_code;
    std::ifstream v_shader_file;
    std::ifstream f_shader_file;

    v_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    f_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        v_shader_file.open(vertex_path);
        f_shader_file.open(fragment_path);
        std::stringstream v_shader_stream, f_shader_stream;

        v_shader_stream << v_shader_file.rdbuf();
        f_shader_stream << f_shader_file.rdbuf();

        v_shader_file.close();
        f_shader_file.close();

        vertex_code = v_shader_stream.str();
        fragment_code = f_shader_stream.str();
    } catch (std::ifstream::failure& e) {
        std::cerr << "Failed to read shader file" << std::endl;
    }

    const char *v_shader_code = vertex_code.c_str();
    const char *f_shader_code = fragment_code.c_str();
    unsigned int vertex, fragment;

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &v_shader_code, NULL);
    glCompileShader(vertex);
    check_compile_errors(vertex, "Vertex");

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &f_shader_code, NULL);
    glCompileShader(fragment);
    check_compile_errors(fragment, "Fragment");

    m_id = glCreateProgram();
    glAttachShader(m_id, vertex);
    glAttachShader(m_id, fragment);
    glLinkProgram(m_id);
    check_compile_errors(m_id, "Program");

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::use() {
    glUseProgram(m_id);
}

void Shader::set_bool(const std::string &name, bool value) const {
    glUniform1i(glGetUniformLocation(m_id, name.c_str()), (int)value);
}

void Shader::set_int(const std::string &name, int value) const {
    glUniform1i(glGetUniformLocation(m_id, name.c_str()), value);
}

void Shader::set_float(const std::string &name, f32 value) const {
    glUniform1f(glGetUniformLocation(m_id, name.c_str()), value);
}

void Shader::set_vec2(const std::string &name, const glm::vec2 &value) const {
    glUniform2fv(glGetUniformLocation(m_id, name.c_str()), 1, &value[0]);
}

void Shader::set_vec3(const std::string &name, const glm::vec3 &value) const {
    glUniform3fv(glGetUniformLocation(m_id, name.c_str()), 1, &value[0]);
}

void Shader::set_vec4(const std::string &name, const glm::vec4 &value) const {
    glUniform4fv(glGetUniformLocation(m_id, name.c_str()), 1, &value[0]);
}

void Shader::set_mat2(const std::string &name, const glm::mat2 &mat) const {
    glUniformMatrix2fv(glGetUniformLocation(m_id, name.c_str()), 1, false, &mat[0][0]);
}

void Shader::set_mat3(const std::string &name, const glm::mat3 &mat) const {
    glUniformMatrix3fv(glGetUniformLocation(m_id, name.c_str()), 1, false, &mat[0][0]);
}
    
void Shader::set_mat4(const std::string &name, const glm::mat4 &mat) const {
    glUniformMatrix4fv(glGetUniformLocation(m_id, name.c_str()), 1, false, &mat[0][0]);
}

void Shader::check_compile_errors(uint shader, std::string type) {
    int success;
    char info_log[1024];
    if (type != "Program") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, info_log);
            std::cout << "Shader compilation error of type: " << type << std::endl << info_log << std::endl;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, info_log);
            std::cout << "Program linking error of type: " << type << std::endl << info_log << std::endl;
        }
    }
}