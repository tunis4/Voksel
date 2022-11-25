#include "camera.hpp"

Camera::Camera(glm::vec3 position) : m_pos(position) {
    m_world_up = glm::vec3(0, 1, 0);
    m_yaw = -90;
    m_pitch = 0;
    m_front = glm::vec3(0, 0, -1);
    m_mouse_sensitivity = 0.1;
    m_zoom = 90;
    m_free_speed = 10;
    update_vectors();
}

void Camera::update_matrices(usize window_width, usize window_height) {
    m_view_matrix = glm::lookAt(pos(), pos() + m_front, m_up);
    m_projection_matrix = glm::perspective(glm::radians(m_zoom), (f32)window_width / (f32)window_height, 0.01f, 1000.0f);
}

void Camera::process_mouse_movement(f32 mouse_x, f32 mouse_y) {
    if (m_first_mouse) {
        m_last_x = mouse_x;
        m_last_y = mouse_y;
        m_first_mouse = false;
    }

    float x_offset = mouse_x - m_last_x;
    float y_offset = m_last_y - mouse_y; // reversed since y-coordinates go from bottom to top

    m_last_x = mouse_x;
    m_last_y = mouse_y;

    x_offset *= m_mouse_sensitivity;
    y_offset *= m_mouse_sensitivity;

    m_yaw += x_offset;
    m_pitch += y_offset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (m_pitch > 89.0)
        m_pitch = 89.0;
    if (m_pitch < -89.0)
        m_pitch = -89.0;

    update_vectors();
}

void Camera::process_free_movement(MovementDirection direction, f32 delta_time) {
    f32 velocity = m_free_speed * delta_time;
    if (direction == MovementDirection::FORWARD)
        m_free_pos += m_front * velocity;
    if (direction == MovementDirection::BACKWARD)
        m_free_pos -= m_front * velocity;
    if (direction == MovementDirection::LEFT)
        m_free_pos -= m_right * velocity;
    if (direction == MovementDirection::RIGHT)
        m_free_pos += m_right * velocity;
}

void Camera::update_vectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front = glm::normalize(front);
    m_right = glm::normalize(glm::cross(m_front, m_world_up));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}

void Camera::set_free(bool free) { 
    if (!m_free_cam && free)
        m_free_pos = m_pos;
    m_free_cam = free;
}
