#include <format>

#include "gl45_render_context.hpp"
#include "tracy/Tracy.hpp"
#include "tracy/TracyOpenGL.hpp"

namespace stardraw::gl45
{
    inline GLenum gl_draw_mode(const draw_mode mode)
    {
        switch (mode)
        {
            case draw_mode::TRIANGLES: return GL_TRIANGLES;
            case draw_mode::TRIANGLE_FAN: return GL_TRIANGLE_FAN;
            case draw_mode::TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
        }

        return -1;
    }

    inline GLenum gl_index_size(const draw_indexed_index_type type)
    {
        switch (type)
        {
            case draw_indexed_index_type::UINT_32: return GL_UNSIGNED_INT;
            case draw_indexed_index_type::UINT_16: return GL_UNSIGNED_SHORT;
            case draw_indexed_index_type::UINT_8: return GL_UNSIGNED_BYTE;
        }

        return -1;
    }

    inline uint32_t gl_type_size(const GLenum type)
    {
        switch (type)
        {
            case GL_INT: return sizeof(GLint);
            case GL_SHORT: return sizeof(GLshort);
            case GL_BYTE: return sizeof(GLbyte);
            case GL_UNSIGNED_INT: return sizeof(GLuint);
            case GL_UNSIGNED_SHORT: return sizeof(GLushort);
            case GL_UNSIGNED_BYTE: return sizeof(GLubyte);
            case GL_FLOAT: return sizeof(GLfloat);
            case GL_DOUBLE: return sizeof(GLdouble);
            default: return -1;
        }
    }

    inline GLenum gl_buffer_attach_point(const buffer_attachment_type attachment)
    {
        switch (attachment)
        {
            case buffer_attachment_type::SHADER_STORAGE_BLOCK: return GL_SHADER_STORAGE_BUFFER;
            case buffer_attachment_type::SHADER_UNIFORM_BLOCK: return GL_UNIFORM_BUFFER;
            case buffer_attachment_type::SHADER_ATOMIC_COUNTER_BLOCK: return GL_ATOMIC_COUNTER_BUFFER;
        }

        return -1;
    }

    inline void gl_set_flag(const GLenum flag, const bool enable, const GLuint index = 0)
    {
        if (enable)
        {
            glEnablei(flag, index);
        }
        else
        {
            glDisablei(flag, index);
        }
    }

    inline GLenum gl_face_cull_mode(const face_cull_mode& mode)
    {
        switch (mode)
        {
            case BACK: return GL_BACK;
            case FRONT: return GL_FRONT;
            case BOTH: return GL_FRONT_AND_BACK;
            default: return -1;
        }
    }

    inline GLenum gl_depth_test_func(const depth_test_func& func)
    {
        switch (func)
        {
            case depth_test_func::ALWAYS: return GL_ALWAYS;
            case depth_test_func::NEVER: return GL_NEVER;
            case depth_test_func::LESS: return GL_LESS;
            case depth_test_func::LESS_EQUAL: return GL_LEQUAL;
            case depth_test_func::GREATER: return GL_GREATER;
            case depth_test_func::GREATER_EQUAL: return GL_GEQUAL;
            case depth_test_func::EQUAL: return GL_EQUAL;
            case depth_test_func::NOT_EQUAL: return GL_NOTEQUAL;
            default: return -1;
        }
    }

    inline GLbitfield gl_clear_mask(const clear_window_mode& mode)
    {
        switch (mode)
        {
            case clear_window_mode::COLOR: return GL_COLOR_BUFFER_BIT;
            case clear_window_mode::DEPTH: return GL_DEPTH_BUFFER_BIT;
            case clear_window_mode::STENCIL: return GL_STENCIL_BUFFER_BIT;
            case clear_window_mode::COLOR_AND_DEPTH: return GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
            case clear_window_mode::COLOR_AND_STENCIL: return GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
            case clear_window_mode::DEPTH_AND_STENCIL: return GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
            case clear_window_mode::ALL: return GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
        }
        return -1;
    }

    status gl45_render_context::execute_draw_cmd(const draw_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute draw cmd");

        GLsizeiptr index_offset_unused;
        status bind_result = bind_vertex_specification_state(cmd->vertex_specification_source, index_offset_unused, false);
        if (is_status_error(bind_result)) return bind_result;

        glDrawArraysInstancedBaseInstance(gl_draw_mode(cmd->mode), cmd->start_vertex, cmd->count, cmd->instances, cmd->start_instance);
        return status_type::SUCCESS;
    }

    status gl45_render_context::execute_draw_indexed(const draw_indexed_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute draw indexed cmd");

        GLsizeiptr index_offset;
        status bind_result = bind_vertex_specification_state(cmd->vertex_specification_source, index_offset, true);
        if (is_status_error(bind_result)) return bind_result;

        const GLenum index_element_type = gl_index_size(cmd->index_type);
        const uint32_t index_element_size = gl_type_size(index_element_type);

        glDrawElementsInstancedBaseVertexBaseInstance(gl_draw_mode(cmd->mode), cmd->count, index_element_type, reinterpret_cast<const void*>(cmd->start_index * index_element_size + index_offset), cmd->instances, cmd->vertex_index_offset, cmd->start_instance);

        return status_type::SUCCESS;
    }

    status gl45_render_context::execute_draw_indirect(const draw_indirect_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute draw indirect cmd");

        GLsizeiptr index_offset_unused;
        status bind_result = bind_vertex_specification_state(cmd->vertex_specification_source, index_offset_unused, false);
        if (is_status_error(bind_result)) return bind_result;

        glMultiDrawArraysIndirect(gl_draw_mode(cmd->mode), reinterpret_cast<const void*>(cmd->indirect_source_offset * sizeof(draw_arrays_indirect_params)), cmd->draw_count, 0);
        return status_type::SUCCESS;
    }

    status gl45_render_context::execute_draw_indexed_indirect(const draw_indexed_indirect_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute draw indirect cmd");

        GLsizeiptr index_offset_unused;
        status bind_result = bind_vertex_specification_state(cmd->vertex_specification_source, index_offset_unused, true);
        if (is_status_error(bind_result)) return bind_result;

        //NOTE: will not work properly if a streaming buffer is used for indices

        const GLenum index_element_type = gl_index_size(cmd->index_type);

        glMultiDrawElementsIndirect(gl_draw_mode(cmd->mode), index_element_type, reinterpret_cast<const void*>(cmd->indirect_source_offset * sizeof(draw_elements_indirect_params)), cmd->draw_count, 0);
        return status_type::SUCCESS;
    }

    status gl45_render_context::execute_buffer_upload(const buffer_upload_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute buffer upload cmd");

        buffer_state* buffer_state = find_gl_buffer_state(cmd->buffer_identifier);
        if (buffer_state == nullptr) return { status_type::UNKNOWN_SOURCE, std::format("No buffer with name '{0}' in pipeline", cmd->buffer_identifier.name) };
        if (!buffer_state->is_valid()) return{ status_type::BROKEN_SOURCE, std::format("Buffer '{0}' is in an invalid state", cmd->buffer_identifier.name) };

        switch (cmd->upload_type)
        {
            case buffer_upload_type::UNSAFE_DIRECT: return buffer_state->upload_data_direct(cmd->upload_address, cmd->upload_data, cmd->upload_bytes);
            case buffer_upload_type::SAFE_STREAMING: return buffer_state->upload_data_staged(cmd->upload_address, cmd->upload_data, cmd->upload_bytes);
            case buffer_upload_type::SAFE_ONE_TIME: return buffer_state->upload_data_temp_copy(cmd->upload_address, cmd->upload_data, cmd->upload_bytes);
        }

        return  { status_type::UNSUPPORTED, "Uknowmn buffer upload type" };
    }

    status gl45_render_context::execute_buffer_copy(const buffer_copy_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute buffer copy cmd");

        const buffer_state* source_state = find_gl_buffer_state(cmd->source_identifier);
        if (source_state == nullptr) return { status_type::UNKNOWN_SOURCE, std::format("No buffer with name '{0}' in pipeline", cmd->source_identifier.name) };
        if (!source_state->is_valid()) return{ status_type::BROKEN_SOURCE, std::format("Buffer '{0}' is in an invalid state", cmd->source_identifier.name) };

        const buffer_state* dest_state = find_gl_buffer_state(cmd->dest_identifier);
        if (dest_state == nullptr) return { status_type::UNKNOWN_SOURCE, std::format("No buffer with name '{0}' in pipeline", cmd->dest_identifier.name) };
        if (!dest_state->is_valid()) return{ status_type::BROKEN_SOURCE, std::format("Buffer '{0}' is in an invalid state", cmd->dest_identifier.name) };

        if (!source_state->is_in_buffer_range(cmd->source_address, cmd->bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested copy range is out of range in buffer '{0}'", cmd->source_identifier.name)};
        if (!dest_state->is_in_buffer_range(cmd->dest_address, cmd->bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested copy range is out of range in buffer '{0}'", cmd->dest_identifier.name)};

        return dest_state->copy_data(source_state->gl_id(), cmd->source_address, cmd->dest_address, cmd->bytes);
    }

    status gl45_render_context::execute_buffer_attach(const buffer_attach_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute buffer attach cmd");

        const buffer_state* buffer_state = find_gl_buffer_state(cmd->buffer_identifier);
        if (buffer_state == nullptr) return { status_type::UNKNOWN_SOURCE, std::format("No buffer with name '{0}' in pipeline", cmd->buffer_identifier.name) };
        if (!buffer_state->is_valid()) return{ status_type::BROKEN_SOURCE, std::format("Buffer '{0}' is in an invalid state", cmd->buffer_identifier.name) };

        return buffer_state->bind_to_slot(gl_buffer_attach_point(cmd->attachment_type), cmd->attachment_index);
    }

    inline GLenum gl_blend_factor(const blending_factor factor)
    {
        switch (factor)
        {
            case blending_factor::ZERO: return GL_ZERO;
            case blending_factor::ONE: return GL_ONE;
            case blending_factor::CONSTANT_COLOR: return GL_CONSTANT_COLOR;
            case blending_factor::CONSTANT_ALPHA: return GL_CONSTANT_ALPHA;
            case blending_factor::ONE_MINUS_CONSTANT_COLOR: return GL_ONE_MINUS_CONSTANT_COLOR;
            case blending_factor::ONE_MINUS_CONSTANT_ALPHA: return GL_ONE_MINUS_CONSTANT_ALPHA;
            case blending_factor::SOURCE_COLOR: return GL_SRC_COLOR;
            case blending_factor::DEST_COLOR: return GL_DST_COLOR;
            case blending_factor::ONE_MINUS_SOURCE_COLOR: return GL_ONE_MINUS_SRC_COLOR;
            case blending_factor::ONE_MINUS_DEST_COLOR: return GL_ONE_MINUS_DST_COLOR;
            case blending_factor::SOURCE_ALPHA: return GL_SRC_ALPHA;
            case blending_factor::DEST_ALPHA: return GL_DST_ALPHA;
            case blending_factor::ONE_MINUS_SOURCE_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
            case blending_factor::ONE_MINUS_DEST_ALPHA: return GL_ONE_MINUS_DST_ALPHA;
            case blending_factor::SOURCE_ALPHA_SATURATE: return GL_SRC_ALPHA_SATURATE;
            case blending_factor::SECONDARY_SOURCE_COLOR: return GL_SRC1_COLOR;
            case blending_factor::SECONDARY_SOURCE_ALPHA: return GL_SRC1_ALPHA;
        }
        return -1;
    }

    inline GLenum gl_blend_func(const blending_func func)
    {
        switch (func)
        {
            case blending_func::ADD: return GL_FUNC_ADD;
            case blending_func::SUBTRACT: return GL_FUNC_SUBTRACT;
            case blending_func::REVERSE_SUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
            case blending_func::MIN: return GL_MIN;
            case blending_func::MAX: return GL_MAX;
        }

        return -1;
    }


    status gl45_render_context::execute_config_blending(const config_blending_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute config blending cmd");
        const blending_config& config = cmd->config;

        gl_set_flag(GL_BLEND, config.enabled);
        if (!config.enabled) return status_type::SUCCESS;

        glBlendColor(config.constant_blend_r, config.constant_blend_g, config.constant_blend_b, config.constant_blend_a);
        glBlendEquationSeparatei(cmd->draw_buffer_index, gl_blend_func(config.rgb_equation), gl_blend_func(config.alpha_equation));
        glBlendFuncSeparatei(cmd->draw_buffer_index, gl_blend_factor(config.source_blend_rgb), gl_blend_factor(config.dest_blend_rgb), gl_blend_factor(config.source_blend_alpha), gl_blend_factor(config.dest_blend_alpha));
        return status_type::SUCCESS;
    }

    inline GLenum gl_stencil_facing(const stencil_facing facing)
    {
        switch (facing)
        {
            case stencil_facing::FRONT: return GL_FRONT;
            case stencil_facing::BACK: return GL_BACK;
            case stencil_facing::BOTH: return GL_FRONT_AND_BACK;
        }

        return -1;
    }

    inline GLenum gl_stencil_test_func(const stencil_test_func test_func)
    {
        switch (test_func)
        {
            case stencil_test_func::ALWAYS: return GL_ALWAYS;
            case stencil_test_func::NEVER: return GL_NEVER;
            case stencil_test_func::LESS: return GL_LESS;
            case stencil_test_func::LESS_EQUAL: return GL_LEQUAL;
            case stencil_test_func::GREATER: return GL_GREATER;
            case stencil_test_func::GREATER_EQUAL: return GL_GEQUAL;
            case stencil_test_func::EQUAL: return GL_EQUAL;
            case stencil_test_func::NOT_EQUAL: return GL_NOTEQUAL;
        }
        return -1;
    }

    inline GLenum gl_stencil_test_op(const stencil_result_op stencil_op)
    {
        switch (stencil_op)
        {
            case stencil_result_op::KEEP: return GL_KEEP;
            case stencil_result_op::ZERO: return GL_ZERO;
            case stencil_result_op::REPLACE: return GL_REPLACE;
            case stencil_result_op::INCREMENT: return GL_INCR;
            case stencil_result_op::INCREMENT_WRAP: return GL_INCR_WRAP;
            case stencil_result_op::DECREMENT: return GL_DECR;
            case stencil_result_op::DECREMENT_WRAP: return GL_DECR_WRAP;
            case stencil_result_op::INVERT: return GL_INVERT;
        }
        return -1;
    }

    status gl45_render_context::execute_config_stencil(const config_stencil_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute config stencil cmd");
        const stencil_config& config = cmd->config;

        gl_set_flag(GL_STENCIL_TEST, config.enabled);
        if (!config.enabled) return status_type::SUCCESS;

        const GLenum gl_facing = gl_stencil_facing(cmd->for_facing);
        glStencilFuncSeparate(gl_facing, gl_stencil_test_func(config.test_func), config.reference, config.test_mask);
        glStencilMaskSeparate(gl_facing, config.write_mask);
        glStencilOpSeparate(gl_facing, gl_stencil_test_op(config.stencil_fail_op), gl_stencil_test_op(config.depth_fail_op), gl_stencil_test_op(config.pixel_pass_op));
        return status_type::SUCCESS;
    }

    status gl45_render_context::execute_config_scissor(const config_scissor_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute config scissor cmd");
        const scissor_test_config& config = cmd->config;

        gl_set_flag(GL_STENCIL_TEST, config.enabled, cmd->viewport_index);
        if (!config.enabled) return status_type::SUCCESS;

        glScissorIndexed(cmd->viewport_index, config.left, config.bottom, config.width, config.height);
        return status_type::SUCCESS;
    }

    status gl45_render_context::execute_config_face_cull(const config_face_cull_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute config face cull cmd");

        if (cmd->mode == face_cull_mode::DISABLED)
        {
            gl_set_flag(GL_CULL_FACE, false);
            return status_type::SUCCESS;
        }

        gl_set_flag(GL_CULL_FACE, true);
        glCullFace(gl_face_cull_mode(cmd->mode));

        return status_type::SUCCESS;
    }

    status gl45_render_context::execute_config_depth_test(const config_depth_test_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute config depth test cmd");
        const depth_test_config& config = cmd->config;

        gl_set_flag(GL_DEPTH_TEST, config.enabled);
        if (!config.enabled) return status_type::SUCCESS;

        glDepthFunc(gl_depth_test_func(config.test_func));
        glDepthMask(config.enable_depth_write);
        return status_type::SUCCESS;
    }

    status gl45_render_context::execute_config_depth_range(const config_depth_range_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute config depth range cmd");

        glDepthRangeIndexed(cmd->viewport_index, cmd->near, cmd->far);
        return status_type::SUCCESS;
    }

    status gl45_render_context::execute_clear_window(const clear_window_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute clear window cmd");

        const clear_values_config& config = cmd->config;
        glClearColor(config.color_r, config.color_g, config.color_b, config.color_a);
        glClearDepth(config.depth);
        glClearStencil(config.stencil);
        glClear(gl_clear_mask(cmd->mode));

        return status_type::SUCCESS;
    }

}
