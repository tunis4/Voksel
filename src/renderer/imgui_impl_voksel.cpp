// voksel engine backend for dear imgui
// TODO: maybe clean this up one day
// This needs to be used along with a Platform Backend (e.g. GLFW, SDL, Win32, custom..)

#include "imgui_impl_voksel.hpp"
#include "pipeline.hpp"
using namespace renderer;

#include <cstdio>

// Reusable buffers used for rendering 1 current in-flight frame, for ImGui_ImplVoksel_RenderDrawData()
// [Please zero-clear before use!]
struct ImGui_ImplVokselH_FrameRenderBuffers
{
    VkDeviceMemory      VertexBufferMemory;
    VkDeviceMemory      IndexBufferMemory;
    VkDeviceSize        VertexBufferSize;
    VkDeviceSize        IndexBufferSize;
    VkBuffer            VertexBuffer;
    VkBuffer            IndexBuffer;
};

// Each viewport will hold 1 ImGui_ImplVokselH_WindowRenderBuffers
// [Please zero-clear before use!]
struct ImGui_ImplVokselH_WindowRenderBuffers
{
    uint32_t            Index;
    uint32_t            Count;
    ImGui_ImplVokselH_FrameRenderBuffers*   FrameRenderBuffers;
};

// For multi-viewport support:
// Helper structure we store in the void* RendererUserData field of each ImGuiViewport to easily retrieve our backend data.
struct ImGui_ImplVoksel_ViewportData
{
    bool                                    WindowOwned;
    ImGui_ImplVokselH_WindowRenderBuffers   RenderBuffers;      // Used by all viewports

    ImGui_ImplVoksel_ViewportData()         { WindowOwned = false; memset(&RenderBuffers, 0, sizeof(RenderBuffers)); }
    ~ImGui_ImplVoksel_ViewportData()        { }
};

// Voksel data
struct ImGui_ImplVoksel_Data
{
    ImGui_ImplVoksel_InitInfo   VulkanInitInfo;
    VkRenderPass                RenderPass;
    VkDeviceSize                BufferMemoryAlignment;
    VkPipelineCreateFlags       PipelineCreateFlags;
    VkDescriptorSetLayout       DescriptorSetLayout;
    VkPipelineLayout            PipelineLayout;
    VkPipeline                  Pipeline;
    uint32_t                    Subpass;
    VkShaderModule              ShaderModuleVert;
    VkShaderModule              ShaderModuleFrag;

    // Font data
    VkSampler                   FontSampler;
    VkImage                     FontImage;
    VmaAllocation               FontAllocation;
    VkImageView                 FontView;
    VkDescriptorSet             FontDescriptorSet;
    VkBuffer                    UploadBuffer;
    VmaAllocation               UploadBufferAllocation;

    ImGui_ImplVoksel_Data()
    {
        memset((void*)this, 0, sizeof(*this));
        BufferMemoryAlignment = 256;
    }
};

// Forward Declarations
bool ImGui_ImplVoksel_CreateDeviceObjects();
void ImGui_ImplVoksel_DestroyDeviceObjects();

//-----------------------------------------------------------------------------
// FUNCTIONS
//-----------------------------------------------------------------------------

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
// FIXME: multi-context support is not tested and probably dysfunctional in this backend.
static ImGui_ImplVoksel_Data* ImGui_ImplVoksel_GetBackendData()
{
    return ImGui::GetCurrentContext() ? (ImGui_ImplVoksel_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

static uint32_t ImGui_ImplVoksel_MemoryType(VkMemoryPropertyFlags properties, uint32_t type_bits)
{
    ImGui_ImplVoksel_Data* bd = ImGui_ImplVoksel_GetBackendData();
    ImGui_ImplVoksel_InitInfo* v = &bd->VulkanInitInfo;
    VkPhysicalDeviceMemoryProperties prop;
    vkGetPhysicalDeviceMemoryProperties(v->Context->physical_device, &prop);
    for (uint32_t i = 0; i < prop.memoryTypeCount; i++)
        if ((prop.memoryTypes[i].propertyFlags & properties) == properties && type_bits & (1 << i))
            return i;
    return 0xFFFFFFFF; // Unable to find memoryType
}

static void check_vk_result(VkResult err)
{
    ImGui_ImplVoksel_Data* bd = ImGui_ImplVoksel_GetBackendData();
    if (!bd)
        return;
    ImGui_ImplVoksel_InitInfo* v = &bd->VulkanInitInfo;
    if (v->CheckVkResultFn)
        v->CheckVkResultFn(err);
}

static void CreateOrResizeBuffer(VkBuffer& buffer, VkDeviceMemory& buffer_memory, VkDeviceSize& p_buffer_size, size_t new_size, VkBufferUsageFlagBits usage)
{
    ImGui_ImplVoksel_Data* bd = ImGui_ImplVoksel_GetBackendData();
    ImGui_ImplVoksel_InitInfo* v = &bd->VulkanInitInfo;
    VkResult err;
    if (buffer != VK_NULL_HANDLE)
        vkDestroyBuffer(v->Context->device, buffer, v->Allocator);
    if (buffer_memory != VK_NULL_HANDLE)
        vkFreeMemory(v->Context->device, buffer_memory, v->Allocator);

    VkDeviceSize vertex_buffer_size_aligned = ((new_size - 1) / bd->BufferMemoryAlignment + 1) * bd->BufferMemoryAlignment;
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = vertex_buffer_size_aligned;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    err = vkCreateBuffer(v->Context->device, &buffer_info, v->Allocator, &buffer);
    check_vk_result(err);

    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(v->Context->device, buffer, &req);
    bd->BufferMemoryAlignment = (bd->BufferMemoryAlignment > req.alignment) ? bd->BufferMemoryAlignment : req.alignment;
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = req.size;
    alloc_info.memoryTypeIndex = ImGui_ImplVoksel_MemoryType(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, req.memoryTypeBits);
    err = vkAllocateMemory(v->Context->device, &alloc_info, v->Allocator, &buffer_memory);
    check_vk_result(err);

    err = vkBindBufferMemory(v->Context->device, buffer, buffer_memory, 0);
    check_vk_result(err);
    p_buffer_size = req.size;
}

static void ImGui_ImplVoksel_SetupRenderState(ImDrawData* draw_data, VkPipeline pipeline, VkCommandBuffer command_buffer, ImGui_ImplVokselH_FrameRenderBuffers* rb, int fb_width, int fb_height)
{
    ImGui_ImplVoksel_Data* bd = ImGui_ImplVoksel_GetBackendData();

    // Bind pipeline:
    {
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    }

    // Bind Vertex And Index Buffer:
    if (draw_data->TotalVtxCount > 0)
    {
        VkBuffer vertex_buffers[1] = { rb->VertexBuffer };
        VkDeviceSize vertex_offset[1] = { 0 };
        vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, vertex_offset);
        vkCmdBindIndexBuffer(command_buffer, rb->IndexBuffer, 0, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
    }

    // Setup viewport:
    {
        VkViewport viewport;
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = (float)fb_width;
        viewport.height = (float)fb_height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    }

    // Setup scale and translation:
    // Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    {
        float scale[2];
        scale[0] = 2.0f / draw_data->DisplaySize.x;
        scale[1] = 2.0f / draw_data->DisplaySize.y;
        float translate[2];
        translate[0] = -1.0f - draw_data->DisplayPos.x * scale[0];
        translate[1] = -1.0f - draw_data->DisplayPos.y * scale[1];
        vkCmdPushConstants(command_buffer, bd->PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 0, sizeof(float) * 2, scale);
        vkCmdPushConstants(command_buffer, bd->PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, translate);
    }
}

// Render function
void ImGui_ImplVoksel_RenderDrawData(ImDrawData* draw_data, VkCommandBuffer command_buffer, VkPipeline pipeline)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    ImGui_ImplVoksel_Data* bd = ImGui_ImplVoksel_GetBackendData();
    ImGui_ImplVoksel_InitInfo* v = &bd->VulkanInitInfo;
    if (pipeline == VK_NULL_HANDLE)
        pipeline = bd->Pipeline;

    // Allocate array to store enough vertex/index buffers. Each unique viewport gets its own storage.
    ImGui_ImplVoksel_ViewportData* viewport_renderer_data = (ImGui_ImplVoksel_ViewportData*)draw_data->OwnerViewport->RendererUserData;
    IM_ASSERT(viewport_renderer_data != nullptr);
    ImGui_ImplVokselH_WindowRenderBuffers* wrb = &viewport_renderer_data->RenderBuffers;
    if (wrb->FrameRenderBuffers == nullptr)
    {
        wrb->Index = 0;
        wrb->Count = v->ImageCount;
        wrb->FrameRenderBuffers = (ImGui_ImplVokselH_FrameRenderBuffers*)IM_ALLOC(sizeof(ImGui_ImplVokselH_FrameRenderBuffers) * wrb->Count);
        memset(wrb->FrameRenderBuffers, 0, sizeof(ImGui_ImplVokselH_FrameRenderBuffers) * wrb->Count);
    }
    IM_ASSERT(wrb->Count == v->ImageCount);
    wrb->Index = (wrb->Index + 1) % wrb->Count;
    ImGui_ImplVokselH_FrameRenderBuffers* rb = &wrb->FrameRenderBuffers[wrb->Index];

    if (draw_data->TotalVtxCount > 0)
    {
        // Create or resize the vertex/index buffers
        size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
        size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
        if (rb->VertexBuffer == VK_NULL_HANDLE || rb->VertexBufferSize < vertex_size)
            CreateOrResizeBuffer(rb->VertexBuffer, rb->VertexBufferMemory, rb->VertexBufferSize, vertex_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        if (rb->IndexBuffer == VK_NULL_HANDLE || rb->IndexBufferSize < index_size)
            CreateOrResizeBuffer(rb->IndexBuffer, rb->IndexBufferMemory, rb->IndexBufferSize, index_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

        // Upload vertex/index data into a single contiguous GPU buffer
        ImDrawVert* vtx_dst = nullptr;
        ImDrawIdx* idx_dst = nullptr;
        VkResult err = vkMapMemory(v->Context->device, rb->VertexBufferMemory, 0, rb->VertexBufferSize, 0, (void**)(&vtx_dst));
        check_vk_result(err);
        err = vkMapMemory(v->Context->device, rb->IndexBufferMemory, 0, rb->IndexBufferSize, 0, (void**)(&idx_dst));
        check_vk_result(err);
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            vtx_dst += cmd_list->VtxBuffer.Size;
            idx_dst += cmd_list->IdxBuffer.Size;
        }
        VkMappedMemoryRange range[2] = {};
        range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range[0].memory = rb->VertexBufferMemory;
        range[0].size = VK_WHOLE_SIZE;
        range[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range[1].memory = rb->IndexBufferMemory;
        range[1].size = VK_WHOLE_SIZE;
        err = vkFlushMappedMemoryRanges(v->Context->device, 2, range);
        check_vk_result(err);
        vkUnmapMemory(v->Context->device, rb->VertexBufferMemory);
        vkUnmapMemory(v->Context->device, rb->IndexBufferMemory);
    }

    // Setup desired Vulkan state
    ImGui_ImplVoksel_SetupRenderState(draw_data, pipeline, command_buffer, rb, fb_width, fb_height);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    // (Because we merged all buffers into a single one, we maintain our own offset into them)
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr)
            {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplVoksel_SetupRenderState(draw_data, pipeline, command_buffer, rb, fb_width, fb_height);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

                // Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
                if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
                if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
                if (clip_max.x > fb_width) { clip_max.x = (float)fb_width; }
                if (clip_max.y > fb_height) { clip_max.y = (float)fb_height; }
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                // Apply scissor/clipping rectangle
                VkRect2D scissor;
                scissor.offset.x = (int32_t)(clip_min.x);
                scissor.offset.y = (int32_t)(clip_min.y);
                scissor.extent.width = (uint32_t)(clip_max.x - clip_min.x);
                scissor.extent.height = (uint32_t)(clip_max.y - clip_min.y);
                vkCmdSetScissor(command_buffer, 0, 1, &scissor);

                // Bind DescriptorSet with font or user texture
                VkDescriptorSet desc_set[1] = { (VkDescriptorSet)pcmd->TextureId };
                if (sizeof(ImTextureID) < sizeof(ImU64))
                {
                    // We don't support texture switches if ImTextureID hasn't been redefined to be 64-bit. Do a flaky check that other textures haven't been used.
                    IM_ASSERT(pcmd->TextureId == (ImTextureID)bd->FontDescriptorSet);
                    desc_set[0] = bd->FontDescriptorSet;
                }
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bd->PipelineLayout, 0, 1, desc_set, 0, nullptr);

                // Draw
                vkCmdDrawIndexed(command_buffer, pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }

    // Note: at this point both vkCmdSetViewport() and vkCmdSetScissor() have been called.
    // Our last values will leak into user/application rendering IF:
    // - Your app uses a pipeline with VK_DYNAMIC_STATE_VIEWPORT or VK_DYNAMIC_STATE_SCISSOR dynamic state
    // - And you forgot to call vkCmdSetViewport() and vkCmdSetScissor() yourself to explicitly set that state.
    // If you use VK_DYNAMIC_STATE_VIEWPORT or VK_DYNAMIC_STATE_SCISSOR you are responsible for setting the values before rendering.
    // In theory we should aim to backup/restore those values but I am not sure this is possible.
    // We perform a call to vkCmdSetScissor() to set back a full viewport which is likely to fix things for 99% users but technically this is not perfect. (See github #4644)
    VkRect2D scissor = { { 0, 0 }, { (uint32_t)fb_width, (uint32_t)fb_height } };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

bool ImGui_ImplVoksel_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplVoksel_Data* bd = ImGui_ImplVoksel_GetBackendData();
    ImGui_ImplVoksel_InitInfo* v = &bd->VulkanInitInfo;

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    size_t upload_size = width * height * 4;

    VkResult err;
    // Create the Image:
    {
        VkImageCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType = VK_IMAGE_TYPE_2D;
        info.format = VK_FORMAT_R8G8B8A8_UNORM;
        info.extent.width = width;
        info.extent.height = height;
        info.extent.depth = 1;
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.samples = VK_SAMPLE_COUNT_1_BIT;
        info.tiling = VK_IMAGE_TILING_OPTIMAL;
        info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo alloc_create_info {};
        alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
        alloc_create_info.flags = 0;

        err = vmaCreateImage(v->Context->allocator, &info, &alloc_create_info, &bd->FontImage, &bd->FontAllocation, nullptr);
        check_vk_result(err);
    }

    // Create the Image View:
    {
        VkImageViewCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        info.image = bd->FontImage;
        info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        info.format = VK_FORMAT_R8G8B8A8_UNORM;
        info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.layerCount = 1;

        err = vkCreateImageView(v->Context->device, &info, v->Allocator, &bd->FontView);
        check_vk_result(err);
    }

    // Create the Descriptor Set:
    bd->FontDescriptorSet = (VkDescriptorSet)ImGui_ImplVoksel_AddTexture(bd->FontSampler, bd->FontView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Create the Upload Buffer:
    {
        VkBufferCreateInfo buffer_info = {};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = upload_size;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo alloc_create_info {};
        alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
        alloc_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        err = vmaCreateBuffer(v->Context->allocator, &buffer_info, &alloc_create_info, &bd->UploadBuffer, &bd->UploadBufferAllocation, nullptr);
        check_vk_result(err);
    }

    // Upload to Buffer:
    {
        void *map = nullptr;
        err = vmaMapMemory(v->Context->allocator, bd->UploadBufferAllocation, &map);
        check_vk_result(err);
        memcpy(map, pixels, upload_size);
        vmaUnmapMemory(v->Context->allocator, bd->UploadBufferAllocation);
        vmaFlushAllocation(v->Context->allocator, bd->UploadBufferAllocation, 0, upload_size);
    }

    v->Context->transition_image_layout(bd->FontImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    v->Context->copy_buffer_to_image(bd->UploadBuffer, bd->FontImage, width, height);
    v->Context->transition_image_layout(bd->FontImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Store our identifier
    io.Fonts->SetTexID((ImTextureID)bd->FontDescriptorSet);

    return true;
}

struct ImGuiVertex : ImDrawVert {
    static auto alloc_binding_description() {
        auto d = new VkVertexInputBindingDescription();
        d->binding = 0;
        d->stride = sizeof(ImGuiVertex);
        d->inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return d;
    }

    static constexpr usize num_attribute_descriptions = 3;
    static auto alloc_attribute_descriptions() {
        auto d = new VkVertexInputAttributeDescription[num_attribute_descriptions];
        d[0].binding = 0;
        d[0].location = 0;
        d[0].format = VK_FORMAT_R32G32_SFLOAT;
        d[0].offset = offsetof(ImGuiVertex, pos);

        d[1].binding = 0;
        d[1].location = 1;
        d[1].format = VK_FORMAT_R32G32_SFLOAT;
        d[1].offset = offsetof(ImGuiVertex, uv);

        d[2].binding = 0;
        d[2].location = 2;
        d[2].format = VK_FORMAT_R8G8B8A8_UNORM;
        d[2].offset = offsetof(ImGuiVertex, col);
        return d;
    }
};

static void ImGui_ImplVoksel_CreatePipeline(VkDevice device, const VkAllocationCallbacks* allocator, VkPipelineCache pipelineCache, VkRenderPass renderPass, VkSampleCountFlagBits MSAASamples, VkPipeline* pipeline, uint32_t subpass)
{
    ImGui_ImplVoksel_Data* bd = ImGui_ImplVoksel_GetBackendData();

    *pipeline = PipelineBuilder::begin(device)
        .vert_shader("imgui").frag_shader("imgui").vertex_input_info<ImGuiVertex>()
        .input_assembly().viewport_state().rasterizer(VK_CULL_MODE_NONE)
        .multisampling() // TODO: enable multisampling maybe
        .depth_stencil(false, false).color_blending(true)
        .dynamic_states({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
        .layout(bd->PipelineLayout).finish_graphics(renderPass, subpass);
}

bool ImGui_ImplVoksel_CreateDeviceObjects()
{
    ImGui_ImplVoksel_Data* bd = ImGui_ImplVoksel_GetBackendData();
    ImGui_ImplVoksel_InitInfo* v = &bd->VulkanInitInfo;
    VkResult err;

    if (!bd->FontSampler)
    {
        // Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling.
        VkSamplerCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter = VK_FILTER_LINEAR;
        info.minFilter = VK_FILTER_LINEAR;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        info.minLod = -1000;
        info.maxLod = 1000;
        info.maxAnisotropy = 1.0f;
        err = vkCreateSampler(v->Context->device, &info, v->Allocator, &bd->FontSampler);
        check_vk_result(err);
    }

    if (!bd->DescriptorSetLayout)
    {
        VkDescriptorSetLayoutBinding binding[1] = {};
        binding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding[0].descriptorCount = 1;
        binding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        VkDescriptorSetLayoutCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = 1;
        info.pBindings = binding;
        err = vkCreateDescriptorSetLayout(v->Context->device, &info, v->Allocator, &bd->DescriptorSetLayout);
        check_vk_result(err);
    }

    if (!bd->PipelineLayout)
    {
        // Constants: we are using 'vec2 offset' and 'vec2 scale' instead of a full 3d projection matrix
        VkPushConstantRange push_constants[1] = {};
        push_constants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_constants[0].offset = sizeof(float) * 0;
        push_constants[0].size = sizeof(float) * 4;
        VkDescriptorSetLayout set_layout[1] = { bd->DescriptorSetLayout };
        VkPipelineLayoutCreateInfo layout_info = {};
        layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount = 1;
        layout_info.pSetLayouts = set_layout;
        layout_info.pushConstantRangeCount = 1;
        layout_info.pPushConstantRanges = push_constants;
        err = vkCreatePipelineLayout(v->Context->device, &layout_info, v->Allocator, &bd->PipelineLayout);
        check_vk_result(err);
    }

    ImGui_ImplVoksel_CreatePipeline(v->Context->device, v->Allocator, nullptr, bd->RenderPass, v->MSAASamples, &bd->Pipeline, bd->Subpass);

    return true;
}

void    ImGui_ImplVoksel_DestroyFontUploadObjects()
{
    ImGui_ImplVoksel_Data* bd = ImGui_ImplVoksel_GetBackendData();
    ImGui_ImplVoksel_InitInfo* v = &bd->VulkanInitInfo;
    if (bd->UploadBuffer)
    {
        vmaDestroyBuffer(v->Context->allocator, bd->UploadBuffer, bd->UploadBufferAllocation);
        bd->UploadBuffer = VK_NULL_HANDLE;
    }
}

void    ImGui_ImplVoksel_DestroyDeviceObjects()
{
    ImGui_ImplVoksel_Data* bd = ImGui_ImplVoksel_GetBackendData();
    ImGui_ImplVoksel_InitInfo* v = &bd->VulkanInitInfo;
    ImGui_ImplVoksel_DestroyFontUploadObjects();

    if (bd->ShaderModuleVert)     { vkDestroyShaderModule(v->Context->device, bd->ShaderModuleVert, v->Allocator); bd->ShaderModuleVert = VK_NULL_HANDLE; }
    if (bd->ShaderModuleFrag)     { vkDestroyShaderModule(v->Context->device, bd->ShaderModuleFrag, v->Allocator); bd->ShaderModuleFrag = VK_NULL_HANDLE; }
    if (bd->FontView)             { vkDestroyImageView(v->Context->device, bd->FontView, v->Allocator); bd->FontView = VK_NULL_HANDLE; }
    if (bd->FontImage)            { vmaDestroyImage(v->Context->allocator, bd->FontImage, bd->FontAllocation); bd->FontImage = VK_NULL_HANDLE; }
    if (bd->FontSampler)          { vkDestroySampler(v->Context->device, bd->FontSampler, v->Allocator); bd->FontSampler = VK_NULL_HANDLE; }
    if (bd->DescriptorSetLayout)  { vkDestroyDescriptorSetLayout(v->Context->device, bd->DescriptorSetLayout, v->Allocator); bd->DescriptorSetLayout = VK_NULL_HANDLE; }
    if (bd->PipelineLayout)       { vkDestroyPipelineLayout(v->Context->device, bd->PipelineLayout, v->Allocator); bd->PipelineLayout = VK_NULL_HANDLE; }
    if (bd->Pipeline)             { vkDestroyPipeline(v->Context->device, bd->Pipeline, v->Allocator); bd->Pipeline = VK_NULL_HANDLE; }
}

bool    ImGui_ImplVoksel_Init(ImGui_ImplVoksel_InitInfo* info, VkRenderPass render_pass)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    ImGui_ImplVoksel_Data* bd = IM_NEW(ImGui_ImplVoksel_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_voksel";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

    IM_ASSERT(info->Context != nullptr);
    IM_ASSERT(info->DescriptorPool != VK_NULL_HANDLE);
    IM_ASSERT(info->MinImageCount >= 2);
    IM_ASSERT(info->ImageCount >= info->MinImageCount);
    IM_ASSERT(render_pass != VK_NULL_HANDLE);

    bd->VulkanInitInfo = *info;
    bd->RenderPass = render_pass;
    bd->Subpass = info->Subpass;

    ImGui_ImplVoksel_CreateDeviceObjects();

    // Our render function expect RendererUserData to be storing the window render buffer we need (for the main viewport we won't use ->Window)
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->RendererUserData = IM_NEW(ImGui_ImplVoksel_ViewportData)();

    return true;
}

void ImGui_ImplVoksel_Shutdown()
{
    ImGui_ImplVoksel_Data* bd = ImGui_ImplVoksel_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    // First destroy objects in all viewports
    ImGui_ImplVoksel_DestroyDeviceObjects();

    // Manually delete main viewport render data in-case we haven't initialized for viewports
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    if (ImGui_ImplVoksel_ViewportData* vd = (ImGui_ImplVoksel_ViewportData*)main_viewport->RendererUserData) {
        for (u32 i = 0; i < vd->RenderBuffers.Count; i++) {
            auto render_buffer = vd->RenderBuffers.FrameRenderBuffers[i];
            vkDestroyBuffer(bd->VulkanInitInfo.Context->device, render_buffer.VertexBuffer, nullptr);
            vkFreeMemory(bd->VulkanInitInfo.Context->device, render_buffer.VertexBufferMemory, nullptr);
            vkDestroyBuffer(bd->VulkanInitInfo.Context->device, render_buffer.IndexBuffer, nullptr);
            vkFreeMemory(bd->VulkanInitInfo.Context->device, render_buffer.IndexBufferMemory, nullptr);
        }
        IM_DELETE(vd->RenderBuffers.FrameRenderBuffers);
        IM_DELETE(vd);
    }
    main_viewport->RendererUserData = nullptr;

    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasViewports);
    IM_DELETE(bd);
}

void ImGui_ImplVoksel_NewFrame()
{
    ImGui_ImplVoksel_Data* bd = ImGui_ImplVoksel_GetBackendData();
    IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplVoksel_Init()?");
    IM_UNUSED(bd);
}

void ImGui_ImplVoksel_SetMinImageCount(uint32_t min_image_count)
{
    ImGui_ImplVoksel_Data* bd = ImGui_ImplVoksel_GetBackendData();
    IM_ASSERT(min_image_count >= 2);
    if (bd->VulkanInitInfo.MinImageCount == min_image_count)
        return;

    IM_ASSERT(0); // FIXME-VIEWPORT: Unsupported. Need to recreate all swap chains!
    ImGui_ImplVoksel_InitInfo* v = &bd->VulkanInitInfo;
    VkResult err = vkDeviceWaitIdle(v->Context->device);
    check_vk_result(err);

    bd->VulkanInitInfo.MinImageCount = min_image_count;
}

// Register a texture
// FIXME: This is experimental in the sense that we are unsure how to best design/tackle this problem, please post to https://github.com/ocornut/imgui/pull/914 if you have suggestions.
VkDescriptorSet ImGui_ImplVoksel_AddTexture(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout)
{
    ImGui_ImplVoksel_Data* bd = ImGui_ImplVoksel_GetBackendData();
    ImGui_ImplVoksel_InitInfo* v = &bd->VulkanInitInfo;

    // Create Descriptor Set:
    VkDescriptorSet descriptor_set;
    {
        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = v->DescriptorPool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &bd->DescriptorSetLayout;
        VkResult err = vkAllocateDescriptorSets(v->Context->device, &alloc_info, &descriptor_set);
        check_vk_result(err);
    }

    // Update the Descriptor Set:
    {
        VkDescriptorImageInfo desc_image[1] = {};
        desc_image[0].sampler = sampler;
        desc_image[0].imageView = image_view;
        desc_image[0].imageLayout = image_layout;
        VkWriteDescriptorSet write_desc[1] = {};
        write_desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_desc[0].dstSet = descriptor_set;
        write_desc[0].descriptorCount = 1;
        write_desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write_desc[0].pImageInfo = desc_image;
        vkUpdateDescriptorSets(v->Context->device, 1, write_desc, 0, nullptr);
    }
    return descriptor_set;
}

void ImGui_ImplVoksel_RemoveTexture(VkDescriptorSet descriptor_set)
{
    ImGui_ImplVoksel_Data* bd = ImGui_ImplVoksel_GetBackendData();
    ImGui_ImplVoksel_InitInfo* v = &bd->VulkanInitInfo;
    vkFreeDescriptorSets(v->Context->device, v->DescriptorPool, 1, &descriptor_set);
}
