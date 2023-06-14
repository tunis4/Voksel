#include "chunk_renderer.hpp"
#include "context.hpp"
#include "chunk_mesh_builder.hpp"
#include "../client.hpp"

#include <cstring>

namespace render {
    // const auto example_vertices = std::vector<ChunkVertex>({
    //     {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}}, // bottom left
    //     {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}}, // top left
    //     {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}}, // bottom right
    //     {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}}, // top right
        
    //     {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}}, // bottom left
    //     {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}}, // top left
    //     {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f}}, // bottom right
    //     {{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}},  // top right
    // });

    // const auto example_indices = std::vector<u32>({
    //     0, 1, 2, 1, 3, 2, 4, 5, 6, 5, 7, 6
    // });

    static ChunkRender example_chunk;

    void ChunkRenderer::init() {
        VkDescriptorSetLayoutBinding ub_layout_binding {};
        ub_layout_binding.binding = 0;
        ub_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ub_layout_binding.descriptorCount = 1;
        ub_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding sampler_layout_binding {};
        sampler_layout_binding.binding = 1;
        sampler_layout_binding.descriptorCount = 1;
        sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampler_layout_binding.pImmutableSamplers = nullptr;
        sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array bindings = std::to_array<VkDescriptorSetLayoutBinding>({ ub_layout_binding, sampler_layout_binding });
        VkDescriptorSetLayoutCreateInfo layout_info {};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = bindings.size();
        layout_info.pBindings = bindings.data();

        CHECK_VK(vkCreateDescriptorSetLayout(m_context.device, &layout_info, nullptr, &m_descriptor_set_layout));

        m_block_texture.init();
        
        for (auto &frame : m_per_frame) {
            VmaAllocationInfo alloc_info;
            m_context.create_buffer(&frame.m_uniform_buffer, &frame.m_uniform_buffer_allocation, &alloc_info, sizeof(UniformBuffer), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
            frame.m_uniform_buffer_mapped = alloc_info.pMappedData;
        }
        
        VkDescriptorSetAllocateInfo alloc_info {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = m_context.descriptor_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &m_descriptor_set_layout;

        for (auto &frame : m_per_frame) {
            CHECK_VK(vkAllocateDescriptorSets(m_context.device, &alloc_info, &frame.m_descriptor_set));
            
            VkDescriptorBufferInfo buffer_info {};
            buffer_info.buffer = frame.m_uniform_buffer;
            buffer_info.offset = 0;
            buffer_info.range = sizeof(UniformBuffer);

            VkDescriptorImageInfo image_info {};
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info.imageView = m_block_texture.m_image_view;
            image_info.sampler = m_block_texture.m_sampler;

            std::array<VkWriteDescriptorSet, 2> descriptor_writes {};

            descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes[0].dstSet = frame.m_descriptor_set;
            descriptor_writes[0].dstBinding = 0;
            descriptor_writes[0].dstArrayElement = 0;
            descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_writes[0].descriptorCount = 1;
            descriptor_writes[0].pBufferInfo = &buffer_info;

            descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes[1].dstSet = frame.m_descriptor_set;
            descriptor_writes[1].dstBinding = 1;
            descriptor_writes[1].dstArrayElement = 0;
            descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_writes[1].descriptorCount = 1;
            descriptor_writes[1].pImageInfo = &image_info;

            vkUpdateDescriptorSets(m_context.device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
        }

        m_pipeline = PipelineBuilder::begin(m_context.device)
            .shaders("chunk").vertex_input_info<ChunkVertex>()
            .input_assembly().viewport_state().rasterizer(VK_CULL_MODE_BACK_BIT)
            .multisampling().depth_stencil(true, true).color_blending(true)
            .dynamic_states({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .layout<PushConstants>(&m_pipeline_layout, &m_descriptor_set_layout)
            .finish(m_context.render_pass);

        m_mesh_builder = new ChunkMeshBuilder(this);
        auto world = client::Client::get()->world();
        example_chunk.m_chunk_pos = glm::i32vec3(0, 0, 0);
        example_chunk.m_chunk = world->get_chunk(example_chunk.m_chunk_pos).get();
        m_mesh_builder->m_current = &example_chunk;
        m_mesh_builder->build();
        allocate_chunk_mesh(example_chunk, m_mesh_builder->m_vertices, m_mesh_builder->m_indices);
    }

    void ChunkRenderer::cleanup() {
        m_block_texture.cleanup();

        vkDestroyPipeline(m_context.device, m_pipeline, nullptr);
        vkDestroyPipelineLayout(m_context.device, m_pipeline_layout, nullptr);

        for (auto &frame : m_per_frame) {
            vmaDestroyBuffer(m_context.allocator, frame.m_uniform_buffer, frame.m_uniform_buffer_allocation);
        }
    }

    void ChunkRenderer::allocate_chunk_mesh(ChunkRender &chunk, const std::vector<ChunkVertex> &vertices, const std::vector<u32> &indices) {
        VkDeviceSize vertex_buffer_size = vertices.size() * sizeof(ChunkVertex);
        VkDeviceSize index_buffer_size = indices.size() * sizeof(u32);
        VkDeviceSize staging_buffer_size = std::max(vertex_buffer_size, index_buffer_size);
        VkBuffer staging_buffer;
        VmaAllocation staging_buffer_allocation;
        void *staging_buffer_mapped;

        m_context.create_buffer(&staging_buffer, &staging_buffer_allocation, nullptr, staging_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        vmaMapMemory(m_context.allocator, staging_buffer_allocation, &staging_buffer_mapped);

        std::memcpy(staging_buffer_mapped, vertices.data(), vertex_buffer_size);
        m_context.create_buffer(&chunk.m_vertex_buffer, &chunk.m_vertex_buffer_allocation, nullptr, vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0);
        m_context.copy_buffer(staging_buffer, chunk.m_vertex_buffer, vertex_buffer_size);

        std::memcpy(staging_buffer_mapped, indices.data(), index_buffer_size);
        m_context.create_buffer(&chunk.m_index_buffer, &chunk.m_index_buffer_allocation, nullptr, index_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0);
        m_context.copy_buffer(staging_buffer, chunk.m_index_buffer, index_buffer_size);
        chunk.m_num_indices = indices.size();

        vmaUnmapMemory(m_context.allocator, staging_buffer_allocation);
        vmaDestroyBuffer(m_context.allocator, staging_buffer, staging_buffer_allocation);
    }

    void ChunkRenderer::delete_chunk_mesh(ChunkRender &chunk) {
        vmaDestroyBuffer(m_context.allocator, chunk.m_index_buffer, chunk.m_index_buffer_allocation);
        vmaDestroyBuffer(m_context.allocator, chunk.m_vertex_buffer, chunk.m_vertex_buffer_allocation);
    }
    
    void ChunkRenderer::record(VkCommandBuffer cmd, uint frame_index) {
        PerFrame &frame = m_per_frame[frame_index];
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        UniformBuffer ub {};
        ub.view = m_context.camera->m_view_matrix;
        ub.projection = m_context.camera->m_projection_matrix;
        std::memcpy(frame.m_uniform_buffer_mapped, &ub, sizeof(ub));

        ChunkRender &current_chunk = example_chunk;

        VkDeviceSize vb_offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &current_chunk.m_vertex_buffer, &vb_offset);
        vkCmdBindIndexBuffer(cmd, current_chunk.m_index_buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &frame.m_descriptor_set, 0, nullptr);
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0, 0, 0));

        PushConstants push_constants;
        push_constants.model = model;
        vkCmdPushConstants(cmd, m_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &push_constants);
        
        vkCmdDrawIndexed(cmd, current_chunk.m_num_indices, 1, 0, 0, 0);
    }
}
