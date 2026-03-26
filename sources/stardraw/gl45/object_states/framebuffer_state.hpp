#pragma once
#include "texture_state.hpp"
#include "stardraw/gl45/common.hpp"

namespace stardraw::gl45
{
    class framebuffer_state final : public object_state
    {
    public:
        explicit framebuffer_state(const framebuffer_descriptor& descriptor, status& out_status);
        ~framebuffer_state() override;
        [[nodiscard]] status check_complete();
        [[nodiscard]] bool is_valid() const;
        [[nodiscard]] status attach_color_texture(const framebuffer_attachment_info& info, const texture_state* texture_state, u32 attachment_index);
        [[nodiscard]] status attach_depth_texture(const framebuffer_attachment_info& info, const texture_state* texture_state);
        [[nodiscard]] status attach_stencil_texture(const framebuffer_attachment_info& info, const texture_state* texture_state);
        [[nodiscard]] status attach_depth_stencil_texture(const framebuffer_attachment_info& info, const texture_state* texture_state);


        [[nodiscard]] descriptor_type object_type() const override;
    private:
        [[nodiscard]] status attach_texture(const framebuffer_attachment_info& info, const GLenum attachment, const texture_state* texture_state);

        GLuint gl_id;
        bool is_framebuffer_complete = false;
        std::vector<GLuint> attached_textures;
    };
}
