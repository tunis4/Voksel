#include "chunk_renderer.hpp"
#include "context.hpp"
#include "chunk_mesh_builder.hpp"
#include "../client.hpp"

#include <cstring>
#include <chrono>
#include <imgui/imgui.h>
#include <vulkan/vulkan_core.h>

namespace render {
    void ChunkRenderer::init() {
        VkDescriptorSetLayoutBinding ub_layout_binding {};
        ub_layout_binding.binding = 0;
        ub_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ub_layout_binding.descriptorCount = 1;
        ub_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

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
        m_last_camera_pos = glm::i32vec3(1024, 1024, 1024); // so that it immediately gets reset properly
    }

    void ChunkRenderer::cleanup() {
        m_block_texture.cleanup();

        vkDestroyPipeline(m_context.device, m_pipeline, nullptr);
        vkDestroyPipelineLayout(m_context.device, m_pipeline_layout, nullptr);

        for (auto &frame : m_per_frame) {
            vmaDestroyBuffer(m_context.allocator, frame.m_uniform_buffer, frame.m_uniform_buffer_allocation);
        }

        vkDestroyDescriptorSetLayout(m_context.device, m_descriptor_set_layout, nullptr);
    }

    void ChunkRenderer::allocate_chunk_mesh(ChunkRender *chunk, const std::vector<ChunkVertex> &vertices, const std::vector<u32> &indices) {
        VkDeviceSize vertex_buffer_size = vertices.size() * sizeof(ChunkVertex);
        VkDeviceSize index_buffer_size = indices.size() * sizeof(u32);
        VkDeviceSize staging_buffer_size = std::max(vertex_buffer_size, index_buffer_size);
        VkBuffer staging_buffer;
        VmaAllocation staging_buffer_allocation;
        void *staging_buffer_mapped;

        if (staging_buffer_size == 0)
            return;

        m_context.create_buffer(&staging_buffer, &staging_buffer_allocation, nullptr, staging_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        vmaMapMemory(m_context.allocator, staging_buffer_allocation, &staging_buffer_mapped);

        std::memcpy(staging_buffer_mapped, vertices.data(), vertex_buffer_size);
        m_context.create_buffer(&chunk->m_vertex_buffer, &chunk->m_vertex_buffer_allocation, nullptr, vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0);
        m_context.copy_buffer(staging_buffer, chunk->m_vertex_buffer, vertex_buffer_size);

        std::memcpy(staging_buffer_mapped, indices.data(), index_buffer_size);
        m_context.create_buffer(&chunk->m_index_buffer, &chunk->m_index_buffer_allocation, nullptr, index_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0);
        m_context.copy_buffer(staging_buffer, chunk->m_index_buffer, index_buffer_size);
        chunk->m_num_indices = indices.size();

        vmaUnmapMemory(m_context.allocator, staging_buffer_allocation);
        vmaDestroyBuffer(m_context.allocator, staging_buffer, staging_buffer_allocation);
    }

    void ChunkRenderer::update() {
        i64 time_spent = 0;
        auto start = std::chrono::high_resolution_clock::now();
        if (!m_chunks_to_mesh.empty()) {
            while (time_spent < 100000) {
                if (m_chunks_to_mesh.empty())
                    break;
                
                auto chunk = m_chunks_to_mesh.back();
                m_mesh_builder->m_current = chunk;
                m_mesh_builder->build();
                allocate_chunk_mesh(chunk, m_mesh_builder->m_vertices, m_mesh_builder->m_indices);
                m_chunks_meshed.push_back(chunk);
                m_chunks_to_mesh.pop_back();
                auto stop = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
                time_spent += duration.count();
            }
        }
    }
    
    void ChunkRenderer::record(VkCommandBuffer cmd, uint frame_index) {
        ImGui::SeparatorText("Chunk Renderer");
        static i32 render_distance = 8;
        ImGui::SliderInt("Render distance", &render_distance, 1, 32);
        static i32 reiterate_threshold = 32; // when moved this many blocks
        ImGui::SliderInt("Reiteration threshold", &reiterate_threshold, 1, 32);

        PerFrame &frame = m_per_frame[frame_index];
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

        UniformBuffer ub {};
        ub.view = m_context.camera->m_view_matrix;
        ub.projection = m_context.camera->m_projection_matrix;
        ub.fog_color = glm::vec3(0.00143f, 0.35374f, 0.61868f);
        // static f32 fog_near = 352;
        // static f32 fog_far = 384;
        // ImGui::SliderFloat("Fog near", &fog_near, 32, 512);
        // ImGui::SliderFloat("Fog far", &fog_far, 32, 512);
        // ub.fog_near = fog_near;
        // ub.fog_far = fog_far;
        ub.fog_near = ((f32)render_distance - 2.5f) * 64;
        ub.fog_far = ((f32)render_distance - 2.0f) * 64;
        std::memcpy(frame.m_uniform_buffer_mapped, &ub, sizeof(ub));

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &frame.m_descriptor_set, 0, nullptr);

        usize num_vertices_rendered = 0;
        usize num_draw_calls = 0;

        bool reiterate = util::i32vec3_distance_squared(m_context.camera->block_pos(), m_last_camera_pos) > reiterate_threshold * reiterate_threshold;
        if (reiterate)
            m_last_camera_pos = m_context.camera->block_pos();
        glm::i32vec3 camera_chunk_pos = util::signed_i32vec3_divide(m_context.camera->block_pos(), 32).first;

        auto world = client::Client::get()->world();

        if (reiterate) {
            auto remove = std::partition(m_chunks_meshed.begin(), m_chunks_meshed.end(), [=] (ChunkRender *chunk) {
                return util::i32vec3_distance_squared(camera_chunk_pos, chunk->m_chunk_pos) < render_distance * render_distance;
            });
            if (remove != m_chunks_meshed.end()) {
                for (auto i = remove; i != m_chunks_meshed.end(); i++) {
                    ChunkRender *chunk = *i;
                    m_context.buffer_deletions.push(BufferDeletion(chunk->m_vertex_buffer, chunk->m_vertex_buffer_allocation));
                    m_context.buffer_deletions.push(BufferDeletion(chunk->m_index_buffer, chunk->m_index_buffer_allocation));
                    delete chunk;
                }
                
                m_chunks_meshed.erase(remove, m_chunks_meshed.end());
            }
        }

        for (auto chunk : m_chunks_meshed) {
            if (chunk->m_vertex_buffer == nullptr)
                continue;

            glm::vec3 min = glm::vec3(chunk->m_chunk_pos) * 64.0f;
            glm::vec3 max = glm::vec3(chunk->m_chunk_pos + glm::i32vec3(1)) * 64.0f;
            if (!m_context.camera->m_frustum.is_box_visible(min, max))
                continue;

            VkDeviceSize vb_offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &chunk->m_vertex_buffer, &vb_offset);
            vkCmdBindIndexBuffer(cmd, chunk->m_index_buffer, 0, VK_INDEX_TYPE_UINT32);
        
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(chunk->m_chunk_pos.x * 64, chunk->m_chunk_pos.y * 64, chunk->m_chunk_pos.z * 64));

            PushConstants push_constants;
            push_constants.model = model;
            vkCmdPushConstants(cmd, m_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &push_constants);
            
            vkCmdDrawIndexed(cmd, chunk->m_num_indices, 1, 0, 0, 0);
            num_vertices_rendered += chunk->m_num_indices;
            num_draw_calls++;
        }

        if (reiterate) {
            for (auto chunk : m_chunks_to_mesh)
                delete chunk;
            m_chunks_to_mesh.clear();

            for (i32 x = -render_distance; x < render_distance; x++) {
                for (i32 y = -render_distance; y < render_distance; y++) {
                    for (i32 z = -render_distance; z < render_distance; z++) {
                        if (x * x + y * y + z * z >= render_distance * render_distance) // only check in a sphere
                            continue;
                        
                        glm::i32vec3 chunk_pos = glm::i32vec3(x, y, z) + camera_chunk_pos;
                        auto result = std::find_if(m_chunks_meshed.begin(), m_chunks_meshed.end(), [=] (ChunkRender *chunk) {
                            return chunk->m_chunk_pos == chunk_pos;
                        });
                        if (result == m_chunks_meshed.end()) {
                            auto chunk = new ChunkRender();
                            chunk->m_chunk_pos = chunk_pos;
                            chunk->m_chunk = world->get_chunk(chunk_pos).get();
                            m_chunks_to_mesh.push_back(chunk);
                        }
                    }
                }
            }

            std::sort(m_chunks_to_mesh.begin(), m_chunks_to_mesh.end(), [=] (ChunkRender *a, ChunkRender *b) {
                return util::i32vec3_distance_squared(a->m_chunk_pos, camera_chunk_pos) > util::i32vec3_distance_squared(b->m_chunk_pos, camera_chunk_pos);
            });
        }

        ImGui::Text("Draw calls: %ld", num_draw_calls);
        ImGui::Text("Vertices rendered: %ld", num_vertices_rendered);
        ImGui::Text("Chunks to mesh: %ld", m_chunks_to_mesh.size());
        ImGui::Text("Chunks meshed: %ld", m_chunks_meshed.size());
    }
}
