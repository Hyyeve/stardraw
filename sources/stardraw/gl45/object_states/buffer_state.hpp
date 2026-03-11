#pragma once
#include "../common.hpp"
#include "../staging_buffer_uploader.hpp"
#include "glad/glad.h"
namespace stardraw::gl45
{
    class buffer_state final : public object_state
    {
    public:
        explicit buffer_state(const buffer_descriptor& desc, status& out_status);
        ~buffer_state() override;

        [[nodiscard]] descriptor_type object_type() const override;

        [[nodiscard]] bool is_valid() const;

        [[nodiscard]] status bind_to(const GLenum target) const;
        [[nodiscard]] status bind_to_slot(const GLenum target, const GLuint slot) const;
        [[nodiscard]] status bind_to_slot(const GLenum target, const GLuint slot, const GLintptr address, const GLsizeiptr bytes) const;

        [[nodiscard]] status prepare_upload_data_streaming(const GLintptr address, const GLintptr bytes, memory_transfer_handle** out_handle);
        [[nodiscard]] status flush_upload_data_streaming(memory_transfer_handle* handle) const;

        [[nodiscard]] status prepare_upload_data_chunked(const GLintptr address, const GLintptr bytes, memory_transfer_handle** out_handle);
        [[nodiscard]] status flush_upload_data_chunked(memory_transfer_handle* handle) const;

        [[nodiscard]] status prepare_upload_data_unchecked(const GLintptr address, const GLintptr bytes, memory_transfer_handle** out_handle);
        [[nodiscard]] static status flush_upload_data_unchecked(const memory_transfer_handle* handle);

        [[nodiscard]] status copy_data(const GLuint source_buffer_id, const GLintptr read_address, const GLintptr write_address, const GLintptr bytes) const;

        [[nodiscard]] GLsizeiptr get_size() const;
        [[nodiscard]] bool is_in_buffer_range(const GLintptr address, const GLsizeiptr size) const;
        [[nodiscard]] GLuint gl_id() const;

    private:
        enum class upload_chunk_state
        {
            RESERVED, TRANSFERRING
        };

        [[nodiscard]] status map_main_buffer();

        GLuint main_buffer_id = 0;
        GLsizeiptr main_buffer_size = 0;
        void* main_buff_pointer = nullptr;

        staging_buffer_uploader staging_uploader;

        object_identifier buffer_identifier;
    };
}
