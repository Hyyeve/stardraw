#pragma once
#include <slang.h>

namespace stardraw
{
    struct shader_buffer_layout
    {
        struct pad
        {
            uint64_t address;
            uint64_t size;
        };

        uint64_t packed_size = 0;
        uint64_t padded_size = 0;
        std::vector<pad> pads;
    };

    struct shader_program
    {
        void* data;
        uint32_t data_size;
        slang::ShaderReflection* reflection_data;
    };
}
