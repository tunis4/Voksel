#pragma once

#include "pipeline.hpp"
#include "persistent_buffer.hpp"

#include <array>

namespace renderer {
    class SkyRenderer {
        struct UniformBuffer {
            // for vertex shader
            alignas(16) mat4 view;
            alignas(16) mat4 projection;

            // for fragment shader
            alignas(16) vec3 fog_color;
            alignas( 4) f32 fog_near;
            alignas( 4) f32 fog_far;
        };

        struct PushConstants {
            static constexpr VkShaderStageFlags stage_flags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

            // for vertex shader
            alignas(16) mat4 model;

            // for fragment shader
            alignas(16) vec3 color;
        };

        struct PerFrame {
            VkDescriptorSet descriptor_set;
            PersistentBuffer uniform_buffer;
        };

        Context &m_context;
        std::array<PerFrame, MAX_FRAMES_IN_FLIGHT> m_per_frame;
        VkDescriptorSetLayout m_descriptor_set_layout;
        VkPipeline m_pipeline;
        VkPipelineLayout m_pipeline_layout;

    public:
        SkyRenderer(Context &context) : m_context(context) {}

        void init();
        void cleanup();
        void record(VkCommandBuffer cmd, uint frame_index);
    };
};
