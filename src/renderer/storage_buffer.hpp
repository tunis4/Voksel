#pragma once

#include "context.hpp"

namespace renderer {
    class StorageBuffer {
        void *m_staging_mapped;
        VkPipelineStageFlags m_stage_flags;

    public:
        enum Mode {
            READ = 1 << 0,
            WRITE = 1 << 1,
            READWRITE = READ | WRITE,
        };

        VkBuffer m_buffer;
        VmaAllocation m_allocation;

        VkBuffer m_staging_buffer;
        VmaAllocation m_staging_allocation;

        usize m_size;

        // buffer usage will also be marked as transfer destination
        void create(Context &context, usize size, usize staging_size, VkBufferUsageFlags usage, VkPipelineStageFlags stage_flags, Mode mode = WRITE);
        void destroy(Context &context);

        void upload(Context &context, void *data, usize size, usize offset);
        void fill(Context &context, u32 data, usize size, usize offset);
    };
}
