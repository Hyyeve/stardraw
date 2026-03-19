#include "texture_sampler_state.hpp"
#include "stardraw/gl45/api_conversion.hpp"
#include "tracy/Tracy.hpp"
#include "tracy/TracyOpenGL.hpp"

namespace stardraw::gl45 {
    bool is_sampling_config_valid_for_type(const texture_data_type type, const texture_sampling_conifg& sampling_config)
    {
        ZoneScoped;
        const bool is_integer_type = is_texture_data_type_integer(type);
        if (!is_integer_type) return true;

        if (sampling_config.upscale_filter != texture_filtering_mode::NEAREST) return false;
        if (sampling_config.downscale_filter != texture_filtering_mode::NEAREST) return false;
        if (sampling_config.mipmap_filter != texture_filtering_mode::NEAREST) return false;

        return true;
    }

    texture_sampler_state::texture_sampler_state(const texture_sampler_descriptor& desc, status& out_status)
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Create texture sampler state");

        if (desc.integer_texture_sampler)
        {
            if (desc.sampler_config.upscale_filter != texture_filtering_mode::NEAREST ||
                desc.sampler_config.downscale_filter != texture_filtering_mode::NEAREST ||
                desc.sampler_config.mipmap_filter != texture_filtering_mode::NEAREST)
            {

                out_status = {status_type::INVALID, "Integer type textures/samplers must only have nearest filtering"};
                return;
            }
        }

        glCreateSamplers(1, &gl_id);

        if (gl_id == 0)
        {
            out_status = {status_type::BACKEND_ERROR, "Failed to create texture sampler state"};
            return;
        }

        out_status = set_sampling_config(desc.sampler_config, desc.integer_texture_sampler);
    }

    texture_sampler_state::~texture_sampler_state()
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Delete texture sampler state");
        glDeleteSamplers(1, &gl_id);
    }

    inline void set_texture_border_color(const GLuint texture_id, const texture_border_color border_color, const bool is_integer)
    {
        ZoneScoped;
        if (is_integer)
        {
            switch (border_color)
            {
                case texture_border_color::OPAUQE_BLACK:
                {
                    constexpr std::array col = {0, 0, 0, 1};
                    glSamplerParameteriv(texture_id, GL_TEXTURE_BORDER_COLOR, col.data());
                    break;
                }
                case texture_border_color::OPAQUE_WHITE:
                {
                    constexpr std::array col = {1, 1, 1, 1};
                    glSamplerParameteriv(texture_id, GL_TEXTURE_BORDER_COLOR, col.data());
                    break;
                }
                case texture_border_color::TRANSPARENT:
                {
                    constexpr std::array col = {0, 0, 0, 0};
                    glSamplerParameteriv(texture_id, GL_TEXTURE_BORDER_COLOR, col.data());
                    break;
                }
            }
        }
        else
        {
            switch (border_color)
            {
                case texture_border_color::OPAUQE_BLACK:
                {
                    constexpr std::array col = {0.f, 0.f, 0.f, 1.f};
                    glSamplerParameterfv(texture_id, GL_TEXTURE_BORDER_COLOR, col.data());
                    break;
                }
                case texture_border_color::OPAQUE_WHITE:
                {
                    constexpr std::array col = {1.f, 1.f, 1.f, 1.f};
                    glSamplerParameterfv(texture_id, GL_TEXTURE_BORDER_COLOR, col.data());
                    break;
                }
                case texture_border_color::TRANSPARENT:
                {
                    constexpr std::array col = {0.f, 0.f, 0.f, 0.f};
                    glSamplerParameterfv(texture_id, GL_TEXTURE_BORDER_COLOR, col.data());
                    break;
                }
            }
        }
    }

    bool texture_sampler_state::is_valid() const
    {
        return gl_id != 0 && glIsSampler(gl_id);
    }

    descriptor_type texture_sampler_state::object_type() const
    {
        return descriptor_type::TEXTURE_SAMPLER;
    }

    status texture_sampler_state::set_sampling_config(const texture_sampling_conifg& config, const bool is_integer_type) const
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Set texture sampler config");
        glSamplerParameteri(gl_id, GL_TEXTURE_MAG_FILTER, to_gl_filter_mode_from_modes(config.upscale_filter, config.mipmap_filter, false));
        glSamplerParameteri(gl_id, GL_TEXTURE_MIN_FILTER, to_gl_filter_mode_from_modes(config.downscale_filter, config.mipmap_filter, true));

        glSamplerParameteri(gl_id, GL_TEXTURE_WRAP_S, to_gl_wrapping_mode(config.wrapping_mode_x));
        glSamplerParameteri(gl_id, GL_TEXTURE_WRAP_T, to_gl_wrapping_mode(config.wrapping_mode_y));
        glSamplerParameteri(gl_id, GL_TEXTURE_WRAP_R, to_gl_wrapping_mode(config.wrapping_mode_z));

        set_texture_border_color(gl_id, config.border_color, is_integer_type);

        if (GLAD_GL_VERSION_4_6 || GLAD_GL_ARB_texture_filter_anisotropic || GLAD_GL_EXT_texture_filter_anisotropic)
        {
            //Anisotropy is not *strictly* core, but it's supported practically universally
            glSamplerParameterf(gl_id, GL_TEXTURE_MAX_ANISOTROPY, static_cast<float>(config.anisotropy_level));
        }

        glSamplerParameteri(gl_id, GL_TEXTURE_BASE_LEVEL, config.mipmap_min_level);
        glSamplerParameteri(gl_id, GL_TEXTURE_MAX_LEVEL, config.mipmap_max_level);
        glSamplerParameterf(gl_id, GL_TEXTURE_LOD_BIAS, config.mipmap_bias);

        glSamplerParameteri(gl_id, GL_TEXTURE_SWIZZLE_R, to_gl_swizzle_mode(config.swizzling.swizzle_red));
        glSamplerParameteri(gl_id, GL_TEXTURE_SWIZZLE_G, to_gl_swizzle_mode(config.swizzling.swizzle_green));
        glSamplerParameteri(gl_id, GL_TEXTURE_SWIZZLE_B, to_gl_swizzle_mode(config.swizzling.swizzle_blue));
        glSamplerParameteri(gl_id, GL_TEXTURE_SWIZZLE_A, to_gl_swizzle_mode(config.swizzling.swizzle_alpha));

        return status_type::SUCCESS;
    }
}
