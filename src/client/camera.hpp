#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../world/world.hpp"
#include "../util/util.hpp"

namespace client {
    class Camera {
    public:
        static constexpr f32 normal_free_speed = 5;
        static constexpr f32 slow_free_speed = normal_free_speed / 2;
        static constexpr f32 fast_free_speed = normal_free_speed * 2;

        static constexpr f32 default_fov = 90;

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
        f32 m_fov;

        glm::mat4 m_projection_matrix;
        glm::mat4 m_view_matrix;

        // variables for handling mouse movement
        f32 m_last_x, m_last_y;
        bool m_first_mouse = true;

        world::World::RayCastResult m_ray_cast {};

        explicit Camera(glm::vec3 position);

        void update_matrices(usize window_width, usize window_height);
        void process_mouse_movement(f32 mouse_x, f32 mouse_y);
        void process_free_movement(util::MovementDirection direction, f32 delta_time);

        inline bool is_free() { return m_free_cam; }
        void set_free(bool free);

        inline glm::vec3 pos() { return !m_free_cam ? m_pos : m_free_pos; }
        inline glm::i32vec3 block_pos() { return glm::floor(pos()); }

        struct Frustum {
            enum Planes {
                LEFT = 0,
                RIGHT,
                BOTTOM,
                TOP,
                NEAR,
                FAR,
                COUNT,
                COMBINATIONS = COUNT * (COUNT - 1) / 2
            };

            inline static constexpr int ij2k(Planes i, Planes j) {
                return i * (9 - i) / 2 + j - 1;
            } 

            template<Planes a, Planes b, Planes c>
            inline glm::vec3 intersection(const glm::vec3 *crosses) const {
                float D = glm::dot(glm::vec3(m_planes[a]), crosses[ij2k(b, c)]);
                glm::vec3 res = glm::mat3(crosses[ij2k(b, c)], -crosses[ij2k(a, c)], crosses[ij2k(a, b)]) *
                    glm::vec3(m_planes[a].w, m_planes[b].w, m_planes[c].w);
                return res * (-1.0f / D);
            }
            
            glm::vec4 m_planes[COUNT];
            glm::vec3 m_points[8];

            void update(glm::mat4 projview);
            bool is_box_visible(const glm::vec3 &min, const glm::vec3 &max) const;
        };

        Frustum m_frustum;

    private:
        void update_vectors();
    };
}
