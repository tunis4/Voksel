#include "renderer.hpp"
#include "texture.hpp"
#include "chunk_renderer.hpp"
#include "sky_renderer.hpp"
#include "box_renderer.hpp"
#include "depth_reducer.hpp"
#include "imgui_impl_voksel.hpp"
#include "../game.hpp"

#include <array>
#include <cstring>
#include <set>
#include <imgui/imgui_impl_glfw.h>
#include <entt/entt.hpp>

namespace renderer {
    void Renderer::init() {
        m_fog_color = glm::pow(vec3(166, 209, 211) / 255.0f, vec3(2.2));

        CHECK_VK(volkInitialize());
        create_instance();
#ifdef ENABLE_VK_VALIDATION_LAYERS
        setup_debug_messenger();
#endif
        create_surface();
        pick_physical_device();
        create_logical_device();
        m_context.swapchain.create();
        create_render_pass();
        create_command_pools();
        create_allocator();
        m_context.swapchain.create_depth_resources();
        m_context.swapchain.create_framebuffers();
        create_descriptor_pool();
        create_command_buffers();
        create_sync_objects();
        setup_dear_imgui();

        entt::locator<TextureManager>::emplace(m_context).init();
        entt::locator<DepthReducer>::emplace(m_context).init();
    }

    constexpr std::array requested_validation_layers = std::to_array<const char*>({
        "VK_LAYER_KHRONOS_validation",
    });

    constexpr std::array requested_device_extensions = std::to_array<const char*>({
        "VK_KHR_swapchain",
#ifdef ENABLE_VK_VALIDATION_LAYERS
        "VK_KHR_shader_non_semantic_info",
#endif
    });

#ifdef ENABLE_VK_VALIDATION_LAYERS
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        [[maybe_unused]] void* pUserData
    ) {
        LogLevel log_level = LogLevel::INFO;
        if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            log_level = LogLevel::WARN;
        if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
            log_level = LogLevel::ERROR;

        log(log_level, "Vulkan Validation", "{}", pCallbackData->pMessage);
        return false;
    }

    void populate_debug_create_info(VkDebugUtilsMessengerCreateInfoEXT &info) {
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        info.pfnUserCallback = debug_callback;
    }

    void Renderer::setup_debug_messenger() {
        VkDebugUtilsMessengerCreateInfoEXT create_info {};
        populate_debug_create_info(create_info);

        CHECK_VK(vkCreateDebugUtilsMessengerEXT(m_context.instance, &create_info, nullptr, &m_vk_debug_messenger));
    }

    bool Renderer::check_validation_layer_support() {
        u32 layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        for (const char *layer_name : requested_validation_layers) {
            bool layer_found = false;

            for (auto &layer_properties : available_layers) {
                if (std::strcmp(layer_name, layer_properties.layerName) == 0) {
                    layer_found = true;
                    break;
                }
            }

            if (!layer_found)
                return false;
        }

        return true;
    }
#endif

    void Renderer::create_instance() {
        VkApplicationInfo app_info {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "Voksel";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "No Engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo create_info {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        auto extensions = get_instance_extensions();
        create_info.enabledExtensionCount = extensions.size();
        create_info.ppEnabledExtensionNames = extensions.data();

#ifdef ENABLE_VK_VALIDATION_LAYERS
        VkDebugUtilsMessengerCreateInfoEXT debug_create_info {};
        VkValidationFeaturesEXT validation_features {};
        constexpr std::array enabled_validation_features = std::to_array<VkValidationFeatureEnableEXT>({
            // VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
            VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
            VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
        });
        if (check_validation_layer_support()) {
            create_info.enabledLayerCount = requested_validation_layers.size();
            create_info.ppEnabledLayerNames = requested_validation_layers.data();

            populate_debug_create_info(debug_create_info);
            create_info.pNext = &debug_create_info;

            validation_features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
            validation_features.pEnabledValidationFeatures = enabled_validation_features.data();
            validation_features.enabledValidationFeatureCount = enabled_validation_features.size();
            debug_create_info.pNext = &validation_features;
        } else {
            log(LogLevel::ERROR, "Renderer", "Vulkan validation layers could not be enabled");
        }
#endif

        CHECK_VK(vkCreateInstance(&create_info, nullptr, &m_context.instance));
        volkLoadInstanceOnly(m_context.instance);
    }

    std::vector<const char*> Renderer::get_instance_extensions() {
        u32 glfw_extension_count = 0;
        const char **glfw_extensions;

        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

#ifdef ENABLE_VK_VALIDATION_LAYERS
        extensions.push_back("VK_EXT_debug_utils");
#endif

        return extensions;
    }

    bool Renderer::is_device_suitable(VkPhysicalDevice physical_device) {
        m_context.queue_manager.find_queue_families(physical_device, m_context.surface);

        VkPhysicalDeviceVulkan12Features supported_vulkan12_features {};
        supported_vulkan12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

        VkPhysicalDeviceFeatures2 supported_features {};
        supported_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        supported_features.pNext = &supported_vulkan12_features;
        vkGetPhysicalDeviceFeatures2(physical_device, &supported_features);

        VkFormatProperties format_properties {}; // blitting required for mipmap generation, TODO: make it optional
        vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_R8G8B8A8_SRGB, &format_properties);

        bool features_supported = supported_vulkan12_features.runtimeDescriptorArray &&
            supported_vulkan12_features.descriptorBindingVariableDescriptorCount &&
            supported_vulkan12_features.shaderSampledImageArrayNonUniformIndexing &&
            (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) &&
            supported_features.features.multiDrawIndirect && supported_vulkan12_features.drawIndirectCount &&
            supported_vulkan12_features.samplerFilterMinmax;

        bool extensions_supported = check_device_extension_support(physical_device);
        bool swapchain_adequate = false;
        if (extensions_supported) {
            SwapchainSupportDetails swapchain_support = m_context.swapchain.query_support(physical_device, m_context.surface);
            swapchain_adequate = !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
        }

        return m_context.queue_manager.are_families_complete() && features_supported && extensions_supported && swapchain_adequate;
    }

    void Renderer::pick_physical_device() {
        u32 device_count = 0;
        vkEnumeratePhysicalDevices(m_context.instance, &device_count, nullptr);

        if (device_count == 0)
            throw std::runtime_error("Failed to find GPUs with Vulkan support");

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(m_context.instance, &device_count, devices.data());

        for (auto device : devices) {
            if (is_device_suitable(device)) {
                m_context.physical_device = device;
                break;
            }
        }

        if (m_context.physical_device == VK_NULL_HANDLE)
            throw std::runtime_error("Failed to find a suitable GPU");
    }

    bool Renderer::check_device_extension_support(VkPhysicalDevice device) {
        u32 extension_count;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

        u32 found_extensions = 0;
        for (auto &available_extension : available_extensions) {
            if (found_extensions == requested_device_extensions.size())
                break;

            for (auto requested_extension : requested_device_extensions) {
                if (std::strcmp(available_extension.extensionName, requested_extension) == 0) {
                    found_extensions++;
                    break;
                }
            }
        }

        return found_extensions == requested_device_extensions.size();
    }

    void Renderer::create_logical_device() {
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        std::set<u32> unique_queue_families = { m_context.queue_manager.graphics_family(), m_context.queue_manager.present_family() };

        float queue_priority = 1;
        for (u32 queue_family : unique_queue_families) {
            VkDeviceQueueCreateInfo queue_create_info {};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = queue_family;
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &queue_priority;
            queue_create_infos.push_back(queue_create_info);
        }

        VkDeviceCreateInfo create_info {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = queue_create_infos.size();
        create_info.pQueueCreateInfos = queue_create_infos.data();

        VkPhysicalDeviceFeatures device_features {};
        device_features.multiDrawIndirect = true;
        create_info.pEnabledFeatures = &device_features;

        VkPhysicalDeviceVulkan12Features vulkan12_features {};
        vulkan12_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        vulkan12_features.runtimeDescriptorArray = true;
        vulkan12_features.descriptorBindingVariableDescriptorCount = true;
        vulkan12_features.shaderSampledImageArrayNonUniformIndexing = true;
        vulkan12_features.drawIndirectCount = true;
        vulkan12_features.samplerFilterMinmax = true;
        create_info.pNext = &vulkan12_features;

        create_info.enabledExtensionCount = requested_device_extensions.size();
        create_info.ppEnabledExtensionNames = requested_device_extensions.data();

#ifdef ENABLE_VK_VALIDATION_LAYERS
        if (check_validation_layer_support()) {
            create_info.enabledLayerCount = requested_validation_layers.size();
            create_info.ppEnabledLayerNames = requested_validation_layers.data();
        }
#endif

        CHECK_VK(vkCreateDevice(m_context.physical_device, &create_info, nullptr, &m_context.device));
        volkLoadDevice(m_context.device);
        m_context.queue_manager.create_queues(m_context.device);

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(m_context.physical_device, &properties);
        log(LogLevel::INFO, "Renderer", "Using device: {}", properties.deviceName);

        m_context.max_compute_shared_memory_size = properties.limits.maxComputeSharedMemorySize;
        m_context.max_compute_work_group_invocations = properties.limits.maxComputeWorkGroupInvocations;
        m_context.max_compute_work_group_size.x = properties.limits.maxComputeWorkGroupSize[0];
        m_context.max_compute_work_group_size.y = properties.limits.maxComputeWorkGroupSize[1];
        m_context.max_compute_work_group_size.z = properties.limits.maxComputeWorkGroupSize[2];
        m_context.timestamp_period = properties.limits.timestampPeriod;
    }

    void Renderer::create_surface() {
        if (glfwCreateWindowSurface(m_context.instance, Game::get()->window()->m_glfw_window, nullptr, &m_context.surface) != VK_SUCCESS)
            throw std::runtime_error("Failed to create window surface");
    }

    void Renderer::create_render_pass() {
        VkAttachmentDescription color_attachment {};
        color_attachment.format = m_context.swapchain.m_image_format;
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference color_attachment_ref {};
        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depth_attachment {};
        depth_attachment.format = m_context.swapchain.m_depth_image_format;
        depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // for depth pyramid
        depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth_attachment_ref {};
        depth_attachment_ref.attachment = 1;
        depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment_ref;
        subpass.pDepthStencilAttachment = &depth_attachment_ref;

        VkSubpassDependency dependency {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        // dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        // dependency.srcAccessMask = 0;
        // dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        // dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        std::array attachments = std::to_array({ color_attachment, depth_attachment });

        VkRenderPassCreateInfo render_pass_info {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = attachments.size();
        render_pass_info.pAttachments = attachments.data();
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass;
        render_pass_info.dependencyCount = 1;
        render_pass_info.pDependencies = &dependency;

        CHECK_VK(vkCreateRenderPass(m_context.device, &render_pass_info, nullptr, &m_context.render_pass));
    }

    void Renderer::create_command_pools() {
        VkCommandPoolCreateInfo pool_info {};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_info.queueFamilyIndex = m_context.queue_manager.graphics_family();

        CHECK_VK(vkCreateCommandPool(m_context.device, &pool_info, nullptr, &m_context.command_pool));

        VkCommandPoolCreateInfo transient_pool_info {};
        transient_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        transient_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        transient_pool_info.queueFamilyIndex = m_context.queue_manager.graphics_family();

        CHECK_VK(vkCreateCommandPool(m_context.device, &transient_pool_info, nullptr, &m_context.transient_command_pool));
    }

    void Renderer::create_command_buffers() {
        VkCommandBufferAllocateInfo alloc_info {};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = m_context.command_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = 1;

        for (auto &frame : m_per_frame)
            CHECK_VK(vkAllocateCommandBuffers(m_context.device, &alloc_info, &frame.m_command_buffer));
    }

    void Renderer::create_sync_objects() {
        VkSemaphoreCreateInfo semaphore_info {};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fence_info {};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (auto &frame : m_per_frame) {
            CHECK_VK(vkCreateSemaphore(m_context.device, &semaphore_info, nullptr, &frame.m_image_available_semaphore));
            CHECK_VK(vkCreateSemaphore(m_context.device, &semaphore_info, nullptr, &frame.m_render_finished_semaphore));
            CHECK_VK(vkCreateFence(m_context.device, &fence_info, nullptr, &frame.m_in_flight_fence));
        }
    }

    void Renderer::setup_dear_imgui() {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.Fonts->AddFontFromFileTTF("res/thirdparty/Roboto-Regular.ttf", 18.0f);

        ImGuiStyle &style = ImGui::GetStyle();
        ImGui::StyleColorsDark(&style);
        style.FrameRounding = 8.0f;
        style.WindowRounding = 4.0f;

        ImGui_ImplGlfw_InitForVulkan(Game::get()->window()->m_glfw_window, true);
        ImGui_ImplVoksel_InitInfo init_info {};
        init_info.Context = &m_context;
        init_info.DescriptorPool = m_context.descriptor_pool;
        init_info.Subpass = 0;
        init_info.MinImageCount = 2;
        init_info.ImageCount = 2;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = [] (VkResult err) {
            if (err == 0)
                return;
            log(LogLevel::ERROR, "Dear ImGui", "Error: VkResult = {}", err);
            if (err < 0)
                abort();
        };
        ImGui_ImplVoksel_Init(&init_info, m_context.render_pass);

        // now we need to upload the fonts
        ImGui_ImplVoksel_CreateFontsTexture();

        ImGui_ImplVoksel_DestroyFontUploadObjects();
    }

    void Renderer::create_allocator() {
        VmaVulkanFunctions vulkan_functions {};
        vulkan_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vulkan_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocator_create_info {};
        allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_2;
        allocator_create_info.physicalDevice = m_context.physical_device;
        allocator_create_info.device = m_context.device;
        allocator_create_info.instance = m_context.instance;
        allocator_create_info.pVulkanFunctions = &vulkan_functions;
        CHECK_VK(vmaCreateAllocator(&allocator_create_info, &m_context.allocator));
    }

    void Renderer::create_descriptor_pool() {
        std::array pool_sizes = std::to_array<VkDescriptorPoolSize>({
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 128 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 128 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 16 }
        });

        VkDescriptorPoolCreateInfo pool_info {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = pool_sizes.size();
        pool_info.pPoolSizes = pool_sizes.data();
        pool_info.maxSets = 256; // gotta be careful about this

        CHECK_VK(vkCreateDescriptorPool(m_context.device, &pool_info, nullptr, &m_context.descriptor_pool));
    }

    void Renderer::record_command_buffer(VkCommandBuffer command_buffer, uint image_index) {
        bool in_game = Game::get()->is_in_game();

        VkCommandBufferBeginInfo begin_info {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        CHECK_VK(vkBeginCommandBuffer(command_buffer, &begin_info));

        if (in_game)
            entt::locator<ChunkRenderer>::value().record_compute(command_buffer, m_frame_index);

        VkRenderPassBeginInfo render_pass_info {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = m_context.render_pass;
        render_pass_info.framebuffer = m_context.swapchain.m_framebuffers[image_index];
        render_pass_info.renderArea.offset = { 0, 0 };
        render_pass_info.renderArea.extent = m_context.swapchain.m_extent;

        std::array clear_values = std::to_array<VkClearValue>({
            { .color = { { m_fog_color.r, m_fog_color.g, m_fog_color.b, 1.0f } } },
            { .depthStencil = { 0.0f, 0 } }
        });

        render_pass_info.clearValueCount = clear_values.size();
        render_pass_info.pClearValues = clear_values.data();

        vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
            VkViewport viewport {};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (f32)m_context.swapchain.m_extent.width;
            viewport.height = (f32)m_context.swapchain.m_extent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(command_buffer, 0, 1, &viewport);

            VkRect2D scissor {};
            scissor.offset = {0, 0};
            scissor.extent = m_context.swapchain.m_extent;
            vkCmdSetScissor(command_buffer, 0, 1, &scissor);

            if (in_game) {
                entt::locator<SkyRenderer>::value().record(command_buffer, m_frame_index);
                entt::locator<ChunkRenderer>::value().record_solid_pass(command_buffer, m_frame_index);
                entt::locator<ChunkRenderer>::value().record_transparent_pass(command_buffer, m_frame_index);
                entt::locator<BoxRenderer>::value().record(command_buffer, m_frame_index);
            }

            ImGui::End(); // end the voksel window
            ImGui::Render();
            ImGui_ImplVoksel_RenderDrawData(ImGui::GetDrawData(), command_buffer);
        vkCmdEndRenderPass(command_buffer);

        if (in_game) {
            entt::locator<DepthReducer>::value().record(command_buffer, m_frame_index);
        }

        CHECK_VK(vkEndCommandBuffer(command_buffer));
    }

    void Renderer::render([[maybe_unused]] f64 delta_time) {
        PerFrame &frame = m_per_frame[m_frame_index];

        // if (!m_context.buffer_updates.empty()) {
        //     std::array<VkFence, MAX_FRAMES_IN_FLIGHT> in_flight_fences;
        //     for (usize i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        //         in_flight_fences[i] = m_per_frame[i].m_in_flight_fence;
        //     vkWaitForFences(m_context.device, in_flight_fences.size(), in_flight_fences.data(), true, UINT64_MAX);
        //     do {
        //         auto &update = m_context.buffer_updates.front();
        //         update.m_buffer.upload_data(m_context, update.m_data, update.m_size, update.m_offset);
        //         delete[] update.m_data;
        //         m_context.buffer_updates.pop();
        //     } while (!m_context.buffer_updates.empty());
        // } else 
        vkWaitForFences(m_context.device, 1, &frame.m_in_flight_fence, true, UINT64_MAX);

        u32 image_index;
        VkResult result = vkAcquireNextImageKHR(m_context.device, m_context.swapchain.m_swapchain, UINT64_MAX, frame.m_image_available_semaphore, VK_NULL_HANDLE, &image_index);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            m_context.swapchain.recreate();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw std::runtime_error("Failed to acquire swap chain image");

        vkResetFences(m_context.device, 1, &frame.m_in_flight_fence);

        vkResetCommandBuffer(frame.m_command_buffer, 0);
        record_command_buffer(frame.m_command_buffer, image_index);

        VkSubmitInfo submit_info {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &frame.m_image_available_semaphore;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &frame.m_render_finished_semaphore;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &frame.m_command_buffer;

        VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submit_info.pWaitDstStageMask = wait_stages;

        CHECK_VK(vkQueueSubmit(m_context.queue_manager.m_graphics_queue, 1, &submit_info, frame.m_in_flight_fence));

        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &frame.m_render_finished_semaphore;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &m_context.swapchain.m_swapchain;
        present_info.pImageIndices = &image_index;

        result = vkQueuePresentKHR(m_context.queue_manager.m_present_queue, &present_info);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebuffer_resized) {
            m_framebuffer_resized = false;
            m_context.swapchain.recreate();
        } else if (result != VK_SUCCESS)
            throw std::runtime_error("Failed to present swap chain image");

        m_frame_index = (m_frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void Renderer::begin_cleanup() {
        vkDeviceWaitIdle(m_context.device);

        ImGui_ImplVoksel_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        // if (!m_context.buffer_deletions.empty()) {
        //     do {
        //         auto &deletion = m_context.buffer_deletions.front();
        //         vmaDestroyBuffer(m_context.allocator, deletion.m_buffer, deletion.m_allocation);
        //         m_context.buffer_deletions.pop();
        //     } while (!m_context.buffer_deletions.empty());
        // }
    }

    void Renderer::cleanup() {
        auto device = m_context.device;

        entt::locator<DepthReducer>::value().cleanup();
        entt::locator<TextureManager>::value().cleanup();

        for (auto &frame : m_per_frame) {
            vkDestroySemaphore(device, frame.m_image_available_semaphore, nullptr);
            vkDestroySemaphore(device, frame.m_render_finished_semaphore, nullptr);
            vkDestroyFence(device, frame.m_in_flight_fence, nullptr);
        }

        m_context.swapchain.cleanup();
        vmaDestroyAllocator(m_context.allocator);
        vkDestroyCommandPool(device, m_context.command_pool, nullptr);
        vkDestroyCommandPool(device, m_context.transient_command_pool, nullptr);
        vkDestroyRenderPass(device, m_context.render_pass, nullptr);
        vkDestroyDescriptorPool(device, m_context.descriptor_pool, nullptr);
        vkDestroyDevice(device, nullptr);
#ifdef ENABLE_VK_VALIDATION_LAYERS
        vkDestroyDebugUtilsMessengerEXT(m_context.instance, m_vk_debug_messenger, nullptr);
#endif
        vkDestroySurfaceKHR(m_context.instance, m_context.surface, nullptr);
        vkDestroyInstance(m_context.instance, nullptr);
    }

    void Renderer::on_framebuffer_resize([[maybe_unused]] uint width, [[maybe_unused]] uint height) {
        m_framebuffer_resized = true;
    }

    void Renderer::on_cursor_move([[maybe_unused]] f64 x, [[maybe_unused]] f64 y) {}
}
