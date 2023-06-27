#pragma once

#include "pipeline.hpp"
#include "persistent_buffer.hpp"

#include <glm/glm.hpp>

namespace render {
    class BoxRenderer {
        struct UniformBuffer {
            // for vertex shader
            alignas(16) glm::mat4 view;
            alignas(16) glm::mat4 projection;

            // for fragment shader
            alignas( 4) f32 thickness;
        };

        struct PushConstants {
            alignas(16) glm::mat4 model;
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

        bool m_show_box;
        glm::vec3 m_box_pos;
    
    public:
        BoxRenderer(Context &context) : m_context(context) {}

        void init();
        void cleanup();
        void set_box(bool show, glm::vec3 pos = glm::vec3(0.0f));
        void record(VkCommandBuffer cmd, uint frame_index);
    };
};
