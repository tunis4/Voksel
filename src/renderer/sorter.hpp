#pragma once

#include "context.hpp"
#include "storage_buffer.hpp"
#include "chunk_renderer.hpp"

namespace renderer {
    class Sorter {
        enum Algorithm {
            LOCAL_BMS,
            LOCAL_DISPERSE,
            BIG_FLIP,
            BIG_DISPERSE
        };

        struct PushConstants {
            static constexpr VkShaderStageFlags stage_flags = VK_SHADER_STAGE_COMPUTE_BIT;

            alignas( 4) u32 h;
            alignas( 4) u32 algorithm;
        };

        struct PerFrame {
            VkDescriptorSet descriptor_set;
        };

        Context &m_context;
        std::array<PerFrame, MAX_FRAMES_IN_FLIGHT> m_per_frame;

        VkDescriptorSetLayout m_descriptor_set_layout;
        VkPipelineLayout m_pipeline_layout;
        std::array<VkPipeline, 4> m_pipelines;
        usize m_work_group_size;

        static constexpr usize sort_size = 1024;
        StorageBuffer m_sort_buffer;

    public:
        Sorter(Context &context) : m_context(context) {}

        void init();
        void cleanup();
        void dispatch(VkCommandBuffer cmd, Algorithm algorithm, u32 n, u32 h);
        void record(VkCommandBuffer cmd, uint frame_index);
    };
}
