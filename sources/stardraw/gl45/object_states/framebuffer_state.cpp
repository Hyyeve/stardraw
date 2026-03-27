#include "framebuffer_state.hpp"

#include "texture_state.hpp"
#include "stardraw/gl45/api_conversion.hpp"
#include "tracy/Tracy.hpp"
#include "tracy/TracyOpenGL.hpp"

namespace stardraw::gl45
{
    framebuffer_state::framebuffer_state(const framebuffer& descriptor, status& out_status) : depth_format(0), stencil_format(0)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Create framebuffer object");

        glCreateFramebuffers(1, &gl_id);

        if (gl_id <= 0)
        {
            out_status = {status_type::BACKEND_ERROR, "Failed to create framebuffer object"};
            return;
        }

        out_status = status_type::SUCCESS;
    }

    framebuffer_state::~framebuffer_state()
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Delete framebuffer object");
        glDeleteFramebuffers(1, &gl_id);
    }

    status framebuffer_state::check_complete()
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Validate framebuffer");

        const GLenum status = glCheckNamedFramebufferStatus(gl_id, GL_FRAMEBUFFER);
        if (status == GL_FRAMEBUFFER_COMPLETE)
        {
            is_framebuffer_complete = true;
            return status_type::SUCCESS;
        }
        std::string status_string;
        switch (status)
        {
            case GL_FRAMEBUFFER_UNDEFINED:
            {
                status_string = "Framebuffer object not valid (this is a stardraw bug!)";
                break;
            }
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            {
                status_string = "Incomplete framebuffer attachments (this is a stardraw bug!)";
                break;
            }
            case GL_FRAMEBUFFER_UNSUPPORTED:
            {
                status_string = "This platform/driver doesn't support the provided framebuffer attachment combination";
                break;
            }
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            {
                status_string = "Framebuffer attachments have mismatched multisample settings (this is a stardraw bug!)";
                break;
            }
            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
            {
                status_string = "Framebuffer attachments have mismatched layer settings (this is a stardraw bug!)";
                break;
            }
            default:
            {
                status_string = "Unknown framebuffer error (this is probably a stardraw bug!)";
                break;
            }
        }

        is_framebuffer_complete = false;
        return {status_type::BACKEND_ERROR, status_string};
    }

    bool framebuffer_state::is_valid() const
    {
        for (const GLuint attached_texture : attached_textures)
        {
            if (!glIsTexture(attached_texture)) return false;
        }

        return gl_id != 0 && glIsFramebuffer(gl_id) && is_framebuffer_complete;
    }

    status framebuffer_state::attach_color_texture(const framebuffer_attachment_info& info, const texture_state* texture_state, const u32 attachment_index)
    {
        attached_color_formats[attachment_index] = texture_state->data_type;
        return attach_texture(info, GL_COLOR_ATTACHMENT0 + attachment_index, texture_state);
    }

    status framebuffer_state::attach_depth_texture(const framebuffer_attachment_info& info, const texture_state* texture_state)
    {
        depth_format = texture_state->gl_texture_format;
        return attach_texture(info, GL_DEPTH_ATTACHMENT, texture_state);
    }

    status framebuffer_state::attach_stencil_texture(const framebuffer_attachment_info& info, const texture_state* texture_state)
    {
        stencil_format = texture_state->gl_texture_format;
        return attach_texture(info, GL_STENCIL_ATTACHMENT, texture_state);
    }

    status framebuffer_state::attach_depth_stencil_texture(const framebuffer_attachment_info& info, const texture_state* texture_state)
    {
        depth_format = texture_state->gl_texture_format;
        stencil_format = texture_state->gl_texture_format;
        return attach_texture(info, GL_DEPTH_STENCIL_ATTACHMENT, texture_state);
    }

    status framebuffer_state::bind() const
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gl_id);
        return status_type::SUCCESS;
    }

    descriptor_type framebuffer_state::object_type() const
    {
        return descriptor_type::FRAMEBUFFER;
    }

    status framebuffer_state::blit_to(framebuffer_state* dest_state, const framebuffer_copy_info& info)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Framebuffer copy");

        if (info.components != attachment_components::COLOR && info.filtering != framebuffer_copy_filtering::NEAREST)
        {
            return {status_type::INVALID, "Cannot use linear filtering on depth/stencil buffer copies"};
        }

        if (info.components != attachment_components::COLOR && (depth_format != dest_state->depth_format || stencil_format != dest_state->stencil_format))
        {
            return {status_type::INVALID, "Read and write framebuffers must have the same depth/stencil formats for this copy"};
        }

        if (info.components != attachment_components::DEPTH && info.components != attachment_components::STENCIL)
        {
            if (!attached_color_formats.contains(info.read_color_attachment_index))
            {
                return {status_type::RANGE_OVERFLOW, "Read color attachment index has no attachment"};
            }

            if (!dest_state->attached_color_formats.contains(info.write_color_attachment_index))
            {
                return {status_type::RANGE_OVERFLOW, "Write color attachment index has no attachment"};
            }

            const texture_data_type source_data_type = attached_color_formats[info.read_color_attachment_index];
            const texture_data_type dest_data_type = dest_state->attached_color_formats[info.write_color_attachment_index];

            const bool is_source_color_integer = is_texture_data_type_integer(source_data_type);
            const bool is_dest_color_integer = is_texture_data_type_integer(dest_data_type);
            const bool is_mismatched_integer_formats = is_source_color_integer && is_dest_color_integer && (is_integer_texture_data_type_signed(source_data_type) != is_integer_texture_data_type_signed(dest_data_type));

            if (is_source_color_integer != is_dest_color_integer || is_mismatched_integer_formats)
            {
                return {status_type::INVALID, "Read and write color attachments have incompatible data types"};
            }

            glNamedFramebufferReadBuffer(gl_id, GL_COLOR_ATTACHMENT0 + info.read_color_attachment_index);
            glNamedFramebufferDrawBuffer(dest_state->gl_id, GL_COLOR_ATTACHMENT0 + info.write_color_attachment_index);
        }

        glBlitNamedFramebuffer(gl_id, dest_state->gl_id, info.read_x, info.read_y, info.read_x + info.read_width, info.read_y + info.read_height, info.write_x, info.write_y, info.write_x + info.write_width, info.write_y + info.write_height, to_gl_attachment_bitfield(info.components), to_gl_filter_mode(info.filtering));

        return status_type::SUCCESS;
    }

    status framebuffer_state::blit_to_default(const framebuffer_copy_info& info)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Framebuffer copy");

        if (info.components != attachment_components::COLOR)
        {
            return {status_type::INVALID, "Cannot blit non-color values to the default (window) framebuffer"};
        }

        if (!attached_color_formats.contains(info.read_color_attachment_index))
        {
            return {status_type::RANGE_OVERFLOW, "Read color attachment index has no attachment"};
        }

        if (info.write_color_attachment_index != 0)
        {
            return {status_type::RANGE_OVERFLOW, "Write color attachment index isn't valid. Only 0 is supported for copying to the default framebuffer"};
        }

        const texture_data_type source_data_type = attached_color_formats[info.read_color_attachment_index];
        const bool is_source_color_integer = is_texture_data_type_integer(source_data_type);

        if (is_source_color_integer)
        {
            return {status_type::INVALID, "Cannot copy integer data to the default framebuffer"};
        }

        glNamedFramebufferReadBuffer(gl_id, GL_COLOR_ATTACHMENT0 + info.read_color_attachment_index);
        glBlitNamedFramebuffer(gl_id, 0, info.read_x, info.read_y, info.read_x + info.read_width, info.read_y + info.read_height, info.write_x, info.write_y, info.write_x + info.write_width, info.write_y + info.write_height, to_gl_attachment_bitfield(info.components), to_gl_filter_mode(info.filtering));

        return status_type::SUCCESS;
    }

    status framebuffer_state::clear_depth(const f32 value) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Framebuffer clear (depth)");
        glClearNamedFramebufferfv(gl_id, GL_DEPTH, 0, &value);
        return status_type::SUCCESS;
    }

    status framebuffer_state::clear_stencil(const u32 value) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Framebuffer clear (stencil)");
        glClearNamedFramebufferuiv(gl_id, GL_STENCIL, 0, &value);
        return status_type::SUCCESS;
    }

    status framebuffer_state::clear_color(u32 attachment, const clear_values& values)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Framebuffer clear (color)");
        if (!attached_color_formats.contains(attachment))
        {
            return {status_type::INVALID, std::format("Can't clear attachment; no color attachment with index {0}", attachment)};
        }

        const texture_data_type data_format = attached_color_formats[attachment];
        if (is_texture_data_type_integer(data_format))
        {
            if (is_integer_texture_data_type_signed(data_format))
            {
                glClearNamedFramebufferiv(gl_id, GL_COLOR, GL_COLOR_ATTACHMENT0 + attachment, reinterpret_cast<const GLint*>(values.channels.data.data()));
            }
            else
            {
                glClearNamedFramebufferuiv(gl_id, GL_COLOR, GL_COLOR_ATTACHMENT0 + attachment, reinterpret_cast<const GLuint*>(values.channels.data.data()));
            }
        }
        else
        {
            glClearNamedFramebufferfv(gl_id, GL_COLOR, GL_COLOR_ATTACHMENT0 + attachment, reinterpret_cast<const GLfloat*>(values.channels.data.data()));
        }

        return status_type::SUCCESS;
    }

    status framebuffer_state::attach_texture(const framebuffer_attachment_info& info, const GLenum attachment, const texture_state* texture_state)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Attach framebuffer texture");

        if (info.layered)
        {
            glNamedFramebufferTexture(gl_id, attachment, texture_state->gl_texture_id, info.mipmap_level);
        }
        else
        {
            glNamedFramebufferTextureLayer(gl_id, attachment, texture_state->gl_texture_id, info.mipmap_level, info.layer);
        }

        attached_textures.push_back(texture_state->gl_texture_id);

        return status_type::SUCCESS;
    }
}
