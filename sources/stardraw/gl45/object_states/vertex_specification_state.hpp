#pragma once
#include "../common.hpp"
#include "../gl_headers.hpp"
namespace stardraw::gl45
{
    class vertex_specification_state final : public object_state
    {
    public:

        struct vertex_buffer_binding
        {
            object_identifier identifier;
            GLuint id;
        };

        explicit vertex_specification_state();
        ~vertex_specification_state() override;

        [[nodiscard]] bool is_valid() const;

        [[nodiscard]] status bind() const;
        [[nodiscard]] status attach_vertex_buffer(const object_identifier& identifier, const GLuint slot, const GLuint id, const GLintptr offset, const GLsizei stride);
        [[nodiscard]] status attach_index_buffer(const object_identifier& identifier, GLuint index_buffer_id);

        [[nodiscard]] descriptor_type object_type() const override
        {
            return descriptor_type::VERTEX_CONFIGURATION;
        }

        std::vector<vertex_buffer_binding> vertex_buffers;
        vertex_buffer_binding index_buffer = {};
        bool has_index_buffer = false;
        GLuint vertex_array_id = 0;
    };
}
