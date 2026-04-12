#include "render_context.hpp"

#include <format>

#include "api_conversion.hpp"
#include "object_states/framebuffer_state.hpp"
#include "object_states/texture_sampler_state.hpp"
#include "stardraw/internal/internal.hpp"
#include "starlib/utility/string.hpp"

namespace stardraw::gl45
{
    [[nodiscard]] status render_context::create_buffer_state(const buffer* descriptor)
    {
        ZoneScoped;
        status create_status = status_type::SUCCESS;
        buffer_state* buffer = new buffer_state(*descriptor, create_status);
        if (create_status.is_error())
        {
            delete buffer;
            return create_status;
        }

        return record_object_state(descriptor->identifier(), buffer);
    }

    status render_context::create_shader_state(const shader* descriptor)
    {
        ZoneScoped;
        status shader_create_status = status_type::SUCCESS;
        shader_state* shader = new shader_state(*descriptor, shader_create_status);
        if (shader_create_status.is_error())
        {
            delete shader;
            return shader_create_status;
        }

        return record_object_state(descriptor->identifier(), shader);
    }

    status render_context::create_texture_state(const texture* descriptor)
    {
        ZoneScoped;
        status texture_create_status = status_type::SUCCESS;

        texture_state* texture;

        if (descriptor->as_view_of.has_value())
        {
            texture_state* original;
            status find_status = find_texture_state(descriptor->as_view_of.value(), &original);
            if (find_status.is_error()) return find_status;
            texture = new texture_state(original, *descriptor, texture_create_status);
        }
        else
        {
            texture = new texture_state(*descriptor, texture_create_status);
        }

        if (texture_create_status.is_error())
        {
            delete texture;
            return texture_create_status;
        }

        return record_object_state(descriptor->identifier(), texture);
    }

    status render_context::create_texture_sampler_state(const sampler* descriptor)
    {
        ZoneScoped;
        status create_status = status_type::SUCCESS;
        texture_sampler_state* sampler_state = new texture_sampler_state(*descriptor, create_status);
        if (create_status.is_error())
        {
            delete sampler_state;
            return create_status;
        }
        return record_object_state(descriptor->identifier(), sampler_state);
    }

    status render_context::find_and_validate_attachment_texture(const framebuffer_attachment_info& attachment, u32& lowest_msaa_level, u32& highest_msaa_level, bool& any_texture_layered, bool& any_texture_not_layered, texture_state** texture_out)
    {
        texture_state* texture;
        status find_status = find_texture_state(attachment.texture, &texture);
        if (find_status.is_error()) return find_status;

        *texture_out = texture;

        if (attachment.layered) any_texture_layered = true;
        else any_texture_not_layered = true;
        if (texture->num_texture_msaa_samples < lowest_msaa_level) lowest_msaa_level = texture->num_texture_msaa_samples;
        if (texture->num_texture_msaa_samples > highest_msaa_level) highest_msaa_level = texture->num_texture_msaa_samples;
        if (texture->num_texture_msaa_samples > GL_MAX_FRAMEBUFFER_SAMPLES) return {status_type::INVALID, std::format("Texture multisample count is too high on texture '{0}'", attachment.texture.name)};
        if (attachment.layer > GL_MAX_FRAMEBUFFER_LAYERS) return {status_type::INVALID, std::format("Texture attachment layer is too high on texture '{0}'", attachment.texture.name)};
        if (attachment.layer >= texture->num_texture_array_layers) return {status_type::INVALID, std::format("Texture attachment layer is outside the bounds of the texture object '{0}'", attachment.texture.name)};
        if (attachment.mipmap_level >= texture->num_texture_mipmap_levels) return {status_type::INVALID, std::format("Texture attachment mipmap level is not a valid mipmap level in the texture object '{0}'", attachment.texture.name)};
        return status_type::SUCCESS;
    }


    status render_context::create_framebuffer_state(const framebuffer* descriptor)
    {
        ZoneScoped;

        if (descriptor->color_attachments.size() > 32) return {status_type::UNSUPPORTED, std::format("Maximum number of color attachments in a framebuffer is 32. (requested {0} for framebuffer '{1}'", descriptor->color_attachments.size(), descriptor->identifier().name)};

        bool any_texture_not_layered = false;
        bool any_texture_layered = false;

        u32 highest_msaa_level = 0;
        u32 lowest_msaa_level = u32_max;

        std::vector<texture_state*> color_textures;
        for (const framebuffer_attachment_info& attachment : descriptor->color_attachments)
        {
            texture_state* texture;
            status validate_status = find_and_validate_attachment_texture(attachment,  lowest_msaa_level, highest_msaa_level, any_texture_layered, any_texture_not_layered, &texture);
            if (validate_status.is_error()) return validate_status;
            if (!is_texture_data_type_color_renderable(texture->data_type)) return {status_type::INVALID, std::format("Texture data format of '{0}' cannot be uesd as a framebuffer color attachment. (for framebuffer '{1}')", attachment.texture.name, descriptor->identifier().name)};
            color_textures.push_back(texture);
        }

        texture_state* depth_texture = nullptr;
        if (descriptor->depth_attachment.has_value())
        {
            const framebuffer_attachment_info& attachment = descriptor->depth_attachment.value();
            status validate_status = find_and_validate_attachment_texture(attachment,  lowest_msaa_level, highest_msaa_level, any_texture_layered, any_texture_not_layered, &depth_texture);
            if (validate_status.is_error()) return validate_status;
            if (!is_texture_data_type_depth_renderable(depth_texture->data_type)) return {status_type::INVALID, std::format("Texture data format of '{0}' cannot be uesd as a framebuffer depth attachment. (for framebuffer '{1}')", attachment.texture.name, descriptor->identifier().name)};
        }

        texture_state* stencil_texture = nullptr;
        if (descriptor->stencil_attachment.has_value())
        {
            const framebuffer_attachment_info& attachment = descriptor->stencil_attachment.value();
            status validate_status = find_and_validate_attachment_texture(attachment,  lowest_msaa_level, highest_msaa_level, any_texture_layered, any_texture_not_layered, &stencil_texture);
            if (validate_status.is_error()) return validate_status;
            if (!is_texture_data_type_stencil_renderable(stencil_texture->data_type)) return {status_type::INVALID, std::format("Texture data format of '{0}' cannot be uesd as a framebuffer stencil attachment. (for framebuffer '{1}')", attachment.texture.name, descriptor->identifier().name)};
        }

        if (lowest_msaa_level != highest_msaa_level) return {status_type::INVALID, std::format("Cannot mix textures with different MSAA levels in framebuffer attachments for framebuffer '{0}'", descriptor->identifier().name)};
        if (any_texture_layered && any_texture_not_layered) return {status_type::INVALID, std::format("Cannot mix layered and non-layered textures in framebuffer attachments for framebuffer '{0}'", descriptor->identifier().name)};

        status create_status = status_type::SUCCESS;
        framebuffer_state* fbuff_state = new framebuffer_state(*descriptor, create_status);
        if (create_status.is_error())
        {
            delete fbuff_state;
            return create_status;
        }

        for (u32 idx = 0; idx < descriptor->color_attachments.size(); idx++)
        {
            const framebuffer_attachment_info& attachment = descriptor->color_attachments[idx];
            const texture_state* texture = color_textures[idx];
            status attach_status = fbuff_state->attach_color_texture(attachment, texture, idx);
            if (attach_status.is_error())
            {
                delete fbuff_state;
                return attach_status;
            }
        }

        if (depth_texture != nullptr && depth_texture == stencil_texture)
        {
            status attach_status = fbuff_state->attach_depth_stencil_texture(descriptor->depth_attachment.value(), depth_texture);
            if (attach_status.is_error())
            {
                delete fbuff_state;
                return attach_status;
            }
            return record_object_state(descriptor->identifier(), fbuff_state);
        }

        //Non-combined depth/stencil

        if (depth_texture != nullptr)
        {
            status attach_status = fbuff_state->attach_depth_texture(descriptor->depth_attachment.value(), depth_texture);
            if (attach_status.is_error())
            {
                delete fbuff_state;
                return attach_status;
            }
        }

        if (stencil_texture != nullptr)
        {
            status attach_status = fbuff_state->attach_stencil_texture(descriptor->stencil_attachment.value(), stencil_texture);
            if (attach_status.is_error())
            {
                delete fbuff_state;
                return attach_status;
            }
        }

        status validate_status = fbuff_state->check_complete();
        if (validate_status.is_error())
        {
            delete fbuff_state;
            return validate_status;
        }

        return record_object_state(descriptor->identifier(), fbuff_state);
    }

    status render_context::create_vertex_specification_state(const vertex_configuration* descriptor)
    {
        ZoneScoped;
        vertex_specification_state* vertex_spec = new vertex_specification_state();
        if (vertex_spec->vertex_array_id == 0)
        {
            delete vertex_spec;
            return {status_type::BACKEND_ERROR, std::format("Attempting to create vertex specification '{0}' resulted in an invalid object", descriptor->identifier().name)};
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

            {
                ZoneScopedN("GL calls");
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

    status render_context::create_draw_specification_state(const draw_configuration* descriptor)
    {
        ZoneScoped;
        vertex_specification_state* vertex_spec;
        status v_find_status = find_vertex_specification_state(descriptor->vertex_specification, &vertex_spec);
        if (v_find_status.is_error()) return v_find_status;

        shader_state* shader;
        status find_status = find_shader_state(descriptor->shader, &shader);
        if (find_status.is_error()) return find_status;

        //draw specification is a thin wrapper that references shader and vertex specifications
        return record_object_state(descriptor->identifier(), new draw_specification_state(*descriptor, vertex_spec->has_index_buffer));
    }

    status render_context::create_transfer_buffer_state(const transfer_buffer* descriptor)
    {
        ZoneScoped;
        status create_status = status_type::SUCCESS;
        transfer_buffer_state* state = new transfer_buffer_state(*descriptor, create_status);
        if (create_status.is_error())
        {
            delete state;
            return create_status;
        }

        return record_object_state(descriptor->identifier(), state);
    }

    status render_context::record_object_state(const object_identifier& identifier, object_state* state)
    {
        ZoneScoped;
        if (state == nullptr) return {status_type::UNEXPECTED, std::format("Unexpected null state while trying to find object '{0}'", identifier.name)};
        if (!objects.contains(state->object_type())) objects[state->object_type()] = {};
        if (objects[state->object_type()].contains(identifier.hash)) return {status_type::DUPLICATE, std::format("An object of this type with the name '{0}' already exists (or there is a hash collision)!", identifier.name)};
        objects[state->object_type()][identifier.hash] = state;
        return status_type::SUCCESS;
    }
}