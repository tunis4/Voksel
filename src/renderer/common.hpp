#pragma once

#include "../util.hpp"
#include <volk/volk.h>

#define VMA_VULKAN_VERSION 1002000 // Vulkan 1.2
#include <vma/vk_mem_alloc.h>

#define MAX_FRAMES_IN_FLIGHT 2

#define CHECK_VK(expression)                                                                                      \
    ({                                                                                                            \
        auto&& _temporary_result = (expression);                                                                  \
        if (_temporary_result != VK_SUCCESS) [[unlikely]]                                                         \
            log(LogLevel::ERROR, "Renderer", "Vulkan call " #expression " failed at {}:{} with error code: {}", \
                __FILE__, __LINE__, _temporary_result);                                                           \
    })

namespace renderer {
    template<class V>
    concept VertexFormat = requires {
        { V::alloc_binding_description() } -> std::same_as<VkVertexInputBindingDescription*>;
        V::num_attribute_descriptions;
        { V::alloc_attribute_descriptions() } -> std::same_as<VkVertexInputAttributeDescription*>;
    };
}
