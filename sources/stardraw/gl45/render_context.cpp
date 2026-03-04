#include "render_context.hpp"
#include "window.hpp"

#include <format>

#include "stardraw/internal/internal.hpp"

namespace stardraw::gl45
{
    std::tuple<GLenum, GLuint, bool, bool> gl_vertex_element_data_type(const vertex_data_type& type)
    {
        switch (type)
        {
            case vertex_data_type::UINT_U8: return {GL_UNSIGNED_BYTE, 1, false, true};
            case vertex_data_type::UINT2_U8: return {GL_UNSIGNED_BYTE, 2, false, true};
            case vertex_data_type::UINT3_U8: return {GL_UNSIGNED_BYTE, 3, false, true};
            case vertex_data_type::UINT4_U8: return {GL_UNSIGNED_BYTE, 4, false, true};
            case vertex_data_type::UINT_U16: return {GL_UNSIGNED_SHORT, 1, false, true};
            case vertex_data_type::UINT2_U16: return {GL_UNSIGNED_SHORT, 2, false, true};
            case vertex_data_type::UINT3_U16: return {GL_UNSIGNED_SHORT, 3, false, true};
            case vertex_data_type::UINT4_U16: return {GL_UNSIGNED_SHORT, 4, false, true};
            case vertex_data_type::UINT_U32: return {GL_UNSIGNED_INT, 1, false, true};
            case vertex_data_type::UINT2_U32: return {GL_UNSIGNED_INT, 2, false, true};
            case vertex_data_type::UINT3_U32: return {GL_UNSIGNED_INT, 3, false, true};
            case vertex_data_type::UINT4_U32: return {GL_UNSIGNED_INT, 4, false, true};
            case vertex_data_type::INT_I8: return {GL_BYTE, 1, false, true};
            case vertex_data_type::INT2_I8: return {GL_BYTE, 2, false, true};
            case vertex_data_type::INT3_I8: return {GL_BYTE, 3, false, true};
            case vertex_data_type::INT4_I8: return {GL_BYTE, 4, false, true};
            case vertex_data_type::INT_I16: return {GL_SHORT, 1, false, true};
            case vertex_data_type::INT2_I16: return {GL_SHORT, 2, false, true};
            case vertex_data_type::INT3_I16: return {GL_SHORT, 3, false, true};
            case vertex_data_type::INT4_I16: return {GL_SHORT, 4, false, true};
            case vertex_data_type::INT_I32: return {GL_INT, 1, false, true};
            case vertex_data_type::INT2_I32: return {GL_INT, 2, false, true};
            case vertex_data_type::INT3_I32: return {GL_INT, 3, false, true};
            case vertex_data_type::INT4_I32: return {GL_INT, 4, false, true};
            case vertex_data_type::FLOAT_U8_NORM: return {GL_UNSIGNED_BYTE, 1, true, false};
            case vertex_data_type::FLOAT2_U8_NORM: return {GL_UNSIGNED_BYTE, 2, true, false};
            case vertex_data_type::FLOAT3_U8_NORM: return {GL_UNSIGNED_BYTE, 3, true, false};
            case vertex_data_type::FLOAT4_U8_NORM: return {GL_UNSIGNED_BYTE, 4, true, false};
            case vertex_data_type::FLOAT_I8_NORM: return {GL_BYTE, 1, true, false};
            case vertex_data_type::FLOAT2_I8_NORM: return {GL_BYTE, 2, true, false};
            case vertex_data_type::FLOAT3_I8_NORM: return {GL_BYTE, 3, true, false};
            case vertex_data_type::FLOAT4_I8_NORM: return {GL_BYTE, 4, true, false};
            case vertex_data_type::FLOAT_U16_NORM: return {GL_UNSIGNED_SHORT, 1, true, false};
            case vertex_data_type::FLOAT2_U16_NORM: return {GL_UNSIGNED_SHORT, 2, true, false};
            case vertex_data_type::FLOAT3_U16_NORM: return {GL_UNSIGNED_SHORT, 3, true, false};
            case vertex_data_type::FLOAT4_U16_NORM: return {GL_UNSIGNED_SHORT, 4, true, false};
            case vertex_data_type::FLOAT_I16_NORM: return {GL_SHORT, 1, true, false};
            case vertex_data_type::FLOAT2_I16_NORM: return {GL_SHORT, 2, true, false};
            case vertex_data_type::FLOAT3_I16_NORM: return {GL_SHORT, 3, true, false};
            case vertex_data_type::FLOAT4_I16_NORM: return {GL_SHORT, 4, true, false};
            case vertex_data_type::FLOAT_F16: return {GL_HALF_FLOAT, 1, false, false};
            case vertex_data_type::FLOAT2_F16: return {GL_HALF_FLOAT, 2, false, false};
            case vertex_data_type::FLOAT3_F16: return {GL_HALF_FLOAT, 3, false, false};
            case vertex_data_type::FLOAT4_F16: return {GL_HALF_FLOAT, 4, false, false};
            case vertex_data_type::FLOAT_F32: return {GL_FLOAT, 1, false, false};
            case vertex_data_type::FLOAT2_F32: return {GL_FLOAT, 2, false, false};
            case vertex_data_type::FLOAT3_F32: return {GL_FLOAT, 3, false, false};
            case vertex_data_type::FLOAT4_F32: return {GL_FLOAT, 4, false, false};
        }
        return {-1, -1, false, false};
    }

    render_context::render_context(window* window) : parent_window(window) {}

    [[nodiscard]] status render_context::execute_command_buffer(const std::string_view& name)
    {
        status context_status = parent_window->make_gl_context_active();
        if (is_status_error(context_status)) return context_status;

        //Opengl doesn't have any persistant command buffers, so we just execute it like a temporary one without consuming it.
        if (!command_lists.contains(std::string(name))) return status_type::UNKNOWN;
        const command_list& refren = command_lists[std::string(name)];

        for (const starlib::polymorphic<command>& cmd : refren)
        {
            const status result = execute_command(cmd.ptr());
            if (is_status_error(result)) return result;
        }

        return status_from_last_gl_error();
    }

    [[nodiscard]] status render_context::execute_temp_command_buffer(const command_list&& commands)
    {
        status context_status = parent_window->make_gl_context_active();
        if (is_status_error(context_status)) return context_status;

        for (const starlib::polymorphic<command>& cmd : commands)
        {
            const status result = execute_command(cmd.ptr());
            if (is_status_error(result)) return result;
        }

        return status_from_last_gl_error();
    }

    [[nodiscard]] status render_context::create_command_buffer(const std::string_view& name, const command_list&& commands)
    {
        if (command_lists.contains(std::string(name))) return {status_type::DUPLICATE, std::format("A command buffer named '{0}' already exists", name)};
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

        for (const starlib::polymorphic<descriptor>& descriptor : descriptors)
        {
            const status create_status = create_object(descriptor.ptr());
            if (is_status_error(create_status)) return create_status;
        }

        return status_from_last_gl_error();
    }

    [[nodiscard]] status render_context::delete_object(const descriptor_type type, const std::string_view& name)
    {
        const object_identifier identifier = object_identifier(name);
        if (!objects.contains(type)) return status_type::NOTHING_TO_DO;
        if (!objects[type].contains(identifier.hash)) return status_type::NOTHING_TO_DO;

        delete objects[type][identifier.hash];
        objects[type].erase(identifier.hash);

        return status_from_last_gl_error();
    }

    [[nodiscard]] signal_status render_context::check_signal(const std::string_view& name)
    {
        return wait_signal(name, 0);
    }

    [[nodiscard]] signal_status render_context::wait_signal(const std::string_view& name, const u64 timeout)
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

    status render_context::prepare_memory_transfer(const memory_transfer_info& info, memory_transfer_handle** out_handle)
    {
        switch (info.target_type)
        {
            case memory_transfer_target_type::BUFFER:
            {
                buffer_state* buffer = find_buffer_state(object_identifier(info.target));
                if (buffer == nullptr) return {status_type::UNKNOWN, std::format("No buffer with name '{0}' in context", info.target)};
                if (!buffer->is_valid()) return {status_type::INVALID, std::format("Buffer '{0}' is in an invalid state", info.target)};

                switch (info.transfer_type)
                {
                    case memory_transfer_type::STREAMING_UPLOAD:
                    {
                        memory_transfer_handle* handle;
                        status prepare_status = buffer->prepare_upload_data_streaming(info.address, info.bytes, &handle);
                        if (is_status_error(prepare_status)) return prepare_status;
                        memory_transfers[handle] = info;
                        *out_handle = handle;
                        return status_type::SUCCESS;
                    }
                    case memory_transfer_type::CHUNKED_UPLOAD:
                    {
                        memory_transfer_handle* handle;
                        status prepare_status = buffer->prepare_upload_data_chunked(info.address, info.bytes, &handle);
                        if (is_status_error(prepare_status)) return prepare_status;
                        memory_transfers[handle] = info;
                        *out_handle = handle;
                        return status_type::SUCCESS;
                    }
                    case memory_transfer_type::UNCHECKED_UPLOAD:
                    {
                        memory_transfer_handle* handle;
                        status prepare_status = buffer->prepare_upload_data_unchecked(info.address, info.bytes, &handle);
                        if (is_status_error(prepare_status)) return prepare_status;
                        memory_transfers[handle] = info;
                        *out_handle = handle;
                        return status_type::SUCCESS;
                    }
                    default: return {status_type::UNIMPLEMENTED};
                }
            }

            default: return {status_type::UNIMPLEMENTED};
        }
    }

    status render_context::flush_memory_transfer(memory_transfer_handle* handle)
    {
        if (!memory_transfers.contains(handle)) return {status_type::UNKNOWN, "Memory transfer handle not recognized - did you create it with a different context?"};
        const memory_transfer_info info = memory_transfers[handle];
        memory_transfers.erase(handle);

        switch (info.target_type)
        {
            case memory_transfer_target_type::BUFFER:
            {
                const buffer_state* buffer = find_buffer_state(object_identifier(info.target));
                if (buffer == nullptr) return {status_type::UNKNOWN, std::format("No buffer with name '{0}' in context", info.target)};
                if (!buffer->is_valid()) return {status_type::INVALID, std::format("Buffer '{0}' is in an invalid state", info.target)};
                switch (info.transfer_type)
                {
                    case memory_transfer_type::STREAMING_UPLOAD: return buffer->flush_upload_data_streaming(handle);
                    case memory_transfer_type::CHUNKED_UPLOAD: return buffer->flush_upload_data_chunked(handle);
                    case memory_transfer_type::UNCHECKED_UPLOAD: return buffer->flush_upload_data_unchecked(handle);

                    default: return {status_type::UNIMPLEMENTED};
                }
            }

            default: return {status_type::UNIMPLEMENTED};
        }
    }

    status render_context::status_from_last_gl_error()
    {
        GLenum latest_status = glGetError();
        if (latest_status == GL_NO_ERROR) return status_type::SUCCESS;

        std::vector<GLenum> errors;
        while (latest_status != GL_NO_ERROR)
        {
            errors.push_back(latest_status);
            latest_status = glGetError();
        }

        std::string error_string = "???";
        switch (errors[0])
        {
            case GL_INVALID_ENUM: error_string = "Invalid enum (probably stardraw bug)";
                break;
            case GL_INVALID_OPERATION: error_string = "Invalid operation (probably stardraw bug)";
                break;
            case GL_INVALID_VALUE: error_string = "Invalid value (probably stardraw bug)";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error_string = "Invalid framebuffer operation (probably stardraw bug)";
                break;
            case GL_OUT_OF_MEMORY: error_string = "Out of memory";
                break;
            case GL_STACK_OVERFLOW: error_string = "Stack overflow (this is definitely a stardraw bug!)";
                break;
            case GL_STACK_UNDERFLOW: error_string = "Stack underflow (this is definitely a stardraw bug!)";
                break;
            default: error_string = "Unknown error";
                break;
        }
        return {status_type::BACKEND_ERROR, std::format("Operations generated {0} GL errors. First error triggered: {1}", errors.size(), error_string)};
    }

    [[nodiscard]] status render_context::execute_command(const command* cmd)
    {
        if (cmd == nullptr)
        {
            return {status_type::UNEXPECTED, "Null command"};
        }

        const command_type type = cmd->type();
        switch (type)
        {
            case command_type::DRAW: return execute_draw(dynamic_cast<const draw_command*>(cmd));
            case command_type::DRAW_INDEXED: return execute_draw_indexed(dynamic_cast<const draw_indexed_command*>(cmd));
            case command_type::DRAW_INDIRECT: return execute_draw_indirect(dynamic_cast<const draw_indirect_command*>(cmd));
            case command_type::DRAW_INDEXED_INDIRECT: return execute_draw_indexed_indirect(dynamic_cast<const draw_indexed_indirect_command*>(cmd));

            case command_type::CONFIG_DRAW: return execute_draw_config(dynamic_cast<const draw_config_command*>(cmd));
            case command_type::CONFIG_BLENDING: return execute_config_blending(dynamic_cast<const blending_config_command*>(cmd));
            case command_type::CONFIG_STENCIL: return execute_config_stencil(dynamic_cast<const stencil_config_command*>(cmd));
            case command_type::CONFIG_SCISSOR: return execute_config_scissor(dynamic_cast<const scissor_config_command*>(cmd));
            case command_type::CONFIG_FACE_CULL: return execute_config_face_cull(dynamic_cast<const face_cull_config_command*>(cmd));
            case command_type::CONFIG_DEPTH_TEST: return execute_config_depth_test(dynamic_cast<const depth_test_config_command*>(cmd));
            case command_type::CONFIG_DEPTH_RANGE: return execute_config_depth_range(dynamic_cast<const depth_range_config_command*>(cmd));

            case command_type::BUFFER_COPY: return execute_buffer_copy(dynamic_cast<const buffer_copy_command*>(cmd));

            case command_type::CLEAR_WINDOW: return execute_clear_window(dynamic_cast<const clear_window_command*>(cmd));
            case command_type::CLEAR_BUFFER: return status_type::UNIMPLEMENTED; //TODO
            case command_type::CONFIG_SHADER: return execute_shader_parameters_upload(dynamic_cast<const shader_config_command*>(cmd));
            case command_type::SIGNAL: return execute_signal(dynamic_cast<const signal_command*>(cmd));
            case command_type::TEXTURE_COPY: break; //TODO
        }

        return {status_type::UNSUPPORTED, "Unsupported command"};
    }

    [[nodiscard]] status render_context::create_object(const descriptor* descriptor)
    {
        const descriptor_type type = descriptor->type();
        switch (type)
        {
            case descriptor_type::BUFFER: return create_buffer_state(dynamic_cast<const buffer_descriptor*>(descriptor));
            case descriptor_type::SHADER: return create_shader_state(dynamic_cast<const shader_descriptor*>(descriptor));
            case descriptor_type::VERTEX_SPECIFICATION: return create_vertex_specification_state(dynamic_cast<const vertex_specification_descriptor*>(descriptor));
            case descriptor_type::DRAW_SPECIFICATION: return create_draw_specification_state(dynamic_cast<const draw_specification_descriptor*>(descriptor));
            case descriptor_type::TEXTURE: return create_texture_state(dynamic_cast<const texture_descriptor*>(descriptor));
            case descriptor_type::TEXTURE_SAMPLER: return create_texture_sampler_state(dynamic_cast<const texture_sampler_descriptor*>(descriptor));
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

        return record_object_state(descriptor->identifier(), buffer);
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

        return record_object_state(descriptor->identifier(), shader);
    }

    status render_context::create_texture_state(const texture_descriptor* descriptor)
    {
        status texture_create_status = status_type::SUCCESS;
        texture_state* texture = new texture_state(*descriptor, texture_create_status);
        if (is_status_error(texture_create_status))
        {
            delete texture;
            return texture_create_status;
        }

        return record_object_state(descriptor->identifier(), texture);
    }

    status render_context::create_texture_sampler_state(const texture_sampler_descriptor* descriptor)
    {
        return status_type::UNIMPLEMENTED;
    }

    status render_context::create_vertex_specification_state(const vertex_specification_descriptor* descriptor)
    {
        vertex_specification_state* vertex_spec = new vertex_specification_state();
        if (vertex_spec->vertex_array_id == 0)
        {
            delete vertex_spec;
            return {status_type::BACKEND_ERROR, std::format("Attempting to create vertex specification '{0}' resulted in an invalid buffer", descriptor->identifier().name)};
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

            buffer_state* buffer_state = find_buffer_state(object_identifier(buffer_name));
            if (buffer_state == nullptr)
            {
                delete vertex_spec;
                return {status_type::UNKNOWN, std::format("No buffer'{0}' found while creating vertex specification '{1}'", buffer_name, descriptor->identifier().name)};
            }
            if (!buffer_state->is_valid())
            {
                delete vertex_spec;
                return {status_type::INVALID, std::format("Can't create vertex specification '{1}', buffer '{0}' is in an invalid state!", buffer_name, descriptor->identifier().name)};
            }
            buffer_states[buffer_name] = buffer_state;
            buffer_slot++;
        }

        buffer_strides.resize(buffer_slot);

        const GLuint vao_id = vertex_spec->vertex_array_id;

        for (u32 idx = 0; idx < format.bindings.size(); idx++)
        {
            const vertex_data_binding& elem = format.bindings[idx];
            GLsizeiptr& offset = buffer_strides[buffer_slots[elem.buffer]];

            glEnableVertexArrayAttrib(vao_id, idx);
            glVertexArrayAttribBinding(vao_id, idx, buffer_slots[elem.buffer]);
            if (elem.instance_divisor > 0)
            {
                glVertexArrayBindingDivisor(vao_id, idx, elem.instance_divisor);
            }

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
            const buffer_state* index_buffer_state = find_buffer_state(object_identifier(descriptor->index_buffer));
            if (index_buffer_state == nullptr)
            {
                delete vertex_spec;
                return {status_type::UNKNOWN, std::format("No buffer named '{0}' found while creating vertex specification '{1}'", descriptor->index_buffer, descriptor->identifier().name)};
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
            return {status_type::BACKEND_ERROR, std::format("Creating vertex specification '{0}' resulted in an invalid object", descriptor->identifier().name)};
        }

        return record_object_state(descriptor->identifier(), vertex_spec);
    }

    status render_context::create_draw_specification_state(const draw_specification_descriptor* descriptor)
    {
        const vertex_specification_state* vertex_spec = find_vertex_specification_state(object_identifier(descriptor->vertex_specification));
        if (!vertex_spec)
        {
            return {status_type::UNKNOWN, std::format("Referenced vertex specification '{0}' not found in context", descriptor->vertex_specification)};
        }

        if (!find_shader_state(object_identifier(descriptor->shader)))
        {
            return {status_type::UNKNOWN, std::format("Referenced shader '{0}' not found in context", descriptor->shader)};
        }

        //draw specification is a thin wrapper that references shader and vertex specifications
        return record_object_state(descriptor->identifier(), new draw_specification_state(*descriptor, vertex_spec->has_index_buffer()));
    }

    status render_context::bind_vertex_specification_state(const object_identifier& source)
    {
        const vertex_specification_state* state = find_vertex_specification_state(source);
        if (state == nullptr) return {status_type::UNKNOWN, std::format("No vertex specification with name '{0}' exists in context", source.name)};
        if (!state->is_valid()) return {status_type::INVALID, std::format("Vertex specification object '{0}' is in an invalid state", source.name)};
        return state->bind();
    }

    status render_context::bind_draw_specification_state(const object_identifier& source)
    {
        const draw_specification_state* state = find_draw_specification_state(source);
        if (state == nullptr) return {status_type::UNKNOWN, std::format("Draw specification object '{0}' not found in context", source.name)};

        status vertex_specification_bind = bind_vertex_specification_state(state->vertex_specification);
        if (is_status_error(vertex_specification_bind)) return vertex_specification_bind;

        status shader_bind = bind_shader(state->shader);
        if (is_status_error(shader_bind)) return shader_bind;

        active_draw_specification = state;

        return status_type::SUCCESS;
    }

    [[nodiscard]] status render_context::bind_buffer(const object_identifier& source, const GLenum target)
    {
        const buffer_state* buffer_state = find_buffer_state(source);
        if (buffer_state == nullptr) return {status_type::UNKNOWN, std::format("No buffer with name '{0}' exists in context", source.name)};
        return buffer_state->bind_to(target);
    }

    status render_context::bind_shader(const object_identifier& source)
    {
        shader_state* shader = find_shader_state(source);
        if (shader == nullptr) return {status_type::UNKNOWN, std::format("Shader object '{0}' not found in context", source.name)};
        if (!shader->is_valid()) return {status_type::INVALID, std::format("Shader object '{0}' is in an invalid state", source.name)};

        status activate_status = shader->make_active();
        if (is_status_error(activate_status)) return activate_status;

        for (shader_parameter& param : shader->parameter_store)
        {
            const shader_parameter_location& location = param.location;
            shader_parameter_value& value = param.value;

            status result_status = status_type::SUCCESS;

            switch (value.type)
            {
                case shader_parameter_value::value_type::BUFFER_REFERENCE:
                {
                    result_status = bind_shader_buffer_parameter(shader, location, value);
                    break;
                }
                default:
                {
                    result_status = bind_shader_data_parameter(shader, location, value);
                    break;
                }
            }

            if (is_status_error(result_status)) return result_status;
        }

        return status_type::SUCCESS;
    }

    status render_context::bind_shader_buffer_parameter(shader_state* shader, const shader_parameter_location& location, const shader_parameter_value& value)
    {
        const binding_location_info binding_info = vk_binding_for_location(location);
        const u32 actual_slot = binding_info.slot + shader->descriptor_set_binding_offsets[binding_info.set];
        GLenum binding_type = 0;

        //To bind a buffer, we make sure the location is *explicitly* pointed at the buffer variable, not something contained inside the buffer.
        if (binding_info.binding_type != location.offset_ptr)
        {
            return {status_type::UNSUPPORTED, "The shader parameter location provided cannot have a buffer bound to it!"};
        }

        switch (binding_info.binding_type->getKind())
        {
            case slang::TypeReflection::Kind::ParameterBlock:
            case slang::TypeReflection::Kind::ConstantBuffer:
            {
                binding_type = GL_UNIFORM_BUFFER;
                break;
            }

            case slang::TypeReflection::Kind::ShaderStorageBuffer:
            {
                binding_type = GL_SHADER_STORAGE_BUFFER;
                break;
            }

            case slang::TypeReflection::Kind::Resource:
            {
                const SlangResourceShape shape = binding_info.binding_type->getResourceShape();
                if (shape == SLANG_STRUCTURED_BUFFER || shape == SLANG_BYTE_ADDRESS_BUFFER)
                {
                    binding_type = GL_SHADER_STORAGE_BUFFER;
                    break;
                }

                //fallthrough to unsupported
            }
            default:
            {
                return {status_type::UNSUPPORTED, "The shader parameter location provided cannot have a buffer bound to it!"};
            }
        }

        const buffer_state* buffer = find_buffer_state(object_identifier(value.opaque_reference));
        if (buffer == nullptr) return {status_type::UNKNOWN, std::format("Buffer object '{0}' not found in context (referenced by shader parameter)", value.opaque_reference)};
        if (!buffer->is_valid()) return {status_type::INVALID, std::format("Buffer object '{0}' is in an invalid state (referenced by shader parameter)", value.opaque_reference)};
        status bind_status = buffer->bind_to_slot(binding_type, actual_slot);
        if (is_status_error(bind_status)) return bind_status;
        shader->bound_objects[actual_slot] = value.opaque_reference;
        return status_type::SUCCESS;
    }

    status render_context::bind_shader_data_parameter(shader_state* shader, const shader_parameter_location& location, shader_parameter_value& value)
    {
        const binding_location_info binding_info = vk_binding_for_location(location);
        const u32 actual_slot = binding_info.slot + shader->descriptor_set_binding_offsets[binding_info.set];

        if (!shader->bound_objects.contains(actual_slot))
        {
            return {status_type::INVALID, "Can't upload shader parameter; the shader does not have a backing buffer set for the given location!"};
        }

        return transfer_memory_immediate({shader->bound_objects[actual_slot], location.byte_address, value.bytes.size(), memory_transfer_type::STREAMING_UPLOAD}, value.bytes.data());
    }

    status render_context::record_object_state(const object_identifier& identifier, object_state* state)
    {
        if (state == nullptr) return {status_type::UNEXPECTED, "Unexpected null state"};
        if (!objects.contains(state->object_type())) objects[state->object_type()] = {};
        if (objects[state->object_type()].contains(identifier.hash)) return {status_type::DUPLICATE, std::format("An object of this type with the name '{0}' already exists (or there is a hash collision)!", identifier.name)};
        objects[state->object_type()][identifier.hash] = state;
        return status_type::SUCCESS;
    }
}
