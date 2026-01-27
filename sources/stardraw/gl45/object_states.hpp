#pragma once
#include <array>
#include <queue>
#include <unordered_map>

#include "types.hpp"
#include "glad/glad.h"

namespace stardraw::gl45
{
    class buffer_state final : public object_state
    {
    public:
        explicit buffer_state(const buffer_descriptor& desc);
        ~buffer_state() override;

        [[nodiscard]] descriptor_type object_type() const override;

        [[nodiscard]] bool is_valid() const;

        [[nodiscard]] status bind_to(const GLenum target) const;
        [[nodiscard]] status bind_to_slot(const GLenum target, const GLuint slot) const;
        [[nodiscard]] status bind_to_slot(const GLenum target, const GLuint slot, const GLintptr address, const GLsizeiptr bytes) const;

        [[nodiscard]] status upload_data_direct(const GLintptr address, const void* const data, const GLintptr bytes);
        [[nodiscard]] status upload_data_staged(const GLintptr address, const void* const data, const GLintptr bytes);
        [[nodiscard]] status upload_data_temp_copy(const GLintptr address, const void* const data, const GLintptr bytes) const;

        [[nodiscard]] status copy_data(const GLuint source_buffer_id, const GLintptr read_address, const GLintptr write_address, const GLintptr bytes) const;

        [[nodiscard]] GLsizeiptr get_size() const;
        [[nodiscard]] bool is_in_buffer_range(const GLintptr address, const GLsizeiptr size) const;
        [[nodiscard]] GLuint gl_id() const;

    private:
        struct upload_chunk
        {
            GLintptr address;
            GLintptr size;
            GLsync fence;
        };


        [[nodiscard]] status prepare_staging_buffer(const GLsizeiptr size);
        void update_staging_buffer_space();

        [[nodiscard]] status map_main_buffer();

        GLuint main_buffer_id = 0;
        GLsizeiptr main_buffer_size = 0;
        void* main_buff_pointer = nullptr;

        GLuint staging_buffer_id = 0;
        GLsizeiptr staging_buffer_size = 0;
        void* staging_buff_pointer = nullptr;

        GLsizeiptr current_staging_buff_address = 0;
        GLsizeiptr remaining_staging_buffer_space = 0;
        std::queue<upload_chunk> upload_chunks;

        std::string buffer_name;
    };

    class vertex_specification_state final : public object_state
    {
    public:

        explicit vertex_specification_state();
        ~vertex_specification_state() override;

        [[nodiscard]] bool is_valid() const;

        [[nodiscard]] status bind() const;
        [[nodiscard]] status attach_vertex_buffer(const GLuint slot, const GLuint id, const GLintptr offset, const GLsizei stride);
        [[nodiscard]] status attach_index_buffer(GLuint index_buffer_id);

        [[nodiscard]] descriptor_type object_type() const override
        {
            return descriptor_type::VERTEX_SPECIFICATION;
        }

        std::vector<GLuint> vertex_buffers;
        GLuint index_buffer = 0;
        GLuint vertex_array_id = 0;
    };
}
