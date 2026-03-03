#pragma once
#include <format>
#include <queue>
#include <unordered_map>

#include "staging_buffer_uploader.hpp"
#include "types.hpp"
#include "glad/glad.h"
#include "stardraw/api/commands.hpp"
#include "stardraw/api/memory_transfer.hpp"

template <>
struct std::hash<stardraw::shader_parameter_location>
{
    std::size_t operator()(const stardraw::shader_parameter_location& key) const noexcept
    {
        return hash<uint32_t>()(key.binding_range_index + key.binding_range + key.byte_address + key.root_idx);
    }
};

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

        std::string buffer_name;
    };

    class shader_state final : public object_state
    {
    public:
        struct binding_block_location
        {
            GLenum type; //buffer type: SSBO, UBO, etc
            GLuint slot;
        };

        explicit shader_state(const shader_descriptor& desc, status& out_status);
        ~shader_state() override;

        [[nodiscard]] bool is_valid() const;

        [[nodiscard]] status make_active() const;
        [[nodiscard]] status upload_parameter(const shader_parameter& parameter);
        void clear_parameters();
        [[nodiscard]] descriptor_type object_type() const override;

        std::vector<uint32_t> descriptor_set_binding_offsets;
        std::vector<shader_parameter> parameter_store;
        std::unordered_map<uint32_t, std::string> bound_objects;
    private:
        [[nodiscard]] status create_from_stages(const std::vector<shader_stage>& stages);

        [[nodiscard]] static GLenum gl_shader_type(shader_stage_type stage);
        [[nodiscard]] status remap_spirv_stages(const std::vector<shader_stage>& stages, std::vector<std::string>& out_sources);

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
        [[nodiscard]] bool has_index_buffer() const;

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

    class draw_specification_state final : public object_state
    {
    public:
        explicit draw_specification_state(const draw_specification_descriptor& descriptor, bool has_index_buffer);
        [[nodiscard]] descriptor_type object_type() const override;

        object_identifier shader;
        object_identifier vertex_specification;
        bool has_index_buffer;
    };

    class texture_state final : public object_state
    {
    public:
        explicit texture_state(const texture_descriptor& desc, status& out_status);
        explicit texture_state(const texture_state* original, const texture_descriptor& desc, status& out_status);
        ~texture_state() override;

        [[nodiscard]] bool is_valid() const;
        [[nodiscard]] status is_view_compatible(const texture_descriptor& view_descriptor) const;
        [[nodiscard]] status set_sampling_config(const texture_sampling_conifg& config) const;

        [[nodiscard]] descriptor_type object_type() const override
        {
            return descriptor_type::TEXTURE;
        }

    private:
        GLuint texture_id = 0;
        uint32_t num_texture_mipmap_levels;
        uint32_t num_texture_array_layers;
        GLenum gl_texture_format;
        GLenum gl_texture_target;
    };
}
