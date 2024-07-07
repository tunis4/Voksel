#include "sorter.hpp"
#include "pipeline.hpp"
#include "descriptor.hpp"

#include <chrono>

namespace renderer {
    void Sorter::init() {
        // m_sort_buffer.create(m_context, sort_size * sizeof(ChunkTransparentIndirect), sort_size * sizeof(ChunkTransparentIndirect), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        // auto db = DescriptorBuilder::begin(m_context.device, m_context.descriptor_pool);
        // m_descriptor_set_layout = db
        //     .bind_single(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
        //     .create_layout();

        // for (auto &frame : m_per_frame) {
        //     db.write_buffer(m_sort_buffer.m_buffer, 0, sort_size * sizeof(ChunkTransparentIndirect));
        //     frame.descriptor_set = db.create_set();
        // }

        // m_work_group_size = std::min(
        //     std::min(m_context.max_compute_work_group_invocations, m_context.max_compute_work_group_size.x), 
        //     u32(m_context.max_compute_shared_memory_size / sizeof(ChunkTransparentIndirect) / 2));

        // log(LogLevel::INFO, "Sorter", "Work group size: {}", m_work_group_size);

        // for (uint i = 0; i < m_pipelines.size(); i++) {
        //     m_pipelines[i] = PipelineBuilder::begin(m_context.device)
        //         .comp_shader_specialization(1, m_work_group_size).comp_shader("sort")
        //         .layout<PushConstants>(&m_pipeline_layout, 1, &m_descriptor_set_layout)
        //         .finish_compute();
        // }

        // VkQueryPoolCreateInfo query_pool_create_info {};
        // query_pool_create_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        // query_pool_create_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
        // query_pool_create_info.queryCount = 2;

        // VkQueryPool timestamp_query_pool;
        // vkCreateQueryPool(m_context.device, &query_pool_create_info, nullptr, &timestamp_query_pool);

        // std::vector<ChunkTransparentIndirect> test_vector;
        // test_vector.reserve(sort_size);
        // for (u32 i = 0; i < sort_size; i++)
        //     test_vector.emplace_back(1024, 1, 0, 0, i);

        // std::random_device rd;
        // std::mt19937 g(rd());
        // std::shuffle(test_vector.begin(), test_vector.end(), g);

        // float total_time = 0.0f;
        // for (uint i = 0; i < 1024; i++) {
        //     m_sort_buffer.upload(m_context, test_vector.data(), sort_size * sizeof(ChunkTransparentIndirect), 0);

        //     PerFrame &frame = m_per_frame[0];
        //     VkCommandBuffer cmd = m_context.begin_single_time_commands();
        //     vkCmdResetQueryPool(cmd, timestamp_query_pool, 0, 2);
        //     vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, timestamp_query_pool, 0);
        //     vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline_layout, 0, 1, &frame.descriptor_set, 0, nullptr);

        //     u32 h = m_work_group_size * 2;


        //     dispatch(cmd, LOCAL_BMS, sort_size, h);
        //     h *= 2;

        //     for (; h <= sort_size; h *= 2) {
        //         dispatch(cmd, BIG_FLIP, sort_size, h);
        //         for (u32 hh = h / 2; hh > 1; hh /= 2) {
        //             if (hh <= m_work_group_size * 2) {
        //                 dispatch(cmd, LOCAL_DISPERSE, sort_size, hh);
        //                 break;
        //             } else {
        //                 dispatch(cmd, BIG_DISPERSE, sort_size, hh);
        //             }
        //         }
        //     }

        //     vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, timestamp_query_pool, 1);
        //     m_context.end_single_time_commands(cmd);

        //     u64 results[2] = {};
        //     vkGetQueryPoolResults(m_context.device, timestamp_query_pool, 0, 2, 2 * sizeof(u64), results, sizeof(u64), VK_QUERY_RESULT_64_BIT);

        //     f32 time = (results[1] - results[0]) * (m_context.timestamp_period / 1000000.0f);
        // log(LogLevel::INFO, "Sorter", "GPU took {} ms to sort", time);
        //     total_time += time;
        // }
        // log(LogLevel::INFO, "Sorter", "GPU took {} ms average to sort", total_time / 1024);

        // auto begin = std::chrono::steady_clock::now();
        // std::sort(test_vector.begin(), test_vector.end(), [](ChunkTransparentIndirect &a, ChunkTransparentIndirect &b) {
        //     return a.distance2 > b.distance2;
        // });
        // auto end = std::chrono::steady_clock::now();
        // log(LogLevel::INFO, "Sorter", "CPU took {} ms to sort", std::chrono::duration_cast<std::chrono::duration<f32>>(end - begin).count() * 1000.0f);

        // vkDestroyQueryPool(m_context.device, timestamp_query_pool, nullptr);
    }

    void Sorter::cleanup() {
        // for (VkPipeline pipeline : m_pipelines)
        //     vkDestroyPipeline(m_context.device, pipeline, nullptr);
        // vkDestroyPipelineLayout(m_context.device, m_pipeline_layout, nullptr);
        // vkDestroyDescriptorSetLayout(m_context.device, m_descriptor_set_layout, nullptr);
    }

    void Sorter::dispatch(VkCommandBuffer cmd, Algorithm algorithm, u32 n, u32 h) {
        // vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines[algorithm]);

        // PushConstants push_constants;
        // push_constants.h = h;
        // push_constants.algorithm = algorithm;
        // vkCmdPushConstants(cmd, m_pipeline_layout, PushConstants::stage_flags, 0, sizeof(PushConstants), &push_constants);
        // vkCmdDispatch(cmd, n / (m_work_group_size * 2), 1, 1);

        // VkBufferMemoryBarrier barrier {};
        // barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        // barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        // barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        // barrier.buffer = m_sort_buffer.m_buffer;
        // barrier.offset = 0;
        // barrier.size = VK_WHOLE_SIZE;
        // barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        // barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        // vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
    }

    void Sorter::record(VkCommandBuffer cmd, uint frame_index) {
    }
}
