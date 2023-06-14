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
    struct alignas(16) UniformBuffer {
        glm::mat4 view;
        glm::mat4 projection;
    };

    struct alignas(16) PushConstants {
        glm::mat4 model;
    };

    class Renderer {
        Context m_context;
        
        bool m_framebuffer_resized;

        VkDescriptorSetLayout m_descriptor_set_layout;

        struct PerFrame {
            VkCommandBuffer m_command_buffer;
            VkSemaphore m_image_available_semaphore;
            VkSemaphore m_render_finished_semaphore;
            VkFence m_in_flight_fence;
        };

        std::array<PerFrame, MAX_FRAMES_IN_FLIGHT> m_per_frame;
        uint m_frame_index;

        ChunkRenderer *m_chunk_renderer;

#ifdef ENABLE_VK_VALIDATION_LAYERS
        VkDebugUtilsMessengerEXT m_vk_debug_messenger;
        void setup_debug_messenger();
        bool check_validation_layer_support();
#endif

        void init_vulkan();
        void cleanup_vulkan();
        void create_instance();
        std::vector<const char*> get_required_extensions();
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
        Renderer(client::Window *window, client::Camera *camera);
        ~Renderer();

        void render();

        void on_framebuffer_resize(uint width, uint height);
        void on_cursor_move(f64 x, f64 y);
    };
}
