#include "render_context.hpp"

#include <format>

#include "api_conversion.hpp"
#include "stardraw/internal/internal.hpp"
#include "starlib/general/string.hpp"
#include "tracy/TracyOpenGL.hpp"

namespace stardraw::gl45
{
    status render_context::bind_vertex_specification_state(const object_identifier& source)
    {
        ZoneScoped;
        vertex_specification_state* vertex_spec;
        status v_find_status = find_vertex_specification_state(source, &vertex_spec);
        if (v_find_status.is_error()) return v_find_status;
        return vertex_spec->bind();
    }

    status render_context::bind_draw_specification_state(const object_identifier& source)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Bind draw configuration");
        draw_specification_state* state;
        status find_status = find_draw_specification_state(source, &state);
        if (find_status.is_error()) return find_status;

        status vertex_specification_bind = bind_vertex_specification_state(state->vertex_specification);
        if (vertex_specification_bind.is_error()) return vertex_specification_bind;

        status shader_bind = bind_shader(state->shader);
        if (shader_bind.is_error()) return shader_bind;

        if (state->framebuffer.has_value())
        {
            framebuffer_state* framebuffer;
            status framebuffer_find = find_framebuffer_state(state->framebuffer.value(), &framebuffer);
            if (framebuffer_find.is_error()) return framebuffer_find;

            status bind_status = framebuffer->bind();
            if (bind_status.is_error()) return bind_status;
        }
        else
        {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        }

        active_draw_specification = state;

        return status_type::SUCCESS;
    }

    [[nodiscard]] status render_context::bind_buffer(const object_identifier& source, const GLenum target)
    {
        ZoneScoped;
        buffer_state* buffer_state;
        status find_status = find_buffer_state(source, &buffer_state);
        if (find_status.is_error()) return find_status;
        return buffer_state->bind_to(target);
    }

    status render_context::bind_shader(const object_identifier& source)
    {
        ZoneScoped;
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
                case shader_parameter_value::value_type::SAMPLER_REFERENCE:
                {
                    result_status = bind_shader_sampler_parameter(shader, location, value);
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

    status render_context::bind_shader_sampler_parameter(const shader_state* shader, const shader_parameter_location& location, const shader_parameter_value& value)
    {
        ZoneScoped;
        const binding_location_info binding_info = vk_binding_for_location(location);
        const u32 actual_slot = binding_info.slot + shader->descriptor_set_binding_offsets[binding_info.set];

        //To bind a sampler, we make sure the location is *explicitly* pointed at the texture variable, not something contained inside the texture.
        if (binding_info.binding_type_ptr != location.internal->offset_ptr)
        {
            return {status_type::INVALID, std::format("The shader parameter location '{0}' cannot have a sampler bound to it!", location.internal->path_string)};
        }

        texture_sampler_state* sampler;
        status find_status = find_texture_sampler_state(value.opaque_reference, &sampler);
        if (find_status.is_error()) return find_status;

        return sampler->bind(actual_slot);
    }

    status render_context::bind_shader_texture_parameter(shader_state* shader, const shader_parameter_location& location, const shader_parameter_value& value, const bool as_image = false)
    {
        ZoneScoped;
        const binding_location_info binding_info = vk_binding_for_location(location);
        const u32 actual_slot = binding_info.slot + shader->descriptor_set_binding_offsets[binding_info.set];

        //To bind a texture, we make sure the location is *explicitly* pointed at the texture variable, not something contained inside the texture.
        if (binding_info.binding_type_ptr != location.internal->offset_ptr)
        {
            return {status_type::INVALID, std::format("The shader parameter location '{0}' cannot have a texture bound to it!", location.internal->path_string)};
        }

        texture_shape resource_shape;

        switch (binding_info.binding_type_ptr->getKind())
        {
            case slang::TypeReflection::Kind::Resource:
            {
                const u32 shape = binding_info.binding_type_ptr->getResourceShape() & ~SlangResourceShape::SLANG_TEXTURE_COMBINED_FLAG;
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
                        return {status_type::INVALID, std::format("The shader parameter location '{0}' cannot have a texture bound to it!", location.internal->path_string)};
                    }
                }
                break;
            }
            default:
            {
                return {status_type::INVALID, std::format("The shader parameter location '{0}' cannot have a texture bound to it!", location.internal->path_string)};
            }
        }

        texture_state* texture;
        status find_status = find_texture_state(value.opaque_reference, &texture);
        if (find_status.is_error()) return find_status;
        if (texture->get_shape() != resource_shape) return {status_type::INVALID, std::format("Texture object '{0}' can't be bound to location '{1}' - wrong texture shape!", value.opaque_reference.name, location.internal->path_string)};

        status bind_status = status_type::SUCCESS;
        const SlangResourceAccess access = binding_info.binding_type_ptr->getResourceAccess();
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
        shader->bound_objects[actual_slot] = {value.opaque_reference, as_image && gl_access != GL_READ_ONLY, GL_TEXTURE_FETCH_BARRIER_BIT};
        return status_type::SUCCESS;
    }

    status render_context::bind_shader_buffer_parameter(shader_state* shader, const shader_parameter_location& location, const shader_parameter_value& value)
    {
        ZoneScoped;
        const binding_location_info binding_info = vk_binding_for_location(location);
        const u32 actual_slot = binding_info.slot + shader->descriptor_set_binding_offsets[binding_info.set];
        GLenum binding_type = 0;
        SlangResourceAccess access;

        //To bind a buffer, we make sure the location is *explicitly* pointed at the buffer variable, not something contained inside the buffer.
        if (binding_info.binding_type_ptr != location.internal->offset_ptr)
        {
            return {status_type::INVALID, std::format("The shader parameter location '{0}' cannot have a buffer bound to it!", location.internal->path_string)};
        }

        switch (binding_info.binding_type_ptr->getKind())
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
                const SlangResourceShape shape = binding_info.binding_type_ptr->getResourceShape();
                access = binding_info.binding_type_ptr->getResourceAccess();
                if (shape == SLANG_STRUCTURED_BUFFER || shape == SLANG_BYTE_ADDRESS_BUFFER)
                {
                    binding_type = GL_SHADER_STORAGE_BUFFER;
                    break;
                }
                //fallthrough to unsupported
            }
            default:
            {
                return {status_type::INVALID, std::format("The shader parameter location '{0}' cannot have a buffer bound to it!", location.internal->path_string)};
            }
        }

        buffer_state* buffer;
        status find_status = find_buffer_state(object_identifier(value.opaque_reference), &buffer);
        if (find_status.is_error()) return find_status;

        status bind_status = buffer->bind_to_slot(binding_type, actual_slot);
        const GLbitfield read_barrier = (binding_type == GL_SHADER_STORAGE_BUFFER ? GL_SHADER_STORAGE_BARRIER_BIT : GL_UNIFORM_BARRIER_BIT);
        const bool has_write_access = access != SLANG_RESOURCE_ACCESS_READ;
        shader->bound_objects[actual_slot] = {value.opaque_reference, has_write_access, read_barrier};
        if (bind_status.is_error()) return bind_status;
        return status_type::SUCCESS;
    }

    status render_context::bind_shader_data_parameter(shader_state* shader, const shader_parameter_location& location, shader_parameter_value& value)
    {
        ZoneScoped;
        const binding_location_info binding_info = vk_binding_for_location(location);
        const u32 actual_slot = binding_info.slot + shader->descriptor_set_binding_offsets[binding_info.set];

        if (!shader->bound_objects.contains(actual_slot))
        {
            return {status_type::INVALID, std::format("Can't apply shader parameter value; the shader does not have a buffer bound to store data at '{0}'", location.internal->path_string)};
        }

        return transfer_buffer_memory_immediate({shader->bound_objects[actual_slot].identifier, location.internal->byte_address, value.bytes.size(), buffer_memory_transfer_info::type::UPLOAD_STREAMING}, value.bytes.data());
    }
}
