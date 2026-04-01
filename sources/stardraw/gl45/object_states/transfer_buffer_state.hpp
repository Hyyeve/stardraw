#pragma once
#include "../common.hpp"
#include "../gl_headers.hpp"
#include "starlib/types/block_allocator.hpp"


namespace stardraw::gl45
{
    class transfer_buffer_state final : public object_state
    {
    public:
        explicit transfer_buffer_state(const transfer_buffer& descriptor, status& out_status);

        [[nodiscard]] status allocate_upload(const u64 address, const u64 bytes, gl_memory_transfer_handle** out_handle);
        [[nodiscard]] u64 get_available_space() const;
        [[nodiscard]] u64 get_buffer_size() const;
        static status flush_upload(const gl_memory_transfer_handle* handle);
        ~transfer_buffer_state() override;

        [[nodiscard]] descriptor_type object_type() const override;
    private:

        struct upload_chunk
        {
            u64 address = 0;
            GLuint staging_buffer_id = 0;
            GLsync fence = nullptr;
        };

        void clean_chunks();
        status allocate_buffer();

        std::vector<upload_chunk*> chunks = {};
        starlib::block_allocator chunk_allocator = starlib::block_allocator(0);
        std::unordered_map<GLuint, u32> buffer_refcounts = {};
        GLuint current_buffer_id = 0;
        GLbyte* current_buffer_ptr = nullptr;
        u64 buffer_size = 0;
        transfer_buffer_usage usage;
        object_identifier buffer_id;
    };
}
