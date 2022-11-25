#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../util.hpp"

class Camera {
public:
    glm::vec3 m_pos;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_world_up;

    bool m_free_cam = false;
    glm::vec3 m_free_pos;
    f32 m_free_speed;

    f32 m_yaw, m_pitch;

    f32 m_mouse_sensitivity;
    f32 m_zoom;

    glm::mat4 m_projection_matrix;
    glm::mat4 m_view_matrix;

    // variables for handling mouse movement
    f32 m_last_x, m_last_y;
    bool m_first_mouse = true;

    explicit Camera(glm::vec3 position);

    void update_matrices(usize window_width, usize window_height);
    void process_mouse_movement(f32 mouse_x, f32 mouse_y);
    void process_free_movement(MovementDirection direction, f32 delta_time);

    inline bool is_free() { return m_free_cam; }
    void set_free(bool free);

    inline glm::vec3 pos() { return !m_free_cam ? m_pos : m_free_pos; }

private:
    void update_vectors();
};
