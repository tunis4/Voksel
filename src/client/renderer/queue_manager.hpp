#pragma once

#include <optional>
#include <vulkan/vulkan.h>

#include "../../util/util.hpp"

namespace render {
    class QueueManager {
        std::optional<u32> m_graphics_family;
        std::optional<u32> m_present_family;

    public:
        VkQueue m_graphics_queue;
        VkQueue m_present_queue;
    
        bool are_families_complete();
        void find_queue_families(VkPhysicalDevice physical_device, VkSurfaceKHR surface);
        void create_queues(VkDevice device);

        inline u32 graphics_family() { return m_graphics_family.value(); }
        inline u32 present_family() { return m_present_family.value(); }
    };
};
