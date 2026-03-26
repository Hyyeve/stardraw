#include "framebuffer_state.hpp"

#include "texture_state.hpp"
#include "tracy/Tracy.hpp"
#include "tracy/TracyOpenGL.hpp"

namespace stardraw::gl45
{
    framebuffer_state::framebuffer_state(const framebuffer& descriptor, status& out_status)
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
        return attach_texture(info, GL_COLOR_ATTACHMENT0 + attachment_index, texture_state);
    }

    status framebuffer_state::attach_depth_texture(const framebuffer_attachment_info& info, const texture_state* texture_state)
    {
        return attach_texture(info, GL_DEPTH_ATTACHMENT, texture_state);
    }

    status framebuffer_state::attach_stencil_texture(const framebuffer_attachment_info& info, const texture_state* texture_state)
    {
        return attach_texture(info, GL_STENCIL_ATTACHMENT, texture_state);
    }

    status framebuffer_state::attach_depth_stencil_texture(const framebuffer_attachment_info& info, const texture_state* texture_state)
    {
        return attach_texture(info, GL_DEPTH_STENCIL_ATTACHMENT, texture_state);
    }

    descriptor_type framebuffer_state::object_type() const
    {
        return descriptor_type::FRAMEBUFFER;
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
