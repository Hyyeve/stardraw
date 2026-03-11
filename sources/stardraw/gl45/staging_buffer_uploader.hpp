#pragma once
#include "common.hpp"
#include "gl_headers.hpp"
#include "starlib/types/block_allocator.hpp"

namespace stardraw::gl45
{
    class staging_buffer_uploader
    {
    public:
        status allocate_upload(const u64 address, const u64 bytes, const u64 max_staging_buffer_size, gl_memory_transfer_handle** out_handle);
        static status flush_upload(const gl_memory_transfer_handle* handle);
        ~staging_buffer_uploader();
    private:

        struct upload_chunk
        {
            u64 address = 0;
            GLuint staging_buffer_id = 0;
            GLsync fence = nullptr;
        };

        void clean_chunks();
        status allocate_new_staging_buffer(const u64 size);

        std::vector<upload_chunk*> chunks = {};
        starlib::block_allocator chunk_allocator = starlib::block_allocator(0);
        std::unordered_map<GLuint, u32> staging_buffer_refcounts = {};
        GLuint active_staging_buffer_id = 0;
        GLbyte* active_staging_buffer_ptr = nullptr;
        u64 active_staging_buffer_size = 0;
    };
}
