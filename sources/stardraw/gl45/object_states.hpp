#pragma once
#include <format>
#include <queue>
#include <unordered_map>

#include "types.hpp"
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

    class shader_state final : public object_state
    {
    public:
        explicit shader_state(const shader_descriptor& desc, status& out_status);

        ~shader_state() override;

        [[nodiscard]] bool is_valid() const;

        [[nodiscard]] status make_active() const;
        [[nodiscard]] status make_shader_cache(void** cache_ptr, uint64_t& cache_size) const;

        [[nodiscard]] descriptor_type object_type() const override;

    private:

        struct cache_header
        {
            GLenum format;
        };

        [[nodiscard]] status create_from_stages(const std::vector<shader_stage>& stages);
        [[nodiscard]] status create_from_cache(const void* data, const uint64_t data_size);

        [[nodiscard]] static status store_cache(const GLuint program_id, void** out_data, uint64_t& out_data_size);

        [[nodiscard]] static GLenum gl_shader_type(shader_stage_type stage);

        [[nodiscard]] static status link_shader(const std::vector<GLuint>& stages, GLuint& out_shader_id);
        [[nodiscard]] static status compile_shader_stage(const std::string& source, const GLuint type, GLuint& out_shader_id);

        [[nodiscard]] static status validate_program(const GLuint program);

        [[nodiscard]] static std::string get_shader_log(const GLuint shader);
        [[nodiscard]] static std::string get_program_log(const GLuint program);

        GLuint shader_program_id = 0;
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

    class shader_specification_state final : public object_state
    {
    public:
        [[nodiscard]] descriptor_type object_type() const override
        {
            return descriptor_type::SHADER_SPECIFICATION;
        }
    };

    class draw_specification_state final : public object_state
    {
    public:
        [[nodiscard]] descriptor_type object_type() const override
        {
            return descriptor_type::DRAW_SPECIFICATION;
        }
    };
}
