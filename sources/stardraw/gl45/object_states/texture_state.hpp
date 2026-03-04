#pragma once
#include "../gl_headers.hpp"
#include "../types.hpp"
#include "stardraw/api/commands.hpp"
namespace stardraw::gl45
{
    using namespace starlib_stdint;
    class texture_state final : public object_state
    {
    public:
        explicit texture_state(const texture_descriptor& desc, status& out_status);
        explicit texture_state(const texture_state* original, const texture_descriptor& desc, status& out_status);
        ~texture_state() override;

        [[nodiscard]] bool is_valid() const;
        [[nodiscard]] status is_view_compatible(const texture_descriptor& view_descriptor) const;
        [[nodiscard]] status set_sampling_config(const texture_sampling_conifg& config) const;

        [[nodiscard]] descriptor_type object_type() const override
        {
            return descriptor_type::TEXTURE;
        }

    private:
        GLuint texture_id = 0;
        u32 num_texture_mipmap_levels;
        u32 num_texture_array_layers;
        GLenum gl_texture_format;
        GLenum gl_texture_target;
    };
}