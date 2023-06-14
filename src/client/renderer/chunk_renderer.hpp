#pragma once

#include "common.hpp"
#include "pipeline.hpp"
#include "texture.hpp"
#include "../../world/chunk.hpp"
#include <vulkan/vulkan_core.h>

namespace render {
    // structure that ties a chunk with information for rendering it. managed by the ChunkRenderer class
    struct ChunkRender {
        glm::i32vec3 m_chunk_pos;
        Chunk *m_chunk;

        VkBuffer m_vertex_buffer;
        VmaAllocation m_vertex_buffer_allocation;
        VkBuffer m_index_buffer;
        VmaAllocation m_index_buffer_allocation;
        usize m_num_indices;
    };
    
    struct ChunkVertex {
        glm::vec3 m_pos;
        u32 m_tex; // first 30 bits for layer, last 2 bits for index
        f32 m_ao;

        inline ChunkVertex() {};
        inline ChunkVertex(glm::vec3 pos, u32 tex_index, u32 tex_layer, u8 ao) {
            m_pos = pos;
            m_tex = ((tex_index & 0b11) << 30) | (tex_layer & ~((u32)0b11 << 30));
            m_ao = 1.0f / (ao + 1);
        }

        static auto alloc_binding_description() {
            auto d = new VkVertexInputBindingDescription();
            d->binding = 0;
            d->stride = sizeof(ChunkVertex);
            d->inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return d;
        }

        static constexpr usize num_attribute_descriptions = 3; 
        static auto alloc_attribute_descriptions() {
            auto d = new VkVertexInputAttributeDescription[num_attribute_descriptions];
            d[0].binding = 0;
            d[0].location = 0;
            d[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            d[0].offset = offsetof(ChunkVertex, m_pos);

            d[1].binding = 0;
            d[1].location = 1;
            d[1].format = VK_FORMAT_R32_UINT;
            d[1].offset = offsetof(ChunkVertex, m_tex);

            d[2].binding = 0;
            d[2].location = 2;
            d[2].format = VK_FORMAT_R32_SFLOAT;
            d[2].offset = offsetof(ChunkVertex, m_ao);
            return d;
        }
    };

    class ChunkMeshBuilder;
    class ChunkRenderer {
        struct alignas(16) UniformBuffer {
            glm::mat4 view;
            glm::mat4 projection;
        };

        struct alignas(16) PushConstants {
            glm::mat4 model;
        };

        struct PerFrame {
            VkDescriptorSet m_descriptor_set;
            VkBuffer m_uniform_buffer;
            VmaAllocation m_uniform_buffer_allocation;
            void *m_uniform_buffer_mapped;
        };

        std::array<PerFrame, MAX_FRAMES_IN_FLIGHT> m_per_frame;
        Context &m_context;
    
    public:
        VkDescriptorSetLayout m_descriptor_set_layout;
        VkPipeline m_pipeline;
        VkPipelineLayout m_pipeline_layout;
        Texture m_block_texture;
        ChunkMeshBuilder *m_mesh_builder;

        ChunkRenderer(Context &context) : m_context(context), m_block_texture(context) {}

        void init();
        void cleanup();
        void allocate_chunk_mesh(ChunkRender &chunk, const std::vector<ChunkVertex> &vertices, const std::vector<u32> &indices);
        void delete_chunk_mesh(ChunkRender &chunk);
        void record(VkCommandBuffer cmd, uint frame_index);
    };
}
