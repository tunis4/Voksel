#pragma once

#include "context.hpp"

namespace render {
    class PersistentBuffer {
        void *m_mapped;
        void *m_staging_mapped;
    
    public:
        VkBuffer m_buffer;
        VmaAllocation m_allocation;

        VkBuffer m_staging_buffer;
        VmaAllocation m_staging_allocation;

        // buffer usage will also be marked as transfer destination
        void create(Context &context, usize size, usize max_upload_size, VkBufferUsageFlags usage, bool dedicated = false);
        void destroy(Context &context);
        void upload_data(Context &context, void *data, usize size, usize offset);
    };
}
