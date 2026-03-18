#include <format>

#include "api_conversion.hpp"
#include "render_context.hpp"
#include "tracy/Tracy.hpp"
#include "tracy/TracyOpenGL.hpp"

namespace stardraw::gl45
{
    using namespace starlib_stdint;

    inline void gl_set_flag(const GLenum flag, const bool enable, const GLuint index = 0)
    {
        ZoneScoped;
        if (enable)
        {
            glEnablei(flag, index);
        }
        else
        {
            glDisablei(flag, index);
        }
    }

    status render_context::execute_draw(const draw_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute draw cmd");
        if (active_draw_specification == nullptr) return {status_type::INVALID, "No draw specification is currently active"};

        vertex_specification_state* vertex_spec;
        const status v_find_status = find_vertex_specification_state(active_draw_specification->vertex_specification, &vertex_spec);
        if (v_find_status.is_error()) return v_find_status;

        for (const vertex_specification_state::vertex_buffer_binding& binding : vertex_spec->vertex_buffers) mem_barrier_controller.barrier_if_needed(binding.identifier, GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        shader_state* shader;
        status find_status = find_shader_state(active_draw_specification->shader, &shader);
        if (find_status.is_error()) return find_status;

        shader->barrier_objects_if_needed(mem_barrier_controller);
        glDrawArraysInstancedBaseInstance(to_gl_draw_mode(cmd->mode), cmd->start_vertex, cmd->count, cmd->instances, cmd->start_instance);
        shader->flag_barriers(mem_barrier_controller);
        return status_type::SUCCESS;
    }

    status render_context::execute_draw_indexed(const draw_indexed_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute draw indexed cmd");

        if (active_draw_specification == nullptr) return {status_type::INVALID, "No draw specification is currently active"};
        if (!active_draw_specification->has_index_buffer) return {status_type::INVALID, "The current draw specification does not have an index buffer for indexed drawing"};

        const GLenum index_element_type = to_gl_index_size(cmd->index_type);
        const u32 index_element_size = to_gl_type_size(index_element_type); //Clangd claims this is uninitialized, but it's... not??

        vertex_specification_state* vertex_spec;
        const status v_find_status = find_vertex_specification_state(active_draw_specification->vertex_specification, &vertex_spec);
        if (v_find_status.is_error()) return v_find_status;

        for (const vertex_specification_state::vertex_buffer_binding& binding : vertex_spec->vertex_buffers) mem_barrier_controller.barrier_if_needed(binding.identifier, GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
        mem_barrier_controller.barrier_if_needed(vertex_spec->index_buffer.identifier, GL_ELEMENT_ARRAY_BARRIER_BIT);

        shader_state* shader;
        status find_status = find_shader_state(active_draw_specification->shader, &shader);
        if (find_status.is_error()) return find_status;

        shader->barrier_objects_if_needed(mem_barrier_controller);
        glDrawElementsInstancedBaseVertexBaseInstance(to_gl_draw_mode(cmd->mode), cmd->count, index_element_type, reinterpret_cast<const void*>(cmd->start_index * index_element_size), cmd->instances, cmd->vertex_index_offset, cmd->start_instance);
        shader->flag_barriers(mem_barrier_controller);
        return status_type::SUCCESS;
    }

    status render_context::execute_draw_indirect(const draw_indirect_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute draw indirect cmd");

        if (active_draw_specification == nullptr) return {status_type::INVALID, "No draw specification is currently active"};

        vertex_specification_state* vertex_spec;
        const status v_find_status = find_vertex_specification_state(active_draw_specification->vertex_specification, &vertex_spec);
        if (v_find_status.is_error()) return v_find_status;

        for (const vertex_specification_state::vertex_buffer_binding& binding : vertex_spec->vertex_buffers) mem_barrier_controller.barrier_if_needed(binding.identifier, GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
        mem_barrier_controller.barrier_if_needed(cmd->indirect_buffer, GL_COMMAND_BARRIER_BIT);

        status bind_status = bind_buffer(cmd->indirect_buffer, GL_DRAW_INDIRECT_BUFFER);
        if (bind_status.is_error()) return bind_status;

        shader_state* shader;
        status find_status = find_shader_state(active_draw_specification->shader, &shader);
        if (find_status.is_error()) return find_status;

        shader->barrier_objects_if_needed(mem_barrier_controller);
        glMultiDrawArraysIndirect(to_gl_draw_mode(cmd->mode), reinterpret_cast<const void*>(cmd->indirect_index * sizeof(draw_arrays_indirect_params)), cmd->draw_count, 0);
        shader->flag_barriers(mem_barrier_controller);
        return status_type::SUCCESS;
    }

    status render_context::execute_draw_indexed_indirect(const draw_indexed_indirect_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute draw indirect cmd");

        if (active_draw_specification == nullptr) return {status_type::INVALID, "No draw specification is currently active"};
        if (!active_draw_specification->has_index_buffer) return {status_type::INVALID, "The current draw specification does not have an index buffer for indexed drawing"};

        const GLenum index_element_type = to_gl_index_size(cmd->index_type);

        vertex_specification_state* vertex_spec;
        const status v_find_status = find_vertex_specification_state(active_draw_specification->vertex_specification, &vertex_spec);
        if (v_find_status.is_error()) return v_find_status;

        for (const vertex_specification_state::vertex_buffer_binding& binding : vertex_spec->vertex_buffers) mem_barrier_controller.barrier_if_needed(binding.identifier, GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
        mem_barrier_controller.barrier_if_needed(vertex_spec->index_buffer.identifier, GL_ELEMENT_ARRAY_BARRIER_BIT);
        mem_barrier_controller.barrier_if_needed(cmd->indirect_buffer, GL_COMMAND_BARRIER_BIT);

        status bind_status = bind_buffer(cmd->indirect_buffer, GL_DRAW_INDIRECT_BUFFER);
        if (bind_status.is_error()) return bind_status;

        shader_state* shader;
        status find_status = find_shader_state(active_draw_specification->shader, &shader);
        if (find_status.is_error()) return find_status;

        shader->barrier_objects_if_needed(mem_barrier_controller);
        glMultiDrawElementsIndirect(to_gl_draw_mode(cmd->mode), index_element_type, reinterpret_cast<const void*>(cmd->indirect_index * sizeof(draw_elements_indirect_params)), cmd->draw_count, 0);
        shader->flag_barriers(mem_barrier_controller);

        return status_type::SUCCESS;
    }

    status render_context::execute_buffer_copy(const buffer_copy_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute buffer copy cmd");

        buffer_state* source_state;
        const status find_source_status = find_buffer_state(cmd->read_buffer, &source_state);
        if (find_source_status.is_error()) return find_source_status;

        buffer_state* dest_state;
        const status find_dest_status = find_buffer_state(cmd->write_buffer, &dest_state);
        if (find_dest_status.is_error()) return find_source_status;

        if (!source_state->is_in_buffer_range(cmd->source_address, cmd->bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested copy range is out of range in buffer '{0}'", cmd->read_buffer.name)};
        if (!dest_state->is_in_buffer_range(cmd->dest_address, cmd->bytes)) return {status_type::RANGE_OVERFLOW, std::format("Requested copy range is out of range in buffer '{0}'", cmd->write_buffer.name)};

        mem_barrier_controller.barrier_if_needed(cmd->read_buffer, GL_BUFFER_UPDATE_BARRIER_BIT);
        mem_barrier_controller.barrier_if_needed(cmd->write_buffer, GL_BUFFER_UPDATE_BARRIER_BIT);

        return dest_state->copy_data(source_state->gl_id(), cmd->source_address, cmd->dest_address, cmd->bytes);
    }

    status render_context::execute_texture_copy(const texture_copy_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute texture copy cmd");

        texture_state* source_state;
        const status find_source_status = find_texture_state(cmd->read_texture, &source_state);
        if (find_source_status.is_error()) return find_source_status;

        texture_state* dest_state;
        const status find_dest_status = find_texture_state(cmd->write_texture, &dest_state);
        if (find_dest_status.is_error()) return find_source_status;

        mem_barrier_controller.barrier_if_needed(cmd->read_texture, GL_TEXTURE_UPDATE_BARRIER_BIT);
        mem_barrier_controller.barrier_if_needed(cmd->write_texture, GL_TEXTURE_UPDATE_BARRIER_BIT);

        return dest_state->copy_pixels(source_state, cmd->copy_info);
    }

    status render_context::execute_draw_config(const draw_config_command* cmd)
    {
        ZoneScoped;
        return bind_draw_specification_state(cmd->draw_specification);
    }

    status render_context::execute_config_blending(const blending_config_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute config blending cmd");
        const blending_config& config = cmd->config;

        gl_set_flag(GL_BLEND, config.enabled);
        if (!config.enabled) return status_type::SUCCESS;

        glBlendColor(config.constant_blend_r, config.constant_blend_g, config.constant_blend_b, config.constant_blend_a);
        glBlendEquationSeparatei(cmd->draw_buffer_index, to_gl_blend_func(config.rgb_equation), to_gl_blend_func(config.alpha_equation));
        glBlendFuncSeparatei(cmd->draw_buffer_index, to_gl_blend_factor(config.source_blend_rgb), to_gl_blend_factor(config.dest_blend_rgb), to_gl_blend_factor(config.source_blend_alpha), to_gl_blend_factor(config.dest_blend_alpha));
        return status_type::SUCCESS;
    }

    status render_context::execute_config_stencil(const stencil_config_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute config stencil cmd");
        const stencil_config& config = cmd->config;

        gl_set_flag(GL_STENCIL_TEST, config.enabled);
        if (!config.enabled) return status_type::SUCCESS;

        const GLenum gl_facing = to_gl_stencil_facing(cmd->for_facing);
        glStencilFuncSeparate(gl_facing, to_gl_stencil_test_func(config.test_func), config.reference, config.test_mask);
        glStencilMaskSeparate(gl_facing, config.write_mask);
        glStencilOpSeparate(gl_facing, to_gl_stencil_test_op(config.stencil_fail_op), to_gl_stencil_test_op(config.depth_fail_op), to_gl_stencil_test_op(config.pixel_pass_op));
        return status_type::SUCCESS;
    }

    status render_context::execute_config_scissor(const scissor_config_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute config scissor cmd");
        const scissor_test_config& config = cmd->config;

        gl_set_flag(GL_STENCIL_TEST, config.enabled, cmd->viewport_index);
        if (!config.enabled) return status_type::SUCCESS;

        glScissorIndexed(cmd->viewport_index, config.left, config.bottom, config.width, config.height);
        return status_type::SUCCESS;
    }

    status render_context::execute_config_face_cull(const face_cull_config_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute config face cull cmd");

        if (cmd->mode == face_cull_mode::DISABLED)
        {
            gl_set_flag(GL_CULL_FACE, false);
            return status_type::SUCCESS;
        }

        gl_set_flag(GL_CULL_FACE, true);
        glCullFace(to_gl_face_cull_mode(cmd->mode));

        return status_type::SUCCESS;
    }

    status render_context::execute_config_depth_test(const depth_test_config_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute config depth test cmd");
        const depth_test_config& config = cmd->config;

        gl_set_flag(GL_DEPTH_TEST, config.enabled);
        if (!config.enabled) return status_type::SUCCESS;

        glDepthFunc(to_gl_depth_test_func(config.test_func));
        glDepthMask(config.enable_depth_write);
        return status_type::SUCCESS;
    }

    status render_context::execute_config_depth_range(const depth_range_config_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute config depth range cmd");

        glDepthRangeIndexed(cmd->viewport_index, cmd->near, cmd->far);
        return status_type::SUCCESS;
    }

    status render_context::execute_config_viewports(const configure_viewports_command* cmd) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute config viewport cmd");

        for (u32 idx = 0; idx < cmd->viewports.size(); idx++)
        {
            const viewport_config& config = cmd->viewports[idx];
            glViewportIndexedf(cmd->first_viewport_index + idx, config.x, config.y, config.width, config.height);
        }

        return status_type::SUCCESS;
    }

    status render_context::execute_clear_window(const clear_window_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute clear window cmd");

        const clear_values_config& config = cmd->config;
        glClearColor(config.color_r, config.color_g, config.color_b, config.color_a);
        glClearDepth(config.depth);
        glClearStencil(config.stencil);
        glClear(to_gl_clear_mask(cmd->mode));

        return status_type::SUCCESS;
    }

    status render_context::execute_compute_dispatch(const compute_dispatch_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute compute shader dispatch");

        shader_state* shader;
        status find_status = find_shader_state(cmd->shader, &shader);
        if (find_status.is_error()) return find_status;

        status bind_status = bind_shader(cmd->shader);
        if (bind_status.is_error()) return bind_status;

        shader->barrier_objects_if_needed(mem_barrier_controller);
        status result_status = shader->dispatch_compute(cmd->groups_x, cmd->groups_y, cmd->groups_z);
        shader->flag_barriers(mem_barrier_controller);

        return result_status;
    }

    status render_context::execute_compute_dispatch_indirect(const compute_dispatch_indirect_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute compute shader dispatch indirect");

        buffer_state* buffer;
        status find_buffer_status = find_buffer_state(cmd->indirect_buffer, &buffer);
        if (find_buffer_status.is_error()) return find_buffer_status;

        status bind_buffer_status = buffer->bind_to(GL_DISPATCH_INDIRECT_BUFFER);
        if (bind_buffer_status.is_error()) return bind_buffer_status;

        shader_state* shader;
        status find_status = find_shader_state(cmd->shader, &shader);
        if (find_status.is_error()) return find_status;

        status bind_status = bind_shader(cmd->shader);
        if (bind_status.is_error()) return bind_status;

        shader->barrier_objects_if_needed(mem_barrier_controller);
        status result_status = shader->dispatch_compute_indirect(cmd->indirect_index * sizeof(dispatch_compute_indirect_params));
        shader->flag_barriers(mem_barrier_controller);

        return result_status;
    }

    status render_context::execute_present(const present_command* cmd) const
    {
        //Under OpenGL, there is no specific presentation calls, and presentation is handled entirely by the window refresh.
        //However, we still need to collect performance data and mark the frame boundary.
        ZoneScoped;
        TracyGpuCollect;
        FrameMark;
        return status_type::SUCCESS;
    }

    status render_context::execute_shader_parameters_upload(const shader_config_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute shader parameters upload cmd");

        shader_state* shader;
        status find_status = find_shader_state(cmd->shader, &shader);
        if (find_status.is_error()) return find_status;

        if (cmd->erase_previous) shader->clear_parameters();

        for (const shader_parameter& parameter : cmd->parameters)
        {
            const status write_status = shader->upload_parameter(parameter);
            if (write_status.is_error()) return write_status;
        }

        return status_type::SUCCESS;
    }

    status render_context::execute_signal(const signal_command* cmd)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Execute signal creation cmd");
        if (signals.contains(cmd->signal_name))
        {
            glDeleteSync(signals[cmd->signal_name].sync_point);
        }

        signals[cmd->signal_name] = { glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0)};
        return status_type::SUCCESS;
    }
}
