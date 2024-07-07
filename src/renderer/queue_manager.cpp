#include "queue_manager.hpp"

#include <vector>

namespace renderer {
    bool QueueManager::are_families_complete() {
        return m_graphics_family.has_value() && m_present_family.has_value();
    }

    void QueueManager::find_queue_families(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
        u32 queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

        for (u32 i = 0; i < queue_family_count; i++) {
            auto &queue_family = queue_families[i];

            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                m_graphics_family = i;

            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);

            if (present_support)
                m_present_family = i;

            if (are_families_complete())
                break;
        }
    }

    void QueueManager::create_queues(VkDevice device) {
        vkGetDeviceQueue(device, graphics_family(), 0, &m_graphics_queue);
        vkGetDeviceQueue(device, present_family(), 0, &m_present_queue);
    }
}
