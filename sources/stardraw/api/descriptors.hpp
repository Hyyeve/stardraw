#pragma once
#include <string>
#include <utility>

#include "shaders.hpp"
#include "types.hpp"
#include "starlib/types/polymorphic.hpp"
#include "starlib/types/starlib_stdint.hpp"

namespace stardraw
{
    using namespace starlib_stdint;
    enum class descriptor_type : u8
    {
        BUFFER, SHADER, TEXTURE, TEXTURE_SAMPLER, VERTEX_SPECIFICATION, DRAW_SPECIFICATION,
    };

    struct descriptor
    {
        explicit constexpr descriptor(const std::string_view& name) : ident(name) {}
        virtual ~descriptor() = default;

        [[nodiscard]] virtual descriptor_type type() const = 0;
        [[nodiscard]] const object_identifier& identifier() const;

    private:
        object_identifier ident;
    };

    inline const object_identifier& descriptor::identifier() const
    {
        return ident;
    }

    typedef std::vector<starlib::polymorphic<descriptor>> descriptor_list;

    ///NOTE: Buffer memory storage cannot be guarenteed on OpenGL, but SYSRAM guarentees it will be possible to write into the buffer directly.
    enum class buffer_memory_storage : u8
    {
        SYSRAM, VRAM,
    };

    struct buffer_descriptor final : descriptor
    {
        explicit buffer_descriptor(const std::string_view& name, const u64 size, const buffer_memory_storage memory = buffer_memory_storage::VRAM) : descriptor(name), size(size), memory(memory) {}

        [[nodiscard]] descriptor_type type() const override
        {
            return descriptor_type::BUFFER;
        }

        u64 size;
        buffer_memory_storage memory;
    };

    enum class vertex_data_type : u8
    {
        //Simple non-converting types (same representation in shader and buffer)
        UINT_U8, UINT2_U8, UINT3_U8, UINT4_U8,
        UINT_U16, UINT2_U16, UINT3_U16, UINT4_U16,
        UINT_U32, UINT2_U32, UINT3_U32, UINT4_U32,

        INT_I8, INT2_I8, INT3_I8, INT4_I8,
        INT_I16, INT2_I16, INT3_I16, INT4_I16,
        INT_I32, INT2_I32, INT3_I32, INT4_I32,

        FLOAT_F16, FLOAT2_F16, FLOAT3_F16, FLOAT4_F16,
        FLOAT_F32, FLOAT2_F32, FLOAT3_F32, FLOAT4_F32,

        //Normalized f32 types (f32s in shader, interger types in buffer)
        FLOAT_U8_NORM, FLOAT2_U8_NORM, FLOAT3_U8_NORM, FLOAT4_U8_NORM,
        FLOAT_I8_NORM, FLOAT2_I8_NORM, FLOAT3_I8_NORM, FLOAT4_I8_NORM,

        FLOAT_U16_NORM, FLOAT2_U16_NORM, FLOAT3_U16_NORM, FLOAT4_U16_NORM,
        FLOAT_I16_NORM, FLOAT2_I16_NORM, FLOAT3_I16_NORM, FLOAT4_I16_NORM,
    };

    struct vertex_data_binding
    {
        constexpr vertex_data_binding(const std::string_view& buffer, const vertex_data_type& type, const u32 instance_divisor = 0) : type(type), instance_divisor(instance_divisor), buffer(buffer) {}

        vertex_data_type type;
        u32 instance_divisor;
        std::string buffer;
    };

    struct vertex_data_layout
    {
        constexpr vertex_data_layout(const std::initializer_list<vertex_data_binding> bindings) : bindings(bindings) {}
        explicit constexpr vertex_data_layout(const std::vector<vertex_data_binding>& bindings) : bindings(bindings) {}

        std::vector<vertex_data_binding> bindings;
    };

    struct vertex_specification_descriptor final : descriptor
    {
        constexpr vertex_specification_descriptor(const std::string_view& name, vertex_data_layout layout, const std::string_view& index_buffer = "") : descriptor(name), layout(std::move(layout)), index_buffer(index_buffer) {}

        [[nodiscard]] descriptor_type type() const override
        {
            return descriptor_type::VERTEX_SPECIFICATION;
        }

        vertex_data_layout layout;
        std::string index_buffer;
    };

    struct draw_specification_descriptor final : descriptor
    {
        draw_specification_descriptor(const std::string_view& name, const std::string_view& vertex_specification, const std::string_view& shader) : descriptor(name), vertex_specification(vertex_specification), shader(shader) {}

        [[nodiscard]] descriptor_type type() const override
        {
            return descriptor_type::DRAW_SPECIFICATION;
        }

        std::string vertex_specification;
        std::string shader;
    };

    struct shader_descriptor final : descriptor
    {
        shader_descriptor(const std::string_view& name, const std::vector<shader_stage>& stages) : descriptor(name), stages(stages), cache_ptr(nullptr), cache_size(0) {}
        shader_descriptor(const std::string_view& name, const void* cache_ptr, const u64 cache_size) : descriptor(name), stages({}), cache_ptr(cache_ptr), cache_size(cache_size) {}

        [[nodiscard]] descriptor_type type() const override
        {
            return descriptor_type::SHADER;
        }

        std::vector<shader_stage> stages;
        const void* cache_ptr;
        const u64 cache_size;
    };

    enum class texture_data_type : u8
    {
        //Depth / stencil formats
        DEPTH_32, DEPTH_24, DEPTH_16,
        DEPTH_32_STENCIL_8, DEPTH_24_STENCIL_8,
        STENCIL_8,

        //Standard 8-bit normalized formats (8-bit uint storage, 0-1 f32 reads)
        R_8, RG_8, RGB_8, RGBA_8,
        SRGB_8, SRGBA_8,

        //Specific typed formats
        R_U8, RG_U8, RGB_U8, RGBA_U8,
        R_U16, RG_U16, RGB_U16, RGBA_U16,
        R_U32, RG_U32, RGB_U32, RGBA_U32,

        R_I8, RG_I8, RGB_I8, RGBA_I8,
        R_I16, RG_I16, RGB_I16, RGBA_I16,
        R_I32, RG_I32, RGB_I32, RGBA_I32,

        R_F16, RG_F16, RGB_F16, RGBA_F16,
        R_F32, RG_F32, RGB_F32, RGBA_F32,

        //TODO: Support compressed textures?
    };

    enum class texture_shape : u8
    {
        _1D,
        _2D,
        _3D,
        CUBE_MAP,
    };

    enum class texture_msaa_level : u8
    {
        NONE = 0, X4 = 4, X8 = 8, X16 = 16, X32 = 32,
    };

    struct texture_format
    {
        texture_data_type data_type;
        texture_shape shape;
        texture_msaa_level msaa;

        //Number of mipmaps to allocate *including* the base (level 0) level. Must be >0
        u8 mipmap_levels = 1;
        u32 width;
        u32 height;
        u32 depth = 0;

        //Number of texture layers. Must be a multiple of 6 for cubemaps, and at least 1 for all other texture types. >1 creates an array texture.
        u32 texture_layers = 1;

        //Exclusive to view textures:
        u8 view_texture_base_mipmap = 0;
        u8 view_texture_base_array_index = 0;

        inline static texture_format create_1d(const u32 width, const texture_data_type = texture_data_type::RGBA_8, const texture_data_type data_type = texture_data_type::RGBA_8, const u8 mipmap_levels = 1)
        {
            return {data_type, texture_shape::_1D, texture_msaa_level::NONE, mipmap_levels, width, 0, 0, 1, 0};
        }

        inline static texture_format create_1d_array(const u32 width, const u32 array_size, const texture_data_type = texture_data_type::RGBA_8, const texture_data_type data_type = texture_data_type::RGBA_8, const u8 mipmap_levels = 1)
        {
            return {data_type, texture_shape::_1D, texture_msaa_level::NONE, mipmap_levels, width, 0, 0, array_size, 0};
        }

        inline static texture_format create_2d(const u32 width, const u32 height, const texture_data_type data_type = texture_data_type::RGBA_8, const u8 mipmap_levels = 1, const texture_msaa_level msaa = texture_msaa_level::NONE)
        {
            return {data_type, texture_shape::_2D, msaa, mipmap_levels, width, height, 0, 1, 0};
        }

        inline static texture_format create_2d_array(const u32 width, const u32 height, const u32 array_size, const texture_data_type data_type = texture_data_type::RGBA_8, const u8 mipmap_levels = 1, const texture_msaa_level msaa = texture_msaa_level::NONE)
        {
            return {data_type, texture_shape::_2D, msaa, mipmap_levels, width, height, 0, array_size, 0};
        }

        inline static texture_format create_3d(const u32 width, const u32 height, const u32 depth, const texture_data_type data_type = texture_data_type::RGBA_8, const u8 mipmap_levels = 1, const texture_msaa_level msaa = texture_msaa_level::NONE)
        {
            return {data_type, texture_shape::_3D, msaa, mipmap_levels, width, height, depth, 1, 0};
        }

        inline static texture_format create_cube(const u32 width, const u32 height, const texture_data_type data_type = texture_data_type::RGBA_8, const u8 mipmap_levels = 1)
        {
            return {data_type, texture_shape::CUBE_MAP, texture_msaa_level::NONE, mipmap_levels, width, height, 0, 6, 0};
        }

        inline static texture_format create_cube_array(const u32 width, const u32 height, const u32 num_cubemaps, const texture_data_type data_type = texture_data_type::RGBA_8, const u8 mipmap_levels = 1)
        {
            return {data_type, texture_shape::CUBE_MAP, texture_msaa_level::NONE, mipmap_levels, width, height, 0, num_cubemaps * 6, 0};
        }
    };

    enum class texture_anisotropy_level : u8
    {
        NONE = 1, X2 = 2, X4 = 4, X8 = 8, X16 = 16,
    };

    enum class texture_filtering_mode : u8
    {
        NEAREST = 0, LINEAR = 1,
    };

    enum class texture_wrapping_mode : u8
    {
        CLAMP, REPEAT, MIRROR, BORDER
    };

    enum class texture_border_color : u8
    {
        INTEGER_BLACK,
        INTEGER_WHITE,
        INTEGER_TRANSPARENT,
        FLOAT_BLACK,
        FLOAT_WHITE,
        FLOAT_TRANSPARENT,
    };

    enum class texture_swizzle
    {
        RED, GREEN, BLUE, ALPHA
    };

    struct texture_swizzle_mode
    {
        texture_swizzle swizzle_red = texture_swizzle::RED;
        texture_swizzle swizzle_green = texture_swizzle::GREEN;
        texture_swizzle swizzle_blue = texture_swizzle::BLUE;
        texture_swizzle swizzle_alpha = texture_swizzle::ALPHA;
    };

    struct texture_sampling_conifg
    {
        texture_filtering_mode downscale_filter = texture_filtering_mode::LINEAR;
        texture_filtering_mode upscale_filter = texture_filtering_mode::LINEAR;

        texture_wrapping_mode wrapping_mode_x = texture_wrapping_mode::CLAMP;
        texture_wrapping_mode wrapping_mode_y = texture_wrapping_mode::CLAMP;
        texture_wrapping_mode wrapping_mode_z = texture_wrapping_mode::CLAMP;
        texture_border_color border_color = texture_border_color::FLOAT_TRANSPARENT;

        //Anisotropy is not *strictly* supported in all implementations without extensions, but availablity is near universal.
        texture_anisotropy_level anisotropy_level = texture_anisotropy_level::X16;

        texture_swizzle_mode swizzling = {};

        texture_filtering_mode mipmap_filter = texture_filtering_mode::NEAREST;
        u32 mipmap_min_level = 0;
        u32 mipmap_max_level = 99;
        f32 mipmap_bias = 0;
    };

    namespace texture_sampling_configs
    {
        constexpr texture_sampling_conifg linear = {};
        constexpr texture_sampling_conifg nearest = {.downscale_filter = texture_filtering_mode::NEAREST, .upscale_filter = texture_filtering_mode::NEAREST};
        constexpr texture_sampling_conifg none = {.downscale_filter = texture_filtering_mode::NEAREST, .upscale_filter = texture_filtering_mode::NEAREST, .anisotropy_level = texture_anisotropy_level::NONE, .mipmap_max_level = 0};
    }

    struct texture_descriptor final : descriptor
    {
        explicit texture_descriptor(const std::string_view& name, const texture_format& format, const texture_sampling_conifg& default_sampling_config, const std::string_view& as_view_of_texture = "") : descriptor(name), format(format), default_sampling_config(default_sampling_config), as_view_of(as_view_of_texture) {}

        [[nodiscard]] descriptor_type type() const override
        {
            return descriptor_type::TEXTURE;
        }

        texture_format format;
        texture_sampling_conifg default_sampling_config;
        std::string as_view_of;
    };

    struct texture_sampler_descriptor final : descriptor
    {
        texture_sampler_descriptor(const std::string_view& name, const texture_sampling_conifg& smapler_config) : descriptor(name), smapler_config(smapler_config) {}

        [[nodiscard]] descriptor_type type() const override
        {
            return descriptor_type::TEXTURE_SAMPLER;
        }

        texture_sampling_conifg smapler_config;
    };
}
