#pragma once

#include "../gl_headers.hpp"
#include "../common.hpp"
#include "stardraw/api/commands.hpp"
#include "starlib/math/glm.hpp"

namespace stardraw::gl45
{
    using namespace starlib_stdint;

    class texture_state final : public object_state
    {
    public:
        explicit texture_state(const texture_descriptor& desc, status& out_status);
        explicit texture_state(const texture_state* original, const texture_descriptor& desc, status& out_status);
        ~texture_state() override;

        [[nodiscard]] status unpack_pixels(const u32 mipmap_level, const u32 x, const u32 y, const u32 z, const u32 width, const u32 height, const u32 depth, const GLenum format, const GLenum gl_data_type) const;
        [[nodiscard]] status copy_pixels(const texture_state* read_texture, const texture_copy_info& copy_info) const;

        [[nodiscard]] status prepare_upload(const texture_memory_transfer_info& info, memory_transfer_handle** out_handle) const;
        [[nodiscard]] status flush_upload(const texture_memory_transfer_info& info, memory_transfer_handle* handle) const;

        [[nodiscard]] bool is_valid() const;
        [[nodiscard]] status bind_to_texture_slot(u32 slot) const;
        [[nodiscard]] status bind_to_image_slot(u32 slot, u32 mipmap_level, u32 array_layer, bool entire_array, GLenum access) const;
        [[nodiscard]] static bool is_view_format_compatible(GLenum source_format, GLenum view_format);
        [[nodiscard]] static bool is_view_target_compatible(GLenum source_target, GLenum view_target);
        [[nodiscard]] status is_view_compatible(const texture_descriptor& view_descriptor) const;
        [[nodiscard]] status set_sampling_config(const texture_sampling_conifg& config, bool is_integer_texture) const;

        [[nodiscard]] texture_shape get_shape() const;
        [[nodiscard]] descriptor_type object_type() const override
        {
            return descriptor_type::TEXTURE;
        }

        object_identifier root_data_store_texture;

        u32 num_texture_array_layers = 0;
        u32 num_texture_msaa_samples = 0;
        GLuint gl_texture_id = 0;
        GLenum gl_texture_target = 0;
        GLenum gl_texture_format = 0;

        glm::vec<3, u32> size = {0, 0, 0};
        u32 num_texture_mipmap_levels = 0;
        u32 bytes_per_pixel = 0;

        texture_shape shape = texture_shape::_2D;
        texture_data_type data_type = texture_data_type::R_U8_NORM;
    private:
        [[nodiscard]] u64 compute_bytes_in_transfer(const texture_memory_transfer_info& info) const;
        status initalize_and_validate_texture_descriptor(const texture_descriptor& desc);
    };
}
