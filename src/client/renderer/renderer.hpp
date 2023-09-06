#pragma once

#include <optional>

#include "chunk_renderer.hpp"
#include "common.hpp"
#include "context.hpp"
#include "pipeline.hpp"
#include "../window.hpp"
#include "../camera.hpp"

#ifdef DEBUG
    #define ENABLE_VK_VALIDATION_LAYERS
#endif

namespace render {
    class Renderer {
        bool m_framebuffer_resized = false;

        struct PerFrame {
            VkCommandBuffer m_command_buffer;
            VkSemaphore m_image_available_semaphore;
            VkSemaphore m_render_finished_semaphore;
            VkFence m_in_flight_fence;
        };

        std::array<PerFrame, MAX_FRAMES_IN_FLIGHT> m_per_frame;
        uint m_frame_index = 0;

#ifdef ENABLE_VK_VALIDATION_LAYERS
        VkDebugUtilsMessengerEXT m_vk_debug_messenger;
        void setup_debug_messenger();
        bool check_validation_layer_support();
#endif

        void create_instance();
        std::vector<const char*> get_instance_extensions();
        bool is_device_suitable(VkPhysicalDevice physical_device);
        void pick_physical_device();
        bool check_device_extension_support(VkPhysicalDevice physical_device);
        void create_logical_device();
        void create_surface();
        void create_render_pass();
        void create_command_pools();
        void create_command_buffers();
        void record_command_buffer(VkCommandBuffer command_buffer, uint image_index);
        void create_sync_objects();
        void create_allocator();
        void create_descriptor_pool();
        void setup_dear_imgui();

    public:
        Context m_context;

        glm::vec3 m_fog_color;

        void init(client::Window *window, client::Camera *camera);
        void begin_cleanup();
        void cleanup();
        
        void render(f64 delta_time);

        void on_framebuffer_resize(uint width, uint height);
        void on_cursor_move(f64 x, f64 y);
    };
}
