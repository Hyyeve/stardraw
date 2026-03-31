#include "../api/memory_transfer.hpp"
namespace stardraw
{
    using namespace starlib_stdint;
    void* layout_memory(const memory_layout_info& layout, const void* data, const u64 data_size)
    {
        if (data == nullptr) return nullptr;

        const u64 element_count = data_size / layout.packed_size;
        const u8* in_bytes = static_cast<const u8*>(data);
        u8* output = static_cast<u8*>(malloc(layout.padded_size * element_count));
        if (layout.padded_size == layout.packed_size)
        {
            memcpy(output, in_bytes, layout.padded_size * element_count);
            return output;
        }

        for (u64 idx = 0; idx < element_count; idx++)
        {
            const u64 base_read_address = layout.packed_size * idx;
            const u64 base_write_address = layout.padded_size * idx;

            u64 current_write_offset = 0;
            u64 current_read_offset = 0;

            for (const memory_layout_info::pad& pad : layout.pads)
            {
                const u64 size_to_pad_start = pad.address - current_write_offset;
                memcpy(output + base_write_address + current_write_offset, in_bytes + base_read_address + current_read_offset, size_to_pad_start);
                current_write_offset = pad.address + pad.size;
                current_read_offset += size_to_pad_start;
            }
        }

        return output;
    }
}