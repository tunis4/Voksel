#include "block_storage.hpp"

#include <cstring>
#include <algorithm>

namespace world {
    BlockStorage::BlockStorage(uint size) {
        m_size = size;
        m_bits_per_index = 0;
        m_palette_size = 1;
        m_palette.resize(1 << m_bits_per_index);
        m_palette[0] = { size, 0 };
        m_data.single = &m_palette[0];
    }

    BlockStorage::~BlockStorage() {
        if (m_palette_size > 1)
            PackedArray_destroy(m_data.packed);
    }
    
    void BlockStorage::set_block(uint index, block::NID type) {
        if (m_bits_per_index == 0) {
            if (type == m_data.single->type) return;
            m_data.single->ref_count--;
            uint new_entry = new_palette_entry();
            m_palette[new_entry] = { 1, type };
            PackedArray_set(m_data.packed, index, new_entry);
            m_palette_size++;
            return;
        }
        
        uint palette_index = PackedArray_get(m_data.packed, index);
        PaletteEntry *current = &m_palette[palette_index];

        current->ref_count--;
        
        // is the block type already in the palette?
        auto replace = std::find_if(m_palette.begin(), m_palette.end(), [&] (PaletteEntry &entry) { return entry.type == type; });
        if (replace != m_palette.end()) {
            // then use the existing palette entry
            PackedArray_set(m_data.packed, index, replace - m_palette.begin());
            replace->ref_count++;
            return;
        }
        
        // can we overwrite the current palette entry?
        if (current->ref_count == 0) {
            current->type = type;
            current->ref_count = 1;
            return;
        }
        
        // get the first free palette entry, possibly growing the palette
        uint new_entry = new_palette_entry();
        
        m_palette[new_entry] = { 1, type };
        PackedArray_set(m_data.packed, index, new_entry);
        m_palette_size++;
    }

    block::NID BlockStorage::get_block(uint index) {
        if (m_bits_per_index == 0)
            return m_data.single->type;
        
        uint palette_index = PackedArray_get(m_data.packed, index);
        return m_palette[palette_index].type;
    }
    
    uint BlockStorage::new_palette_entry() {
        if (m_bits_per_index == 0) {
            grow_palette();
            return new_palette_entry();
        }

        auto first_free = std::find_if(m_palette.begin(), m_palette.end(), [&] (PaletteEntry &entry) { return entry.ref_count == 0; });
        if (first_free != m_palette.end())
            return first_free - m_palette.begin();

        // if no free entry, then grow the palette and the packed array
        grow_palette();
        
        // try again now
        return new_palette_entry();
    }

    void BlockStorage::grow_palette() {
        if (m_bits_per_index == 0) {
            m_bits_per_index++;
            m_palette.resize(1 << m_bits_per_index);

            u32 *zero = new u32[m_size];
            std::memset(zero, 0, m_size * sizeof(u32));
            m_data.packed = PackedArray_create(m_bits_per_index, m_size);
            PackedArray_pack(m_data.packed, 0, zero, m_size);
            delete[] zero;
            return;
        }

        u32 *unpacked = new u32[m_size];
        PackedArray_unpack(m_data.packed, 0, unpacked, m_size);
        PackedArray_destroy(m_data.packed);
        
        // double the palette size
        m_bits_per_index++;
        m_palette.resize(1 << m_bits_per_index);

        m_data.packed = PackedArray_create(m_bits_per_index, m_size);
        PackedArray_pack(m_data.packed, 0, unpacked, m_size);
        delete[] unpacked;
    }
    
    void BlockStorage::fit_palette() {
        // // Remove old entries...
        // for(int i = 0; i < palette.length; i++) {
        //     if(palette[i].refcount == 0) {
        //         palette[i] = null;
        //         paletteCount -= 1;
        //     }
        // }
        
        // // Is the palette less than half of its closest power-of-two?
        // if(paletteCount > powerOfTwo(paletteCount)/2) {
        //     // NO: The palette cannot be shrunk!
        //     return;
        // }
        
        // // decode all indices
        // int[] indices = new int[size];
        // for(int i = 0; i < indices.length; i++) {
        //     indices[i] = data.get(i * indicesLength, indicesLength);
        // }
        
        // // Create new palette, halfing it in size
        // indicesLength = indicesLength >> 1;
        // PaletteEntry[] newPalette = new PaletteEntry[2 pow indicesLength];
        
        // // We gotta compress the palette entries!
        // int paletteCounter = 0;
        // for(int pi = 0; pi < palette.length; pi++, paletteCounter++) {
        //     PaletteEntry entry = newPalette[paletteCounter] = palette[pi];
            
        //     // Re-encode the indices (find and replace; with limit)
        //     for(int di = 0, fc = 0; di < indices.length && fc < entry.refcount; di++) {
        //         if(pi == indices[di]) {
        //             indices[di] = paletteCounter;
        //             fc += 1;
        //         }
        //     }
        // }
        
        // // Allocate new BitBuffer
        // data = new BitBuffer(size * indicesLength); // the length is in bits, not bytes!
        
        // // Encode the indices
        // for(int i = 0; i < indices.length; i++) {
        //     data.set(i * indicesLength, indicesLength, indices[i]);
        // }
    }
}
