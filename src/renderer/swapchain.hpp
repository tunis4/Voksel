#pragma once

#include <vector>

#include "common.hpp"

namespace renderer {
    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    struct Context;
    class Swapchain {
        Context &m_context;

    public:
        VkSwapchainKHR m_swapchain;
        std::vector<VkImage> m_images;
        std::vector<VkImageView> m_image_views;
        std::vector<VkFramebuffer> m_framebuffers;
        VkFormat m_image_format;
        VkExtent2D m_extent;

        VkFormat m_depth_image_format;
        VkImage m_depth_image;
        VmaAllocation m_depth_image_allocation;
        VkImageView m_depth_image_view;

        VkExtent2D m_depth_pyramid_extent;
        uint m_depth_pyramid_mip_levels;
        VkImage m_depth_pyramid;
        VmaAllocation m_depth_pyramid_allocation;
        std::vector<VkImageView> m_depth_pyramid_views;
        VkImageView m_depth_pyramid_full_view;
        VkSampler m_depth_reduction_sampler; // used for reducing both the depth image and the depth pyramid
        VkSampler m_depth_pyramid_sampler;

        Swapchain(Context &context) : m_context(context) {}

        static SwapchainSupportDetails query_support(VkPhysicalDevice device, VkSurfaceKHR surface);
        static VkSurfaceFormatKHR choose_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats);
        static VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR> &available_present_modes);
        static VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR &capabilities, u32 width, u32 height);
        void create();
        void create_image_views();
        void create_depth_resources();
        void create_framebuffers();
        void cleanup();
        void recreate();
    };
};
