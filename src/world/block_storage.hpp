#pragma once

#include "../block/block.hpp"

namespace world {
    // based on https://www.reddit.com/r/VoxelGameDev/comments/9yu8qy/palettebased_compression_for_chunked_discrete/
    class BlockStorage {
        struct PaletteEntry {
            u32 ref_count = 0;
            block::NID type;
        };

        int m_size;
        u8 *m_data;
        PaletteEntry m_palette;
        int m_palette_count;
        int m_indices_length;
        
        int new_palette_entry();
        void grow_palette();
    
    public:
        BlockStorage(int size);
        
        void set_block(index, type);
        block::NID get_block(index);
        
        // Shrink the palette (and thus the BitBuffer) every now and then.
        // You may need to apply heuristics to determine when to do this.
        void fit_palette();
    };
}
