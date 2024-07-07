#pragma once

#include <vector>
#include <PackedArray/PackedArray.h>

#include "../block/block.hpp"
#include "../serial_buffer.hpp"

// based on https://www.reddit.com/r/VoxelGameDev/comments/9yu8qy/palettebased_compression_for_chunked_discrete/
class BlockStorage {
public:
    struct PaletteEntry {
        u32 ref_count;
        BlockNID type;
    };

    union Data {
        PaletteEntry *single;
        PackedArray *packed;
    } m_data;
    uint m_size;
    std::vector<PaletteEntry> m_palette;
    uint m_palette_size;
    uint m_bits_per_index;

    uint new_palette_entry();
    void grow_palette();

    BlockStorage(uint size);
    ~BlockStorage();

    void set_block(uint index, BlockNID type);
    BlockNID get_block(uint index);

    // gotta shrink the palette every now and then
    void fit_palette();

    SerialBuffer& serialize(SerialBuffer &buffer);
    SerialBuffer& deserialize(SerialBuffer &buffer);
};
