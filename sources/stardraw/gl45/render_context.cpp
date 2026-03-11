#include "render_context.hpp"
#include "window.hpp"

#include <format>

#include "api_conversion.hpp"
#include "stardraw/internal/internal.hpp"

namespace stardraw::gl45
{
    render_context::render_context(window* window, const bool api_validation, const bool backend_validation) : parent_window(window), api_validation_enabled(api_validation), backend_validation_enabled(backend_validation)
    {

    }

    [[nodiscard]] status render_context::execute_command_buffer(const std::string_view& name)
    {
        status context_status = parent_window->make_gl_context_active();
        if (context_status.is_error()) return context_status;

        //Opengl doesn't have any persistant command buffers, so we just execute it like a temporary one without consuming it.
        if (!command_lists.contains(std::string(name))) return status_type::UNKNOWN;
        const command_list& refren = command_lists[std::string(name)];

        for (const starlib::polymorphic<command>& cmd : refren)
        {
            const status result = execute_command(cmd.ptr());
            if (result.is_error()) return result;
        }

        return status_from_last_gl_error();
    }

    [[nodiscard]] status render_context::execute_temp_command_buffer(const command_list&& commands)
    {
        status context_status = parent_window->make_gl_context_active();
        if (context_status.is_error()) return context_status;

        for (const starlib::polymorphic<command>& cmd : commands)
        {
            const status result = execute_command(cmd.ptr());
            if (result.is_error()) return result;
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
        if (context_status.is_error()) return context_status;

        for (const starlib::polymorphic<descriptor>& descriptor : descriptors)
        {
            const status create_status = create_object(descriptor.ptr());
            if (create_status.is_error()) return create_status;
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
        if (context_status.is_error()) return signal_status::CONTEXT_ERROR;

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

    status render_context::prepare_buffer_memory_transfer(const buffer_memory_transfer_info& info, memory_transfer_handle** out_handle)
    {
        buffer_state* buffer;
        const status find_status = find_buffer_state(info.target, &buffer);
        if (find_status.is_error()) return find_status;

        switch (info.transfer_type)
        {
            case buffer_memory_transfer_info::type::UPLOAD_STREAMING:
            {
                memory_transfer_handle* handle;
                status prepare_status = buffer->prepare_upload_data_streaming(info.address, info.bytes, &handle);
                if (prepare_status.is_error()) return prepare_status;
                buffer_transfers[handle] = info;
                *out_handle = handle;
                return status_type::SUCCESS;
            }
            case buffer_memory_transfer_info::type::UPLOAD_CHUNK:
            {
                memory_transfer_handle* handle;
                status prepare_status = buffer->prepare_upload_data_chunked(info.address, info.bytes, &handle);
                if (prepare_status.is_error()) return prepare_status;
                buffer_transfers[handle] = info;
                *out_handle = handle;
                return status_type::SUCCESS;
            }
            case buffer_memory_transfer_info::type::UPLOAD_UNCHECKED:
            {
                memory_transfer_handle* handle;
                status prepare_status = buffer->prepare_upload_data_unchecked(info.address, info.bytes, &handle);
                if (prepare_status.is_error()) return prepare_status;
                buffer_transfers[handle] = info;
                *out_handle = handle;
                return status_type::SUCCESS;
            }
            default: return {status_type::UNSUPPORTED};
        }
    }

    status render_context::flush_buffer_memory_transfer(memory_transfer_handle* handle)
    {
        if (!buffer_transfers.contains(handle)) return {status_type::UNKNOWN, "Memory transfer handle not recognized - did you create it with a different context or type?"};
        const buffer_memory_transfer_info info = buffer_transfers[handle];
        buffer_transfers.erase(handle);

        buffer_state* buffer;
        const status find_status = find_buffer_state(info.target, &buffer);
        if (find_status.is_error()) return find_status;

        mem_barrier_controller.barrier_if_needed(info.target, GL_BUFFER_UPDATE_BARRIER_BIT);

        switch (info.transfer_type)
        {
            case buffer_memory_transfer_info::type::UPLOAD_STREAMING: return buffer->flush_upload_data_streaming(handle);
            case buffer_memory_transfer_info::type::UPLOAD_CHUNK: return buffer->flush_upload_data_chunked(handle);
            case buffer_memory_transfer_info::type::UPLOAD_UNCHECKED: return buffer->flush_upload_data_unchecked(handle);

            default: return {status_type::UNSUPPORTED};
        }
    }

    status render_context::prepare_texture_memory_transfer(const texture_memory_transfer_info& info, memory_transfer_handle** out_handle)
    {
        texture_state* texture;
        status find_status = find_texture_state(object_identifier(info.target), &texture);
        if (find_status.is_error()) return find_status;

        memory_transfer_handle* handle;
        status prepare_status = texture->prepare_upload(info, &handle);
        if (prepare_status.is_error()) return prepare_status;
        texture_transfers[handle] = info;
        *out_handle = handle;
        return status_type::SUCCESS;
    }

    status render_context::flush_texture_memory_transfer(memory_transfer_handle* handle)
    {
        if (!texture_transfers.contains(handle)) return {status_type::UNKNOWN, "Memory transfer handle not recognized - did you create it with a different context or type?"};
        const texture_memory_transfer_info info = texture_transfers[handle];
        texture_transfers.erase(handle);

        texture_state* texture;
        status find_status = find_texture_state(info.target, &texture);
        if (find_status.is_error()) return find_status;

        mem_barrier_controller.barrier_if_needed(info.target, GL_TEXTURE_UPDATE_BARRIER_BIT);

        return texture->flush_upload(info, handle);
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
            case command_type::TEXTURE_COPY: return execute_texture_copy(dynamic_cast<const texture_copy_command*>(cmd));

            case command_type::CLEAR_WINDOW: return execute_clear_window(dynamic_cast<const clear_window_command*>(cmd));
            case command_type::CLEAR_BUFFER: return status_type::UNIMPLEMENTED; //TODO
            case command_type::CONFIG_SHADER: return execute_shader_parameters_upload(dynamic_cast<const shader_config_command*>(cmd));
            case command_type::SIGNAL: return execute_signal(dynamic_cast<const signal_command*>(cmd));
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
        if (shader_create_status.is_error())
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
        if (texture_create_status.is_error())
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
            const std::string& buffer_name = element.buffer.name;
            if (buffer_slots.contains(buffer_name)) continue;
            buffer_slots[buffer_name] = buffer_slot;
            buffer_names.push_back(buffer_name);

            buffer_state* buffer_state;
            const status find_status = find_buffer_state(object_identifier(buffer_name), &buffer_state);
            if (find_status.is_error())
            {
                delete vertex_spec;
                return find_status;
            }
            buffer_states[buffer_name] = buffer_state;
            buffer_slot++;
        }

        buffer_strides.resize(buffer_slot);

        const GLuint vao_id = vertex_spec->vertex_array_id;

        for (u32 idx = 0; idx < format.bindings.size(); idx++)
        {
            const vertex_data_binding& elem = format.bindings[idx];
            GLsizeiptr& offset = buffer_strides[buffer_slots[elem.buffer.name]];

            glEnableVertexArrayAttrib(vao_id, idx);
            glVertexArrayAttribBinding(vao_id, idx, buffer_slots[elem.buffer.name]);
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
            const status attach_status = vertex_spec->attach_vertex_buffer(object_identifier(vertex_buffer), buffer_slots[vertex_buffer], buffer_state->gl_id(), 0, buffer_strides[buffer_slots[vertex_buffer]]);

            if (attach_status.is_error())
            {
                delete vertex_spec;
                return attach_status;
            }
        }

        if (descriptor->has_index_buffer)
        {
            buffer_state* index_buffer_state;
            const status find_status = find_buffer_state(descriptor->index_buffer, &index_buffer_state);
            if (find_status.is_error())
            {
                delete vertex_spec;
                return {status_type::UNKNOWN, std::format("No buffer named '{0}' found while creating vertex specification '{1}'", descriptor->index_buffer.name, descriptor->identifier().name)};
            }

            const status attach_status = vertex_spec->attach_index_buffer(descriptor->index_buffer, index_buffer_state->gl_id());

            if (attach_status.is_error())
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
        vertex_specification_state* vertex_spec;
        const status v_find_status = find_vertex_specification_state(descriptor->vertex_specification, &vertex_spec);
        if (v_find_status.is_error()) return v_find_status;

        shader_state* shader;
        status find_status = find_shader_state(descriptor->shader, &shader);
        if (find_status.is_error()) return find_status;

        //draw specification is a thin wrapper that references shader and vertex specifications
        return record_object_state(descriptor->identifier(), new draw_specification_state(*descriptor, vertex_spec->has_index_buffer));
    }

    status render_context::bind_vertex_specification_state(const object_identifier& source)
    {
        vertex_specification_state* vertex_spec;
        const status v_find_status = find_vertex_specification_state(source, &vertex_spec);
        if (v_find_status.is_error()) return v_find_status;
        return vertex_spec->bind();
    }

    status render_context::bind_draw_specification_state(const object_identifier& source)
    {
        draw_specification_state* state;
        const status find_status = find_draw_specification_state(source, &state);
        if (find_status.is_error()) return find_status;

        status vertex_specification_bind = bind_vertex_specification_state(state->vertex_specification);
        if (vertex_specification_bind.is_error()) return vertex_specification_bind;

        status shader_bind = bind_shader(state->shader);
        if (shader_bind.is_error()) return shader_bind;

        active_draw_specification = state;

        return status_type::SUCCESS;
    }

    [[nodiscard]] status render_context::bind_buffer(const object_identifier& source, const GLenum target)
    {
        buffer_state* buffer_state;
        const status find_status = find_buffer_state(source, &buffer_state);
        if (find_status.is_error()) return find_status;
        return buffer_state->bind_to(target);
    }

    status render_context::bind_shader(const object_identifier& source)
    {
        shader_state* shader;
        status find_status = find_shader_state(source, &shader);
        if (find_status.is_error()) return find_status;

        status activate_status = shader->make_active();
        if (activate_status.is_error()) return activate_status;

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
                case shader_parameter_value::value_type::TEXTURE_REFERENCE:
                {
                    result_status = bind_shader_texture_parameter(shader, location, value, false);
                    break;
                }
                case shader_parameter_value::value_type::IMAGE_REFERENCE:
                {
                    result_status = bind_shader_texture_parameter(shader, location, value, true);
                    break;
                }
                default:
                {
                    result_status = bind_shader_data_parameter(shader, location, value);
                    break;
                }
            }

            if (result_status.is_error()) return result_status;
        }

        return status_type::SUCCESS;
    }

    status render_context::bind_shader_texture_parameter(shader_state* shader, const shader_parameter_location& location, const shader_parameter_value& value, const bool as_image = false)
    {
        const binding_location_info binding_info = vk_binding_for_location(location);
        const u32 actual_slot = binding_info.slot + shader->descriptor_set_binding_offsets[binding_info.set];

        //To bind a texture, we make sure the location is *explicitly* pointed at the texture variable, not something contained inside the texture.
        if (binding_info.binding_type != location.offset_ptr)
        {
            return {status_type::UNSUPPORTED, "The shader parameter location provided cannot have a texture bound to it!"};
        }

        texture_shape resource_shape;

        switch (binding_info.binding_type->getKind())
        {
            case slang::TypeReflection::Kind::Resource:
            {
                const u32 shape = binding_info.binding_type->getResourceShape() & ~SlangResourceShape::SLANG_TEXTURE_COMBINED_FLAG;
                switch (shape)
                {
                    case SlangResourceShape::SLANG_TEXTURE_1D_ARRAY:
                    case SlangResourceShape::SLANG_TEXTURE_1D:
                    {
                        resource_shape = texture_shape::_1D;
                        break;
                    }
                    case SlangResourceShape::SLANG_TEXTURE_2D_ARRAY:
                    case SlangResourceShape::SLANG_TEXTURE_2D:
                    {
                        resource_shape = texture_shape::_2D;
                        break;
                    }
                    case SlangResourceShape::SLANG_TEXTURE_CUBE_ARRAY:
                    case SlangResourceShape::SLANG_TEXTURE_CUBE:
                    {
                        resource_shape = texture_shape::CUBE_MAP;
                        break;
                    }
                    case SlangResourceShape::SLANG_TEXTURE_3D:
                    {
                        resource_shape = texture_shape::_3D;
                        break;
                    }
                    default:
                    {
                        return {status_type::UNSUPPORTED, "The shader parameter location provided cannot have a texture bound to it!"};
                    }
                }
                break;
            }
            default:
            {
                return {status_type::UNSUPPORTED, "The shader parameter location provided cannot have a texture bound to it!"};
            }
        }

        texture_state* texture;
        status find_status = find_texture_state(value.opaque_reference, &texture);
        if (find_status.is_error()) return find_status;
        if (texture->get_shape() != resource_shape) return {status_type::INVALID, std::format("Texture object '{0}' can't be bound to this location - wrong texture shape!", value.opaque_reference.name)};

        status bind_status = status_type::SUCCESS;
        const SlangResourceAccess access = binding_info.binding_type->getResourceAccess();
        GLenum gl_access = GL_READ_WRITE;
        if (access == SLANG_RESOURCE_ACCESS_READ) gl_access = GL_READ_ONLY;
        else if (access == SLANG_RESOURCE_ACCESS_WRITE) gl_access = GL_WRITE_ONLY;

        if (as_image)
        {
            bind_status = texture->bind_to_image_slot(actual_slot, value.image_texture_mipmap, value.image_texture_layer, value.image_texture_array, gl_access);
        }
        else
        {
            bind_status = texture->bind_to_texture_slot(actual_slot);
        }

        if (bind_status.is_error()) return bind_status;
        shader->bound_objects[actual_slot] = { value.opaque_reference, as_image && gl_access != GL_READ_ONLY, GL_TEXTURE_FETCH_BARRIER_BIT };
        return status_type::SUCCESS;
    }

    status render_context::bind_shader_buffer_parameter(shader_state* shader, const shader_parameter_location& location, const shader_parameter_value& value)
    {
        const binding_location_info binding_info = vk_binding_for_location(location);
        const u32 actual_slot = binding_info.slot + shader->descriptor_set_binding_offsets[binding_info.set];
        GLenum binding_type = 0;
        SlangResourceAccess access;

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
                access = SLANG_RESOURCE_ACCESS_READ;
                break;
            }

            case slang::TypeReflection::Kind::ShaderStorageBuffer:
            {
                binding_type = GL_SHADER_STORAGE_BUFFER;
                access = SLANG_RESOURCE_ACCESS_READ_WRITE;
                break;
            }

            case slang::TypeReflection::Kind::Resource:
            {
                const SlangResourceShape shape = binding_info.binding_type->getResourceShape();
                access = binding_info.binding_type->getResourceAccess();
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

        buffer_state* buffer;
        const status find_status = find_buffer_state(object_identifier(value.opaque_reference), &buffer);
        if (find_status.is_error()) return find_status;

        status bind_status = buffer->bind_to_slot(binding_type, actual_slot);
        const GLbitfield read_barrier = (binding_type == GL_SHADER_STORAGE_BUFFER ? GL_SHADER_STORAGE_BARRIER_BIT : GL_UNIFORM_BARRIER_BIT);
        const bool has_write_access = access != SLANG_RESOURCE_ACCESS_READ;
        shader->bound_objects[actual_slot] = { value.opaque_reference, has_write_access, read_barrier };
        if (bind_status.is_error()) return bind_status;
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

        return transfer_buffer_memory_immediate({shader->bound_objects[actual_slot].identifier, location.byte_address, value.bytes.size(), buffer_memory_transfer_info::type::UPLOAD_STREAMING}, value.bytes.data());
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
