#include "chunk_renderer.hpp"
#include "context.hpp"
#include "chunk_mesh_builder.hpp"
#include "box_renderer.hpp"
#include "../client.hpp"

#include <cstring>
#include <chrono>
#include <imgui/imgui.h>
#include <entt/entt.hpp>
#include <vulkan/vulkan_core.h>

namespace render {
    void ChunkRenderer::init() {
        auto texture_manager = entt::locator<TextureManager>::value();
        u32 num_textures = texture_manager.num_textures();

        VkDescriptorSetLayoutBinding ub_layout_binding {};
        ub_layout_binding.binding = 0;
        ub_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ub_layout_binding.descriptorCount = 1;
        ub_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        
        VkDescriptorSetLayoutBinding sampler_layout_binding {};
        sampler_layout_binding.binding = 1;
        sampler_layout_binding.descriptorCount = num_textures;
        sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampler_layout_binding.pImmutableSamplers = nullptr;
        sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBindingFlagsCreateInfo set_layout_binding_flags {};
        set_layout_binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        set_layout_binding_flags.bindingCount = 2;
        constexpr std::array descriptor_binding_flags = std::to_array<VkDescriptorBindingFlags>({
            0, VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
        });
        set_layout_binding_flags.pBindingFlags = descriptor_binding_flags.data();

        std::array bindings = std::to_array<VkDescriptorSetLayoutBinding>({ ub_layout_binding, sampler_layout_binding });
        VkDescriptorSetLayoutCreateInfo layout_info {};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.pNext = &set_layout_binding_flags;
        layout_info.bindingCount = bindings.size();
        layout_info.pBindings = bindings.data();

        CHECK_VK(vkCreateDescriptorSetLayout(m_context.device, &layout_info, nullptr, &m_descriptor_set_layout));

        for (auto &frame : m_per_frame)
            frame.uniform_buffer.create(m_context, sizeof(UniformBuffer), sizeof(UniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        
        VkDescriptorSetVariableDescriptorCountAllocateInfo variable_descriptor_count_alloc_info {};
        variable_descriptor_count_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
        variable_descriptor_count_alloc_info.descriptorSetCount = 1;
        variable_descriptor_count_alloc_info.pDescriptorCounts = &num_textures;

        VkDescriptorSetAllocateInfo alloc_info {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext = &variable_descriptor_count_alloc_info;
        alloc_info.descriptorPool = m_context.descriptor_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &m_descriptor_set_layout;

        std::vector<VkDescriptorImageInfo> image_info(num_textures);
        for (u32 i = 0; i < num_textures; i++) {
            image_info[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info[i].imageView = texture_manager.m_textures[i].m_image_view;
            image_info[i].sampler = texture_manager.m_textures[i].m_sampler;
        }

        for (auto &frame : m_per_frame) {
            CHECK_VK(vkAllocateDescriptorSets(m_context.device, &alloc_info, &frame.descriptor_set));
            
            VkDescriptorBufferInfo buffer_info {};
            buffer_info.buffer = frame.uniform_buffer.m_buffer;
            buffer_info.offset = 0;
            buffer_info.range = sizeof(UniformBuffer);

            std::array<VkWriteDescriptorSet, 2> descriptor_writes {};

            descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes[0].dstSet = frame.descriptor_set;
            descriptor_writes[0].dstBinding = 0;
            descriptor_writes[0].dstArrayElement = 0;
            descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptor_writes[0].descriptorCount = 1;
            descriptor_writes[0].pBufferInfo = &buffer_info;

            descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_writes[1].dstSet = frame.descriptor_set;
            descriptor_writes[1].dstBinding = 1;
            descriptor_writes[1].dstArrayElement = 0;
            descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_writes[1].descriptorCount = num_textures;
            descriptor_writes[1].pImageInfo = image_info.data();

            vkUpdateDescriptorSets(m_context.device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
        }

        m_pipeline = PipelineBuilder::begin(m_context.device)
            .shaders("chunk").vertex_input_info<ChunkVertex>()
            .input_assembly().viewport_state().rasterizer(VK_CULL_MODE_BACK_BIT)
            .multisampling().depth_stencil(true, true).color_blending(false)
            .dynamic_states({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
            .layout<PushConstants>(&m_pipeline_layout, &m_descriptor_set_layout)
            .finish(m_context.render_pass);

        constexpr usize vertex_buffer_size = 128 * 1024 * 1024;
        m_vertex_buffer.create(m_context, vertex_buffer_size, 64 * 1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, PersistentBuffer::WRITE, true);

        m_mesh_builder = new ChunkMeshBuilder(this);
        m_last_camera_pos = glm::i32vec3(1024, 1024, 1024); // far away so that it immediately gets reset properly
        m_timer = 0.0f;
    }

    void ChunkRenderer::cleanup() {
        delete m_mesh_builder;

        m_vertex_buffer.destroy(m_context);

        for (auto chunk : m_chunks_meshed) {
            vmaDestroyBuffer(m_context.allocator, chunk->m_vertex_buffer, chunk->m_vertex_buffer_allocation);
            vmaDestroyBuffer(m_context.allocator, chunk->m_index_buffer, chunk->m_index_buffer_allocation);
            delete chunk;
        }

        for (auto chunk : m_chunks_to_mesh)
            delete chunk;

        vkDestroyPipeline(m_context.device, m_pipeline, nullptr);
        vkDestroyPipelineLayout(m_context.device, m_pipeline_layout, nullptr);

        for (auto &frame : m_per_frame)
            frame.uniform_buffer.destroy(m_context);

        vkDestroyDescriptorSetLayout(m_context.device, m_descriptor_set_layout, nullptr);
    }

    void ChunkRenderer::allocate_chunk_mesh(ChunkRender *chunk, const std::vector<ChunkVertex> &vertices, const std::vector<u32> &indices) {
        usize vertex_buffer_size = vertices.size() * sizeof(ChunkVertex);
        usize index_buffer_size = indices.size() * sizeof(u32);
        usize staging_buffer_size = std::max(vertex_buffer_size, index_buffer_size);
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
    
    void ChunkRenderer::remesh_chunk_urgent(glm::i32vec3 chunk_pos) {
        auto remove = std::partition(m_chunks_meshed.begin(), m_chunks_meshed.end(), [=] (ChunkRender *chunk) {
            return chunk->m_chunk_pos != chunk_pos;
        });
        if (remove != m_chunks_meshed.end()) {
            ChunkRender *chunk = *remove;
            m_context.buffer_deletions.push(BufferDeletion(chunk->m_vertex_buffer, chunk->m_vertex_buffer_allocation));
            m_context.buffer_deletions.push(BufferDeletion(chunk->m_index_buffer, chunk->m_index_buffer_allocation));
            chunk->m_vertex_buffer = nullptr; chunk->m_vertex_buffer_allocation = nullptr;
            chunk->m_index_buffer = nullptr; chunk->m_index_buffer_allocation = nullptr;
            m_chunks_to_mesh_urgent.push_back(chunk);
            m_chunks_meshed.erase(remove, m_chunks_meshed.end());
        }
    }

    void ChunkRenderer::update(f64 delta_time) {
        m_timer += (f32)delta_time;

        i64 time_spent = 0;
        auto start = std::chrono::high_resolution_clock::now();
        if (!m_chunks_to_mesh.empty()) {
            while (time_spent < 10000) {
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
        auto world = client::Client::get()->world();
        
        ImGui::Begin("Chunk Renderer", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        static i32 render_distance = 5;
        ImGui::SliderInt("Render distance", &render_distance, 1, 32);
        static bool pause_reiteration = false;
        ImGui::Checkbox("Pause reiteration", &pause_reiteration);

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
        ub.fog_near = ((f32)render_distance - 2.5f) * 32;
        ub.fog_far = ((f32)render_distance - 2.0f) * 32;
        ub.timer = m_timer;
        frame.uniform_buffer.upload_data(m_context, &ub, sizeof(UniformBuffer), 0);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &frame.descriptor_set, 0, nullptr);

        usize num_vertices_rendered = 0;
        usize num_draw_calls = 0;

        bool reiterate = !pause_reiteration && util::i32vec3_distance_squared(m_context.camera->block_pos(), m_last_camera_pos) > 32 * 32;
        if (reiterate)
            m_last_camera_pos = m_context.camera->block_pos();
        glm::i32vec3 camera_chunk_pos = util::signed_i32vec3_divide(m_context.camera->block_pos(), 32).first;

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

        if (!m_chunks_to_mesh_urgent.empty()) {
            while (true) {
                if (m_chunks_to_mesh_urgent.empty())
                    break;
                
                auto chunk = m_chunks_to_mesh_urgent.back();
                m_mesh_builder->m_current = chunk;
                m_mesh_builder->build();
                allocate_chunk_mesh(chunk, m_mesh_builder->m_vertices, m_mesh_builder->m_indices);
                m_chunks_meshed.push_back(chunk);
                m_chunks_to_mesh_urgent.pop_back();
            }
        }

        for (auto chunk : m_chunks_meshed) {
            if (chunk->m_vertex_buffer == nullptr)
                continue;

            glm::vec3 min = glm::vec3(chunk->m_chunk_pos) * 32.0f;
            glm::vec3 max = glm::vec3(chunk->m_chunk_pos + glm::i32vec3(1)) * 32.0f;
            if (!m_context.camera->m_frustum.is_box_visible(min, max))
                continue;

            usize vb_offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &chunk->m_vertex_buffer, &vb_offset);
            vkCmdBindIndexBuffer(cmd, chunk->m_index_buffer, 0, VK_INDEX_TYPE_UINT32);
        
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(chunk->m_chunk_pos.x * 32, chunk->m_chunk_pos.y * 32, chunk->m_chunk_pos.z * 32));

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
        ImGui::End();
    }
}
