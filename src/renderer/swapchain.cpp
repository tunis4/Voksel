#include "swapchain.hpp"
#include "context.hpp"
#include "depth_reducer.hpp"
#include "chunk_renderer.hpp"
#include "../game.hpp"

#include <algorithm>
#include <entt/entt.hpp>

namespace renderer {
    SwapchainSupportDetails Swapchain::query_support(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
        SwapchainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &details.capabilities);

        u32 format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);

        if (format_count != 0) {
            details.formats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, details.formats.data());
        }

        u32 present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr);

        if (present_mode_count != 0) {
            details.present_modes.resize(present_mode_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, details.present_modes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR Swapchain::choose_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats) {
        for (const auto &available_format : available_formats)
            if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return available_format;

        return available_formats[0];
    }

    VkPresentModeKHR Swapchain::choose_present_mode(const std::vector<VkPresentModeKHR> &available_present_modes) {
        // for (const auto &available_present_mode : available_present_modes)
        //     if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
        //         return available_present_mode;

        return VK_PRESENT_MODE_FIFO_KHR; // guaranteed to be supported
    }

    VkExtent2D Swapchain::choose_extent(const VkSurfaceCapabilitiesKHR &capabilities, u32 width, u32 height) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            return capabilities.currentExtent;

        VkExtent2D actual_extent = { width, height };
        actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actual_extent;
    }

    void Swapchain::create() {
        auto window = Game::get()->window();
        SwapchainSupportDetails swapchain_support = query_support(m_context.physical_device, m_context.surface);

        VkSurfaceFormatKHR surface_format = choose_surface_format(swapchain_support.formats);
        VkPresentModeKHR present_mode = choose_present_mode(swapchain_support.present_modes);
        VkExtent2D extent = choose_extent(swapchain_support.capabilities, window->m_width, window->m_height);

        u32 image_count = swapchain_support.capabilities.minImageCount + 1;
        if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount)
            image_count = swapchain_support.capabilities.maxImageCount;

        VkSwapchainCreateInfoKHR create_info {};
        create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface = m_context.surface;

        create_info.minImageCount = image_count;
        create_info.imageFormat = surface_format.format;
        create_info.imageColorSpace = surface_format.colorSpace;
        create_info.imageExtent = extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        u32 graphics_family = m_context.queue_manager.graphics_family();
        u32 present_family = m_context.queue_manager.present_family();
        u32 queue_family_indices[] = { graphics_family, present_family };
        if (graphics_family != present_family) {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices = queue_family_indices;
        } else create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

        create_info.preTransform = swapchain_support.capabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode = present_mode;
        create_info.clipped = true;

        create_info.oldSwapchain = VK_NULL_HANDLE;

        CHECK_VK(vkCreateSwapchainKHR(m_context.device, &create_info, nullptr, &m_swapchain));

        vkGetSwapchainImagesKHR(m_context.device, m_swapchain, &image_count, nullptr);
        m_images.resize(image_count);
        vkGetSwapchainImagesKHR(m_context.device, m_swapchain, &image_count, m_images.data());

        m_image_format = surface_format.format;
        m_extent = extent;

        create_image_views();

        m_depth_image_format = VK_FORMAT_D32_SFLOAT;
    }

    void Swapchain::create_image_views() {
        m_image_views.resize(m_images.size());
        for (usize i = 0; i < m_images.size(); i++) {
            VkImageViewCreateInfo create_info {};
            create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            create_info.image = m_images[i];
            create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            create_info.format = m_image_format;

            create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            create_info.subresourceRange.baseMipLevel = 0;
            create_info.subresourceRange.levelCount = 1;
            create_info.subresourceRange.baseArrayLayer = 0;
            create_info.subresourceRange.layerCount = 1;

            CHECK_VK(vkCreateImageView(m_context.device, &create_info, nullptr, &m_image_views[i]));
        }
    }

    void Swapchain::create_depth_resources() {
        m_context.create_image(&m_depth_image, &m_depth_image_allocation, nullptr, m_extent.width, m_extent.height, 1, m_depth_image_format, 
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);

        VkImageViewCreateInfo view_info {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = m_depth_image;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = m_depth_image_format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        CHECK_VK(vkCreateImageView(m_context.device, &view_info, nullptr, &m_depth_image_view));

        m_depth_pyramid_extent = { // previous power of 2
            u32(1 << (std::bit_width(m_extent.width) - 1)), u32(1 << (std::bit_width(m_extent.height) - 1))
        };
        m_depth_pyramid_mip_levels = std::bit_width(std::max(m_depth_pyramid_extent.width, m_depth_pyramid_extent.height));

        m_context.create_image(&m_depth_pyramid, &m_depth_pyramid_allocation, nullptr, m_depth_pyramid_extent.width, m_depth_pyramid_extent.height,
            m_depth_pyramid_mip_levels, VK_FORMAT_R32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, 0);

        for (u32 i = 0; i < m_depth_pyramid_mip_levels; i++)
            m_context.transition_image_layout(m_depth_pyramid, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, i);

        view_info.image = m_depth_pyramid;
        view_info.format = VK_FORMAT_R32_SFLOAT;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        for (uint i = 0; i < m_depth_pyramid_mip_levels; i++) {
            view_info.subresourceRange.baseMipLevel = i;
            VkImageView view;
            CHECK_VK(vkCreateImageView(m_context.device, &view_info, nullptr, &view));
            m_depth_pyramid_views.push_back(view);
        }

        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = m_depth_pyramid_mip_levels;
        CHECK_VK(vkCreateImageView(m_context.device, &view_info, nullptr, &m_depth_pyramid_full_view));

        VkSamplerCreateInfo sampler_info {};
        sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter = VK_FILTER_LINEAR;
        sampler_info.minFilter = VK_FILTER_LINEAR;
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sampler_info.minLod = 0.0f;
        sampler_info.maxLod = m_depth_pyramid_mip_levels;

        VkSamplerReductionModeCreateInfo reduction_mode_info {};
        reduction_mode_info.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
        reduction_mode_info.reductionMode = VK_SAMPLER_REDUCTION_MODE_MIN;

        sampler_info.pNext = &reduction_mode_info;
        CHECK_VK(vkCreateSampler(m_context.device, &sampler_info, nullptr, &m_depth_reduction_sampler));

        sampler_info.pNext = nullptr;
        sampler_info.magFilter = VK_FILTER_NEAREST;
        sampler_info.minFilter = VK_FILTER_NEAREST;
        CHECK_VK(vkCreateSampler(m_context.device, &sampler_info, nullptr, &m_depth_pyramid_sampler));
    }

    void Swapchain::create_framebuffers() {
        m_framebuffers.resize(m_image_views.size());
        for (usize i = 0; i < m_image_views.size(); i++) {
            std::array attachments = std::to_array({ m_image_views[i], m_depth_image_view });

            VkFramebufferCreateInfo framebuffer_info {};
            framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebuffer_info.renderPass = m_context.render_pass;
            framebuffer_info.attachmentCount = attachments.size();
            framebuffer_info.pAttachments = attachments.data();
            framebuffer_info.width = m_extent.width;
            framebuffer_info.height = m_extent.height;
            framebuffer_info.layers = 1;

            CHECK_VK(vkCreateFramebuffer(m_context.device, &framebuffer_info, nullptr, &m_framebuffers[i]));
        }
    }

    void Swapchain::cleanup() {
        vkDestroySampler(m_context.device, m_depth_reduction_sampler, nullptr);
        vkDestroySampler(m_context.device, m_depth_pyramid_sampler, nullptr);
        for (auto image_view : m_depth_pyramid_views)
            vkDestroyImageView(m_context.device, image_view, nullptr);
        m_depth_pyramid_views.clear();
        vkDestroyImageView(m_context.device, m_depth_pyramid_full_view, nullptr);
        vmaDestroyImage(m_context.allocator, m_depth_pyramid, m_depth_pyramid_allocation);

        vkDestroyImageView(m_context.device, m_depth_image_view, nullptr);
        vmaDestroyImage(m_context.allocator, m_depth_image, m_depth_image_allocation);

        for (auto framebuffer : m_framebuffers)
            vkDestroyFramebuffer(m_context.device, framebuffer, nullptr);

        for (auto image_view : m_image_views)
            vkDestroyImageView(m_context.device, image_view, nullptr);

        vkDestroySwapchainKHR(m_context.device, m_swapchain, nullptr);
    }

    void Swapchain::recreate() {
        vkDeviceWaitIdle(m_context.device);
        cleanup();
        create();
        create_depth_resources();
        create_framebuffers();
        entt::locator<DepthReducer>::value().free_descriptor_sets();
        entt::locator<DepthReducer>::value().create_descriptor_sets();
        if (entt::locator<ChunkRenderer>::has_value())
            entt::locator<ChunkRenderer>::value().refresh_depth_pyramid_binding();
    }
}
