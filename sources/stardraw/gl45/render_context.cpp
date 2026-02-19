#include "render_context.hpp"
#include "window.hpp"

#include <format>

namespace stardraw::gl45
{
    std::tuple<GLenum, GLuint, bool, bool> gl_vertex_element_data_type(const vertex_data_type& type)
    {
        switch (type)
        {
            case vertex_data_type::UINT_U8: return { GL_UNSIGNED_BYTE, 1, false, true};
            case vertex_data_type::UNT2_U8: return { GL_UNSIGNED_BYTE, 2, false, true};
            case vertex_data_type::UINT3_U8: return { GL_UNSIGNED_BYTE, 3, false, true};
            case vertex_data_type::UINT4_U8: return { GL_UNSIGNED_BYTE, 4, false, true};
            case vertex_data_type::UINT_U16: return { GL_UNSIGNED_SHORT, 1, false, true};
            case vertex_data_type::UINT2_U16: return { GL_UNSIGNED_SHORT, 2, false, true};
            case vertex_data_type::UINT3_U16: return { GL_UNSIGNED_SHORT, 3, false, true};
            case vertex_data_type::UINT4_U16: return { GL_UNSIGNED_SHORT, 4, false, true};
            case vertex_data_type::UINT_U32: return { GL_UNSIGNED_INT, 1, false, true};
            case vertex_data_type::UINT2_U32: return { GL_UNSIGNED_INT, 2, false, true};
            case vertex_data_type::UINT3_U32: return { GL_UNSIGNED_INT, 3, false, true};
            case vertex_data_type::UINT4_U32: return { GL_UNSIGNED_INT, 4, false, true};
            case vertex_data_type::INT_I8: return { GL_BYTE, 1, false, true};
            case vertex_data_type::INT2_I8: return { GL_BYTE, 2, false, true};
            case vertex_data_type::INT3_I8: return { GL_BYTE, 3, false, true};
            case vertex_data_type::INT4_I8: return { GL_BYTE, 4, false, true};
            case vertex_data_type::INT_I16: return { GL_SHORT, 1, false, true};
            case vertex_data_type::INT2_I16: return { GL_SHORT, 2, false, true};
            case vertex_data_type::INT3_I16: return { GL_SHORT, 3, false, true};
            case vertex_data_type::INT4_I16: return { GL_SHORT, 4, false, true};
            case vertex_data_type::INT_I32: return { GL_INT, 1, false, true};
            case vertex_data_type::INT2_I32: return { GL_INT, 2, false, true};
            case vertex_data_type::INT3_I32: return { GL_INT, 3, false, true};
            case vertex_data_type::INT4_I32: return { GL_INT, 4, false, true};
            case vertex_data_type::FLOAT_U8_NORM: return { GL_UNSIGNED_BYTE, 1, true, false};
            case vertex_data_type::FLOAT2_U8_NORM: return { GL_UNSIGNED_BYTE, 2, true, false};
            case vertex_data_type::FLOAT3_U8_NORM: return { GL_UNSIGNED_BYTE, 3, true, false};
            case vertex_data_type::FLOAT4_U8_NORM: return { GL_UNSIGNED_BYTE, 4, true, false};
            case vertex_data_type::FLOAT_I8_NORM: return { GL_BYTE, 1, true, false};
            case vertex_data_type::FLOAT2_I8_NORM: return { GL_BYTE, 2, true, false};
            case vertex_data_type::FLOAT3_I8_NORM: return { GL_BYTE, 3, true, false};
            case vertex_data_type::FLOAT4_I8_NORM: return { GL_BYTE, 4, true, false};
            case vertex_data_type::FLOAT_U16_NORM: return { GL_UNSIGNED_SHORT, 1, true, false};
            case vertex_data_type::FLOAT2_U16_NORM: return { GL_UNSIGNED_SHORT, 2, true, false};
            case vertex_data_type::FLOAT3_U16_NORM: return { GL_UNSIGNED_SHORT, 3, true, false};
            case vertex_data_type::FLOAT4_U16_NORM: return { GL_UNSIGNED_SHORT, 4, true, false};
            case vertex_data_type::FLOAT_I16_NORM: return { GL_SHORT, 1, true, false};
            case vertex_data_type::FLOAT2_I16_NORM: return { GL_SHORT, 2, true, false};
            case vertex_data_type::FLOAT3_I16_NORM: return { GL_SHORT, 3, true, false};
            case vertex_data_type::FLOAT4_I16_NORM: return { GL_SHORT, 4, true, false};
            case vertex_data_type::FLOAT_F16: return { GL_HALF_FLOAT, 1, false, false};
            case vertex_data_type::FLOAT2_F16: return { GL_HALF_FLOAT, 2, false, false};
            case vertex_data_type::FLOAT3_F16: return { GL_HALF_FLOAT, 3, false, false};
            case vertex_data_type::FLOAT4_F16: return { GL_HALF_FLOAT, 4, false, false};
            case vertex_data_type::FLOAT_F32: return { GL_FLOAT, 1, false, false};
            case vertex_data_type::FLOAT2_F32: return { GL_FLOAT, 2, false, false};
            case vertex_data_type::FLOAT3_F32: return { GL_FLOAT, 3, false, false};
            case vertex_data_type::FLOAT4_F32: return { GL_FLOAT, 4, false, false};
        }
        return {-1, -1, false, false};
    }

    render_context::render_context(window* window) : parent_window(window) {}

    [[nodiscard]] status render_context::execute_command_buffer(const std::string_view& name)
    {
        status context_status = parent_window->make_gl_context_active();
        if (is_status_error(context_status)) return context_status;

        //Opengl doesn't have any persistant command buffers, so we just execute it like a temporary one without consuming it.
        if (!command_lists.contains(std::string(name))) return status_type::UNKNOWN_NAME;
        const command_list& refren = command_lists[std::string(name)];

        for (const polymorphic_ptr<command>& cmd : refren)
        {
            const status result = execute_command(cmd.ptr());
            if (is_status_error(result)) return result;
        }

        return status_type::SUCCESS;
    }

    [[nodiscard]] status render_context::execute_temp_command_buffer(const command_list&& commands)
    {
        status context_status = parent_window->make_gl_context_active();
        if (is_status_error(context_status)) return context_status;

        for (const polymorphic_ptr<command>& cmd : commands)
        {
            const status result = execute_command(cmd.ptr());
            if (is_status_error(result)) return result;
        }

        return status_type::SUCCESS;
    }

    [[nodiscard]] status render_context::create_command_buffer(const std::string_view& name, const command_list&& commands)
    {
        if (command_lists.contains(std::string(name))) return  { status_type::DUPLICATE_NAME, std::format("A command buffer named '{0}' already exists", name) };
        command_lists[std::string(name)] = commands;
        return status_type::SUCCESS;
    }

    [[nodiscard]] status render_context::delete_command_buffer(const std::string_view& name)
    {
        if (!command_lists.contains(std::string(name))) return status_type::NOTHING_TO_DO;
        command_lists.erase(std::string(name));
        return status_type::SUCCESS;
    }

    [[nodiscard]] status render_context::create_objects(const descriptor_list&& descriptors)
    {
        status context_status = parent_window->make_gl_context_active();
        if (is_status_error(context_status)) return context_status;

        for (const polymorphic_ptr<descriptor>& descriptor : descriptors)
        {
            const status create_status = create_object(descriptor.ptr());
            if (is_status_error(create_status)) return create_status;
        }

        return status_type::SUCCESS;
    }

    [[nodiscard]] status render_context::delete_object(const std::string_view& name)
    {
        const object_identifier identifier = object_identifier(name);
        if (!objects.contains(identifier.hash)) return status_type::NOTHING_TO_DO;

        const object_state* state = objects[identifier.hash];
        objects.erase(identifier.hash);
        delete state;

        return status_type::SUCCESS;
    }

    [[nodiscard]] signal_status render_context::check_signal(const std::string_view& name)
    {
        return wait_signal(name, 0);
    }

    [[nodiscard]] signal_status render_context::wait_signal(const std::string_view& name, const uint64_t timeout)
    {
        const status context_status = parent_window->make_gl_context_active();
        if (is_status_error(context_status)) return signal_status::CONTEXT_ERROR;

        if (!signals.contains(std::string(name)))
        {
            return signal_status::UNKNOWN_SIGNAL;
        }

        const signal_state& state = signals[std::string(name)];
        const GLenum status = glClientWaitSync(state.sync_point, 0, timeout);
        switch (status)
        {
            case GL_ALREADY_SIGNALED: return signal_status::SIGNALLED;
            case GL_TIMEOUT_EXPIRED: return signal_status::TIMED_OUT;
            case GL_CONDITION_SATISFIED: return signal_status::SIGNALLED;
            default: return signal_status::NOT_SIGNALLED;
        }
    }

    [[nodiscard]] status render_context::bind_buffer(const object_identifier& source, const GLenum target)
    {
        const buffer_state* buffer_state = find_gl_buffer_state(source);
        if (buffer_state == nullptr) return { status_type::UNKNOWN_SOURCE, std::format("No buffer with name '{0}' exists in context", source.name) };
        return buffer_state->bind_to(target);
    }

    [[nodiscard]] status render_context::execute_command(const command* cmd)
    {
        if (cmd == nullptr)
        {
            return { status_type::UNEXPECTED_NULL, "Null command" };
        }

        const command_type type = cmd->type();
        switch (type)
        {
            case command_type::DRAW: return execute_draw(dynamic_cast<const draw_command*>(cmd));
            case command_type::DRAW_INDEXED: return execute_draw_indexed(dynamic_cast<const draw_indexed_command*>(cmd));
            case command_type::DRAW_INDIRECT: return execute_draw_indirect(dynamic_cast<const draw_indirect_command*>(cmd));
            case command_type::DRAW_INDEXED_INDIRECT: return execute_draw_indexed_indirect(dynamic_cast<const draw_indexed_indirect_command*>(cmd));

            case command_type::CONFIG_BLENDING: return execute_config_blending(dynamic_cast<const blending_config_command*>(cmd));
            case command_type::CONFIG_STENCIL: return execute_config_stencil(dynamic_cast<const stencil_config_command*>(cmd));
            case command_type::CONFIG_SCISSOR: return execute_config_scissor(dynamic_cast<const scissor_config_command*>(cmd));
            case command_type::CONFIG_FACE_CULL: return execute_config_face_cull(dynamic_cast<const face_cull_config_command*>(cmd));
            case command_type::CONFIG_DEPTH_TEST: return execute_config_depth_test(dynamic_cast<const depth_test_config_command*>(cmd));
            case command_type::CONFIG_DEPTH_RANGE: return execute_config_depth_range(dynamic_cast<const depth_range_config_command*>(cmd));

            case command_type::BUFFER_UPLOAD: return execute_buffer_upload(dynamic_cast<const buffer_upload_command*>(cmd));
            case command_type::BUFFER_COPY: return execute_buffer_copy(dynamic_cast<const buffer_copy_command*>(cmd));
            case command_type::BUFFER_DOWNLOAD: return status_type::UNIMPLEMENTED; //TODO

            case command_type::CLEAR_WINDOW: return execute_clear_window(dynamic_cast<const clear_window_command*>(cmd));
            case command_type::CLEAR_BUFFER: return status_type::UNIMPLEMENTED; //TODO
            case command_type::SHADER_PARAMETERS_UPLOAD: return execute_shader_parameters_upload(dynamic_cast<const shader_parameters_upload_command*>(cmd));
        }

        return{ status_type::UNSUPPORTED, "Unsupported command" };
    }

    [[nodiscard]] status render_context::create_object(const descriptor* descriptor)
    {
        if (objects.contains(descriptor->identifier().hash))
        {
            return {status_type::DUPLICATE_NAME, std::format("An object with name {0} already exists (or there is a hash collision)", descriptor->identifier().name)};
        }

        const descriptor_type type = descriptor->type();
        switch (type)
        {
            case descriptor_type::BUFFER: return create_buffer_state(dynamic_cast<const buffer_descriptor*>(descriptor));
            case descriptor_type::SHADER: return create_shader_state(dynamic_cast<const shader_descriptor*>(descriptor));
            case descriptor_type::VERTEX_SPECIFICATION: return create_vertex_specification_state(dynamic_cast<const vertex_specification_descriptor*>(descriptor));
            case descriptor_type::SHADER_SPECIFICATION: return create_shader_specification_state(dynamic_cast<const shader_specification_descriptor*>(descriptor));
            case descriptor_type::DRAW_SPECIFICATION: return create_draw_specification_state(dynamic_cast<const draw_specification_descriptor*>(descriptor));
        }
        return status_type::UNIMPLEMENTED;
    }

    [[nodiscard]] status render_context::create_buffer_state(const buffer_descriptor* descriptor)
    {
        status create_status = status_type::SUCCESS;
        buffer_state* buffer = new buffer_state(*descriptor, create_status);
        if (!buffer->is_valid())
        {
            delete buffer;
            return create_status;
        }

        objects[descriptor->identifier().hash] = buffer;

        return status_type::SUCCESS;
    }

    status render_context::create_shader_state(const shader_descriptor* descriptor)
    {
        status shader_create_status = status_type::SUCCESS;
        shader_state* shader = new shader_state(*descriptor, shader_create_status);
        if (is_status_error(shader_create_status))
        {
            delete shader;
            return shader_create_status;
        }

        objects[descriptor->identifier().hash] = shader;
        return status_type::SUCCESS;
    }

    status render_context::create_shader_specification_state(const shader_specification_descriptor* descriptor)
    {
        shader_specification_state* shader_spec = new shader_specification_state(*descriptor);

        const shader_state* shader = find_gl_shader_state(shader_spec->shader);
        if (shader == nullptr) return { status_type::UNKNOWN_SOURCE, std::format("Referenced shader object '{0}' not found in context", shader_spec->shader.name) };
        if (!shader->is_valid()) return {status_type::BROKEN_SOURCE, std::format("Shader object '{0}' is in an invalid state", shader_spec->shader.name) };

        objects[descriptor->identifier().hash] = shader_spec;
        return status_type::SUCCESS;
    }

    status render_context::create_vertex_specification_state(const vertex_specification_descriptor* descriptor)
    {
        vertex_specification_state* vertex_spec = new vertex_specification_state();
        if (vertex_spec->vertex_array_id == 0)
        {
            delete vertex_spec;
            return { status_type::BACKEND_ERROR, std::format("Attempting to create vertex specification '{0}' resulted in an invalid buffer", descriptor->identifier().name) };
        }

        const vertex_data_layout& format = descriptor->layout;
        std::vector<GLsizeiptr> buffer_strides;
        std::vector<std::string> buffer_names;
        std::unordered_map<std::string, GLuint> buffer_slots;
        std::unordered_map<std::string, buffer_state*> buffer_states;

        GLuint buffer_slot = 0;
        for (const vertex_data_binding& element : format.bindings)
        {
            const std::string& buffer_name = element.buffer;
            if (buffer_slots.contains(buffer_name)) continue;
            buffer_slots[buffer_name] = buffer_slot;
            buffer_names.push_back(buffer_name);

            buffer_state* buffer_state = find_gl_buffer_state(object_identifier(buffer_name));
            if (buffer_state == nullptr)
            {
                delete vertex_spec;
                return { status_type::UNKNOWN_SOURCE, std::format("No buffer named '{0}' found while creating vertex specification '{1}'", buffer_name, descriptor->identifier().name)};
            }
            buffer_states[buffer_name] = buffer_state;
            buffer_slot++;
        }

        buffer_strides.resize(buffer_slot);

        const GLuint vao_id = vertex_spec->vertex_array_id;

        for (uint32_t idx = 0; idx < format.bindings.size(); idx++)
        {
            const vertex_data_binding& elem = format.bindings[idx];
            GLsizeiptr& offset = buffer_strides[buffer_slots[elem.buffer]];

            glEnableVertexArrayAttrib(vao_id, idx);
            glVertexArrayAttribBinding(vao_id, idx, buffer_slots[elem.buffer]);
            if (elem.instance_divisor > 0) glVertexArrayBindingDivisor(vao_id, idx, elem.instance_divisor);

            auto [type, count, normalized, integer] = gl_vertex_element_data_type(elem.type);

            if (integer)
            {
                glVertexArrayAttribIFormat(vao_id, idx, count, type, offset);
            }
            else if (type == GL_DOUBLE)
            {
                glVertexArrayAttribLFormat(vao_id, idx, count, type, offset);
            }
            else
            {
                glVertexArrayAttribFormat(vao_id, idx, count, type, normalized, offset);
            }

            offset += vertex_element_size(elem.type);
        }

        for (const std::string& vertex_buffer : buffer_names)
        {
            const buffer_state* buffer_state = buffer_states[vertex_buffer];
            const status attach_status = vertex_spec->attach_vertex_buffer(buffer_slots[vertex_buffer], buffer_state->gl_id(), 0, buffer_strides[buffer_slots[vertex_buffer]]);

            if (is_status_error(attach_status))
            {
                delete vertex_spec;
                return attach_status;
            }
        }

        if (!descriptor->index_buffer.empty())
        {
            const buffer_state* index_buffer_state = find_gl_buffer_state(object_identifier(descriptor->index_buffer));
            if (index_buffer_state == nullptr)
            {
                delete vertex_spec;
                return { status_type::UNKNOWN_SOURCE, std::format("No buffer named '{0}' found while creating vertex specification '{1}'", descriptor->index_buffer, descriptor->identifier().name)};
            }

            const status attach_status = vertex_spec->attach_index_buffer(index_buffer_state->gl_id());

            if (is_status_error(attach_status))
            {
                delete vertex_spec;
                return attach_status;
            }
        }

        if (!vertex_spec->is_valid())
        {
            delete vertex_spec;
            return { status_type::BACKEND_ERROR, std::format("Creating vertex specification '{0}' resulted in an invalid object", descriptor->identifier().name)};
        }

        objects[descriptor->identifier().hash] = vertex_spec;

        return status_type::SUCCESS;
    }

    status render_context::create_draw_specification_state(const draw_specification_descriptor* descriptor)
    {
        if (!find_gl_vertex_specification_state(object_identifier(descriptor->vertex_specification)))
        {
            return {status_type::UNKNOWN_NAME, std::format("Referenced vertex specification '{0}' not found in context", descriptor->vertex_specification)};
        }

        if (!find_gl_shader_specification_state(object_identifier(descriptor->shader_specification)))
        {
            return {status_type::UNKNOWN_NAME, std::format("Referenced shader specification '{0}' not found in context", descriptor->shader_specification)};
        }

        //draw specification is a thin wrapper that references shader and vertex specifications
        objects[descriptor->identifier().hash] = new draw_specification_state(*descriptor);
        return status_type::SUCCESS;
    }

    status render_context::bind_vertex_specification_state(const object_identifier& source, const bool requires_index_buffer = false)
    {
        const vertex_specification_state* state = find_gl_vertex_specification_state(source);
        if (state == nullptr) return { status_type::UNKNOWN_SOURCE, std::format("No vertex specification with name '{0}' exists in context", source.name) };
        if (!state->is_valid()) return { status_type::BROKEN_SOURCE, std::format("Vertex specification object '{0}' is in an invalid state", source.name) };
        if (requires_index_buffer && state->index_buffer == 0) return { status_type::BROKEN_SOURCE, std::format("Vertex specification '{0}' does not have an index buffer, cannot be used for indexed drawing", source.name)};
        return state->bind();
    }

    status render_context::bind_shader_specification_state(const object_identifier& source)
    {
        const shader_specification_state* shader_spec = find_gl_shader_specification_state(source);
        if (shader_spec == nullptr) return { status_type::UNKNOWN_SOURCE, std::format("No shader specification with name '{0}' exists in context", source.name) };

        const shader_state* shader = find_gl_shader_state(shader_spec->shader);
        if (shader == nullptr) return { status_type::UNKNOWN_SOURCE, std::format("Referenced shader object '{0}' not found in context (referenced by shader specification '{1}')", shader_spec->shader.name, source.name) };
        if (!shader->is_valid()) return {status_type::BROKEN_SOURCE, std::format("Shader object '{0}' is in an invalid state (referenced by shader specification '{1}')", shader_spec->shader.name, source.name) };

        status activate_status = shader->make_active();
        if (is_status_error(activate_status)) return activate_status;

        for (const shader_data_binding& binding : shader_spec->bindings)
        {
            const buffer_state* buffer = find_gl_buffer_state(object_identifier(binding.buffer));
            if (buffer == nullptr) return { status_type::UNKNOWN_SOURCE, std::format("Referenced buffer object '{0}' not found in context (referenced by shader specification '{1}')", binding.buffer, source.name) };
            if (!buffer->is_valid()) return { status_type::BROKEN_SOURCE, std::format("Buffer object '{0}' is in an invalid state (referenced by shader specification '{1}')", binding.buffer, source.name) };

            shader_state::binding_block_location binding_location {};
            status has_binding = shader->get_binding_slot(binding.binding, binding_location);
            if (is_status_error(has_binding)) return has_binding;

            status attach_status = buffer->bind_to_slot(binding_location.type, binding_location.slot);
            if (is_status_error(attach_status)) return attach_status;
        }

        return status_type::SUCCESS;
    }

    status render_context::bind_draw_specification_state(const object_identifier& source, const bool requres_index_buffer)
    {
        const draw_specification_state* state = find_gl_draw_specification_state(source);
        if (state == nullptr) return { status_type::UNKNOWN_SOURCE, std::format("Draw specification object '{0}' not found in context", source.name) };

        status vertex_specification_bind = bind_vertex_specification_state(state->vertex_specification, requres_index_buffer);
        if (is_status_error(vertex_specification_bind)) return vertex_specification_bind;

        status shader_specification_bind = bind_shader_specification_state(state->shader_specification);
        if (is_status_error(shader_specification_bind)) return shader_specification_bind;

        return status_type::SUCCESS;
    }
}
