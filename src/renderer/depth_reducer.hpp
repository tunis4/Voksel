#pragma once

#include "context.hpp"
#include "pipeline.hpp"
#include "descriptor.hpp"

namespace renderer {
    class DepthReducer {
        struct PushConstants {
            static constexpr VkShaderStageFlags stage_flags = VK_SHADER_STAGE_COMPUTE_BIT;

            alignas( 8) vec2 image_size;
        };

        struct PerFrame {
            std::vector<VkDescriptorSet> descriptor_sets; // one for each depth pyramid mip level
        };

        Context &m_context;
        std::array<PerFrame, MAX_FRAMES_IN_FLIGHT> m_per_frame;
        VkDescriptorPool m_descriptor_pool; // local pool cause we have to recreate the sets when resizing the window
        DescriptorBuilder m_descriptor_builder;
        VkDescriptorSetLayout m_descriptor_set_layout;
        VkPipeline m_pipeline;
        VkPipelineLayout m_pipeline_layout;

    public:
        DepthReducer(Context &context) : m_context(context) {}

        void init();
        void cleanup();
        void create_descriptor_sets();
        void free_descriptor_sets();
        void record(VkCommandBuffer cmd, uint frame_index);
    };
};
