#pragma once
#include "texture_state.hpp"
#include "stardraw/gl45/common.hpp"

namespace stardraw::gl45
{
    class framebuffer_state final : public object_state
    {
    public:
        explicit framebuffer_state(const framebuffer& descriptor, status& out_status);
        ~framebuffer_state() override;
        [[nodiscard]] status check_complete();
        [[nodiscard]] bool is_valid() const;
        [[nodiscard]] status attach_color_texture(const framebuffer_attachment_info& info, const texture_state* texture_state, u32 attachment_index);
        [[nodiscard]] status attach_depth_texture(const framebuffer_attachment_info& info, const texture_state* texture_state);
        [[nodiscard]] status attach_stencil_texture(const framebuffer_attachment_info& info, const texture_state* texture_state);
        [[nodiscard]] status attach_depth_stencil_texture(const framebuffer_attachment_info& info, const texture_state* texture_state);
        [[nodiscard]] status bind() const;

        [[nodiscard]] descriptor_type object_type() const override;
        [[nodiscard]] status blit_to(framebuffer_state* dest_state, const framebuffer_copy_info& info);
        [[nodiscard]] status blit_to_default(const framebuffer_copy_info& info);
        [[nodiscard]] status clear_depth(f32 value) const;
        [[nodiscard]] status clear_stencil(u32 value) const;
        [[nodiscard]] status clear_color(u32 attachment, const clear_values& values);

    private:
        [[nodiscard]] status attach_texture(const framebuffer_attachment_info& info, const GLenum attachment, const texture_state* texture_state);

        GLuint gl_id{};
        bool is_framebuffer_complete = false;
        GLenum depth_format;
        GLenum stencil_format;
        std::vector<GLuint> attached_textures;
        std::unordered_map<u32, texture_data_type> attached_color_formats;
        object_identifier framebuffer_id;
    };
}
