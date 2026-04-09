#include "vertex_specification_state.hpp"

#include "stardraw/internal/internal.hpp"
#include "tracy/Tracy.hpp"
#include "tracy/TracyOpenGL.hpp"

namespace stardraw::gl45
{
    vertex_specification_state::vertex_specification_state()
    {
        ZoneScoped;
        {
            ZoneScopedN("GL calls");
            glCreateVertexArrays(1, &vertex_array_id);
        }
    }

    vertex_specification_state::~vertex_specification_state()
    {
        ZoneScoped;
        {
            ZoneScopedN("GL calls");
            glDeleteVertexArrays(1, &vertex_array_id);
        }
    }

    bool vertex_specification_state::is_valid() const
    {
        ZoneScoped;
        if (vertex_array_id == 0) return false;
        for (const vertex_buffer_binding& buffer : vertex_buffers)
        {
            {
                ZoneScopedN("GL calls");
                if (!glIsBuffer(buffer.id)) return false;
            }
        }

        if (has_index_buffer && !glIsBuffer(index_buffer.id)) return false;

        return true;
    }

    status vertex_specification_state::bind() const
    {
        ZoneScoped;
        {
            ZoneScopedN("GL calls");
            glBindVertexArray(vertex_array_id);
        }
        return status_type::SUCCESS;
    }

    status vertex_specification_state::attach_vertex_buffer(const object_identifier& identifier, const GLuint slot, const GLuint id, const GLintptr offset, const GLsizei stride)
    {
        ZoneScoped;
        {
            ZoneScopedN("GL calls");
            glVertexArrayVertexBuffer(vertex_array_id, slot, id, offset, stride);
        }
        vertex_buffers.push_back({identifier, id});
        return status_type::SUCCESS;
    }

    status vertex_specification_state::attach_index_buffer(const object_identifier& identifier, const GLuint index_buffer_id)
    {
        ZoneScoped;
        {
            ZoneScopedN("GL calls");
            glVertexArrayElementBuffer(vertex_array_id, index_buffer_id);
        }
        has_index_buffer = true;
        index_buffer = {identifier, index_buffer_id};
        return status_type::SUCCESS;
    }
}
