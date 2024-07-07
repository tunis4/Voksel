#pragma once

#include "common.hpp"

class Chunk;

namespace renderer {
    struct ChunkMesh {
        Chunk *chunk;
        VkDeviceSize face_buffer_offset = 0;
        VkDeviceSize transparent_vertex_buffer_offset = 0;
        VkDeviceSize transparent_index_buffer_offset = 0;
        VkDeviceSize chunk_info_buffer_offset = 0;
        VmaVirtualAllocation face_buffer_allocation = nullptr;
        VmaVirtualAllocation transparent_vertex_buffer_allocation = nullptr;
        VmaVirtualAllocation transparent_index_buffer_allocation = nullptr;
        VmaVirtualAllocation chunk_info_buffer_allocation = nullptr;
        usize num_faces = 0;
        usize num_transparent_vertices = 0;
        usize num_transparent_indices = 0;

        ChunkMesh(Chunk *chunk) : chunk(chunk) {}

        usize used_vram() const;
    };
};
