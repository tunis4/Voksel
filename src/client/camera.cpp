#include "camera.hpp"
#include "src/client/client.hpp"

namespace client {
    Camera::Camera(glm::vec3 position) : m_pos(position) {
        m_world_up = glm::vec3(0, -1, 0);
        m_yaw = -90;
        m_pitch = 0;
        m_front = glm::vec3(0, 0, 1);
        m_mouse_sensitivity = 0.1;
        m_fov = 90;
        m_free_speed = normal_free_speed;
        update_vectors();
    }

    void Camera::update_matrices(usize window_width, usize window_height) {
        m_view_matrix = glm::lookAt(pos(), pos() + m_front, m_up);
        m_projection_matrix = glm::perspective(glm::radians(m_fov), (f32)window_width / (f32)window_height, 0.01f, 1000.0f);
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

    void Camera::process_free_movement(util::MovementDirection direction, f32 delta_time) {
        f32 velocity = m_free_speed * delta_time;
        if (direction == util::MovementDirection::FORWARD)
            m_free_pos += m_front * velocity;
        if (direction == util::MovementDirection::BACKWARD)
            m_free_pos -= m_front * velocity;
        if (direction == util::MovementDirection::LEFT)
            m_free_pos += m_right * velocity;
        if (direction == util::MovementDirection::RIGHT)
            m_free_pos -= m_right * velocity;
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

    void Camera::Frustum::update(glm::mat4 projview) {
        glm::mat4 m = projview;
        m = glm::transpose(m);
        m_planes[LEFT]   = m[3] + m[0];
        m_planes[RIGHT]  = m[3] - m[0];
        m_planes[BOTTOM] = m[3] + m[1];
        m_planes[TOP]    = m[3] - m[1];
        m_planes[NEAR]   = m[3] + m[2];
        m_planes[FAR]    = m[3] - m[2];

        glm::vec3 crosses[COMBINATIONS] = {
            glm::cross(glm::vec3(m_planes[LEFT]),   glm::vec3(m_planes[RIGHT])),
            glm::cross(glm::vec3(m_planes[LEFT]),   glm::vec3(m_planes[BOTTOM])),
            glm::cross(glm::vec3(m_planes[LEFT]),   glm::vec3(m_planes[TOP])),
            glm::cross(glm::vec3(m_planes[LEFT]),   glm::vec3(m_planes[NEAR])),
            glm::cross(glm::vec3(m_planes[LEFT]),   glm::vec3(m_planes[FAR])),
            glm::cross(glm::vec3(m_planes[RIGHT]),  glm::vec3(m_planes[BOTTOM])),
            glm::cross(glm::vec3(m_planes[RIGHT]),  glm::vec3(m_planes[TOP])),
            glm::cross(glm::vec3(m_planes[RIGHT]),  glm::vec3(m_planes[NEAR])),
            glm::cross(glm::vec3(m_planes[RIGHT]),  glm::vec3(m_planes[FAR])),
            glm::cross(glm::vec3(m_planes[BOTTOM]), glm::vec3(m_planes[TOP])),
            glm::cross(glm::vec3(m_planes[BOTTOM]), glm::vec3(m_planes[NEAR])),
            glm::cross(glm::vec3(m_planes[BOTTOM]), glm::vec3(m_planes[FAR])),
            glm::cross(glm::vec3(m_planes[TOP]),    glm::vec3(m_planes[NEAR])),
            glm::cross(glm::vec3(m_planes[TOP]),    glm::vec3(m_planes[FAR])),
            glm::cross(glm::vec3(m_planes[NEAR]),   glm::vec3(m_planes[FAR]))
        };

        m_points[0] = intersection<LEFT,  BOTTOM, NEAR>(crosses);
        m_points[1] = intersection<LEFT,  TOP,    NEAR>(crosses);
        m_points[2] = intersection<RIGHT, BOTTOM, NEAR>(crosses);
        m_points[3] = intersection<RIGHT, TOP,    NEAR>(crosses);
        m_points[4] = intersection<LEFT,  BOTTOM, FAR>(crosses);
        m_points[5] = intersection<LEFT,  TOP,    FAR>(crosses);
        m_points[6] = intersection<RIGHT, BOTTOM, FAR>(crosses);
        m_points[7] = intersection<RIGHT, TOP,    FAR>(crosses);
    }
    
    bool Camera::Frustum::is_box_visible(const glm::vec3 &min, const glm::vec3 &max) const {
        // check box outside/inside of frustum
        for (int i = 0; i < COUNT; i++) {
            if ((glm::dot(m_planes[i], glm::vec4(min.x, min.y, min.z, 1.0f)) < 0.0) &&
                (glm::dot(m_planes[i], glm::vec4(max.x, min.y, min.z, 1.0f)) < 0.0) &&
                (glm::dot(m_planes[i], glm::vec4(min.x, max.y, min.z, 1.0f)) < 0.0) &&
                (glm::dot(m_planes[i], glm::vec4(max.x, max.y, min.z, 1.0f)) < 0.0) &&
                (glm::dot(m_planes[i], glm::vec4(min.x, min.y, max.z, 1.0f)) < 0.0) &&
                (glm::dot(m_planes[i], glm::vec4(max.x, min.y, max.z, 1.0f)) < 0.0) &&
                (glm::dot(m_planes[i], glm::vec4(min.x, max.y, max.z, 1.0f)) < 0.0) &&
                (glm::dot(m_planes[i], glm::vec4(max.x, max.y, max.z, 1.0f)) < 0.0))
            {
                return false;
            }
        }

        // check frustum outside/inside box
        int out;
        out = 0; for (int i = 0; i < 8; i++) out += ((m_points[i].x > max.x) ? 1 : 0); if (out == 8) return false;
        out = 0; for (int i = 0; i < 8; i++) out += ((m_points[i].x < min.x) ? 1 : 0); if (out == 8) return false;
        out = 0; for (int i = 0; i < 8; i++) out += ((m_points[i].y > max.y) ? 1 : 0); if (out == 8) return false;
        out = 0; for (int i = 0; i < 8; i++) out += ((m_points[i].y < min.y) ? 1 : 0); if (out == 8) return false;
        out = 0; for (int i = 0; i < 8; i++) out += ((m_points[i].z > max.z) ? 1 : 0); if (out == 8) return false;
        out = 0; for (int i = 0; i < 8; i++) out += ((m_points[i].z < min.z) ? 1 : 0); if (out == 8) return false;

        return true;
    }
}
