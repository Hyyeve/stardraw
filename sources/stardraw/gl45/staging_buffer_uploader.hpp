#pragma once
#include "gl_headers.hpp"
#include "types.hpp"
#include "stardraw/api/types.hpp"
#include "stardraw/internal/simple_block_allocator.hpp"

namespace stardraw::gl45
{
    class staging_buffer_uploader
    {
    public:
        status allocate_upload(const uint64_t address, const uint64_t bytes, const uint64_t max_staging_buffer_size, gl_memory_transfer_handle** out_handle);
        static status flush_upload(const gl_memory_transfer_handle* handle);
        ~staging_buffer_uploader();
    private:

        struct upload_chunk
        {
            uint64_t address = 0;
            GLuint staging_buffer_id = 0;
            GLsync fence = nullptr;
        };

        void clean_chunks();
        status allocate_new_staging_buffer(const uint64_t size);

        std::vector<upload_chunk*> chunks = {};
        simple_block_allocator chunk_allocator = simple_block_allocator(0);
        std::unordered_map<GLuint, uint32_t> staging_buffer_refcounts = {};
        GLuint active_staging_buffer_id = 0;
        GLbyte* active_staging_buffer_ptr = nullptr;
        uint64_t active_staging_buffer_size = 0;
    };
}
