#pragma once
#include <optional>
#include <utility>

#include "common.hpp"
#include "shaders.hpp"
#include "starlib/types/polymorphic.hpp"
#include "starlib/types/starlib_stdint.hpp"

namespace stardraw
{

    ///Identifier for descriptors. Used for internal descriptor polymorphism purposes
    enum class descriptor_type : starlib_stdint::u8
    {
        BUFFER, SHADER, TEXTURE, SAMPLER,  FRAMEBUFFER,
        VERTEX_CONFIGURATION, DRAW_CONFIGURATION,
    };

    ///Describes some abstract 'graphics object' that represents either an on-gpu object,
    ///or in some cases cpu-side metadata that's used for convienience and to simplify stardraw
    struct descriptor
    {
        explicit constexpr descriptor(const std::string_view& name) : ident(name) {}
        virtual ~descriptor() = default;

        [[nodiscard]] virtual descriptor_type type() const = 0;
        [[nodiscard]] const inline object_identifier& identifier() const
        {
            return ident;
        }

    private:
        object_identifier ident;
    };

    ///Descriptor list type
    typedef std::vector<starlib::polymorphic<descriptor>> descriptor_list;

    ///NOTE: Buffer memory storage cannot be guarenteed on OpenGL, but SYSRAM guarentees it will be possible to write into the buffer directly.
    ///TODO: This needs to be refactored into more specific usage flags to support other backends.
    enum class buffer_memory_storage : starlib_stdint::u8
    {
        SYSRAM, VRAM,
    };

    ///Describes a generic block of GPU-accessible memory that can be used for almost any purpose.
    struct buffer final : descriptor
    {
        explicit buffer(const std::string_view& name, const starlib_stdint::u64 size, const buffer_memory_storage memory = buffer_memory_storage::VRAM) : descriptor(name), size(size), memory(memory) {}

        [[nodiscard]] descriptor_type type() const override
        {
            return descriptor_type::BUFFER;
        }

        starlib_stdint::u64 size;
        buffer_memory_storage memory;
    };

    ///Specifies how vertex data is interpreted.
    ///Vertex inputs can have a differing 'backing value' to how they're
    ///interpreted in the shader - for instance, FLOAT2_U16_NORM:
    /// - Shader sees a FLOAT2 / VEC2
    /// - Each U16 in the buffer is converted to one float
    /// - The conversion is 'normalized' - U16_MIN maps to 0.0f, U16_MAX maps to 1.0f.
    enum class vertex_data_type : starlib_stdint::u8
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

    ///Describes bindings between a vertex data input and the buffer it reads from.
    ///Instance divisor is used for instanced rendering only.
    ///A >0 value means that this input will only advance per-instance, specifically per X instances where instance_divisor is X.
    struct vertex_data_binding
    {
        constexpr vertex_data_binding(const std::string_view& buffer, const vertex_data_type& type, const starlib_stdint::u32 instance_divisor = 0) : type(type), instance_divisor(instance_divisor), buffer(buffer) {}

        vertex_data_type type;
        starlib_stdint::u32 instance_divisor;
        object_identifier buffer;
    };

    ///Describes a set of bindings between vertex data inputs and the buffers they are read from.
    struct vertex_data_layout
    {
        constexpr vertex_data_layout(const std::initializer_list<vertex_data_binding> bindings) : bindings(bindings) {}
        explicit constexpr vertex_data_layout(const std::vector<vertex_data_binding>& bindings) : bindings(bindings) {}

        std::vector<vertex_data_binding> bindings;
    };

    ///Describes a mapping from data in arbitrary buffers into vertex data inputs for shaders.
    struct vertex_configuration final : descriptor
    {
        constexpr vertex_configuration(const std::string_view& name, vertex_data_layout layout, const std::string_view& index_buffer = "") : descriptor(name), layout(std::move(layout)), index_buffer(index_buffer), has_index_buffer(!index_buffer.empty()) {}

        [[nodiscard]] descriptor_type type() const override
        {
            return descriptor_type::VERTEX_CONFIGURATION;
        }

        vertex_data_layout layout;
        object_identifier index_buffer;
        bool has_index_buffer;
    };

    ///Describes a vertex specification and shader combination required for calling draw commands
    struct draw_configuration final : descriptor
    {
        draw_configuration(const std::string_view& name, const std::string_view& vertex_specification, const std::string_view& shader, const std::optional<object_identifier>& framebuffer = std::nullopt) : descriptor(name), vertex_specification(vertex_specification), shader(shader), framebuffer(framebuffer) {}

        [[nodiscard]] descriptor_type type() const override
        {
            return descriptor_type::DRAW_CONFIGURATION;
        }

        object_identifier vertex_specification;
        object_identifier shader;
        std::optional<object_identifier> framebuffer;
    };

    ///Describes a shader made up of some number of shader states
    struct shader final : descriptor
    {
        shader(const std::string_view& name, const std::vector<shader_stage>& stages) : descriptor(name), stages(stages), cache_ptr(nullptr), cache_size(0) {}
        shader(const std::string_view& name, const void* cache_ptr, const starlib_stdint::u64 cache_size) : descriptor(name), stages({}), cache_ptr(cache_ptr), cache_size(cache_size) {}

        [[nodiscard]] descriptor_type type() const override
        {
            return descriptor_type::SHADER;
        }

        std::vector<shader_stage> stages;
        const void* cache_ptr;
        const starlib_stdint::u64 cache_size;
    };

    ///Specifies how texture data is interpreted.
    ///Similar to vertex data, textures can have differing 'backing value' to how they're
    ///interpreted in the shader - for instance, SRGB_U8_NORM:
    /// - Shader sees a FLOAT3 / VEC3 of RGB
    /// - Each U8 in the texture data is converted to one float
    /// - The conversion is 'normalized' - U8_MIN maps to 0.0f, U8_MAX maps to 1.0f.
    /// - The data is stored with the SRGB colorspace.
    enum class texture_data_type : starlib_stdint::u8
    {
        //Depth / stencil formats
        DEPTH_F32, DEPTH_U32_NORM, DEPTH_U24_NORM, DEPTH_U16_NORM,
        DEPTH_U32_NORM_STENCIL_U8, DEPTH_U24_NORM_STENCIL_U8,
        STENCIL_U8,

        //Standard 8-bit normalized formats (8-bit uint storage, 0-1 f32 reads)
        R_U8_NORM, RG_U8_NORM, RGB_U8_NORM, RGBA_U8_NORM,
        SRGB_U8_NORM, SRGBA_U8_NORM,

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

    ///Texture 'shape' determines what coordinates are used to read from the texture.
    enum class texture_shape : starlib_stdint::u8
    {
        _1D,
        _2D,
        _3D,
        CUBE_MAP,
    };

    ///Texture MSAA level determines whether the texture will store multiple samples per pixel
    enum class texture_msaa_level : starlib_stdint::u8
    {
        NONE = 0, X4 = 4, X8 = 8, X16 = 16, X32 = 32,
    };

    ///Describes all the parameters needed to specify how texture data is stored.
    ///NOTE: Not all combinations of all parameters are valid!
    struct texture_format
    {
        texture_data_type data_type;
        texture_shape shape = texture_shape::_1D;
        texture_msaa_level msaa = texture_msaa_level::NONE;

        starlib_stdint::u32 width = 1;
        starlib_stdint::u32 height = 1;
        starlib_stdint::u32 depth = 1;

        ///Number of texture layers. Must be a multiple of 6 for cubemaps, and at least 1 for all other texture types. >1 (or >6 for cubemaps) creates a layered (array) texture.
        starlib_stdint::u32 layers = 1;

        ///Number of mipmaps to allocate *including* the base (level 0) level. Must be >0
        starlib_stdint::u8 mipmap_levels = 1;

        ///Exclusive to view textures:
        starlib_stdint::u8 view_texture_base_mipmap = 0;
        starlib_stdint::u8 view_texture_base_layer = 0;

        inline static texture_format create_1d(const starlib_stdint::u32 width, const texture_data_type data_type = texture_data_type::RGBA_U8_NORM, const starlib_stdint::u8 mipmap_levels = 1)
        {
            return {.data_type = data_type, .width = width, .mipmap_levels = mipmap_levels};
        }

        inline static texture_format create_1d_layered(const starlib_stdint::u32 width, const starlib_stdint::u32 layers, const texture_data_type data_type = texture_data_type::RGBA_U8_NORM, const starlib_stdint::u8 mipmap_levels = 1)
        {
            return {.data_type = data_type, .width = width, .layers = layers, .mipmap_levels = mipmap_levels};
        }

        inline static texture_format create_2d(const starlib_stdint::u32 width, const starlib_stdint::u32 height, const texture_data_type data_type = texture_data_type::RGBA_U8_NORM, const starlib_stdint::u8 mipmap_levels = 1, const texture_msaa_level msaa = texture_msaa_level::NONE)
        {
            return {.data_type = data_type, .shape = texture_shape::_2D, .msaa = msaa, .width = width, .height = height, .mipmap_levels = mipmap_levels};
        }

        inline static texture_format create_2d_layered(const starlib_stdint::u32 width, const starlib_stdint::u32 height, const starlib_stdint::u32 array_size, const texture_data_type data_type = texture_data_type::RGBA_U8_NORM, const starlib_stdint::u8 mipmap_levels = 1, const texture_msaa_level msaa = texture_msaa_level::NONE)
        {
            return {.data_type = data_type, .shape = texture_shape::_2D, .msaa = msaa, .width = width, .height = height, .layers = array_size, .mipmap_levels = mipmap_levels};
        }

        inline static texture_format create_3d(const starlib_stdint::u32 width, const starlib_stdint::u32 height, const starlib_stdint::u32 depth, const texture_data_type data_type = texture_data_type::RGBA_U8_NORM, const starlib_stdint::u8 mipmap_levels = 1, const texture_msaa_level msaa = texture_msaa_level::NONE)
        {
            return {.data_type = data_type, .shape = texture_shape::_3D, .msaa = msaa, .width = width, .height = height, .depth = depth, .mipmap_levels = mipmap_levels};
        }

        inline static texture_format create_cube(const starlib_stdint::u32 width, const starlib_stdint::u32 height, const texture_data_type data_type = texture_data_type::RGBA_U8_NORM, const starlib_stdint::u8 mipmap_levels = 1)
        {
            return {.data_type = data_type, .shape = texture_shape::CUBE_MAP, .width = width, .height = height, .layers = 6, .mipmap_levels = mipmap_levels};
        }

        inline static texture_format create_cube_layered(const starlib_stdint::u32 width, const starlib_stdint::u32 height, const starlib_stdint::u32 num_cubemaps, const texture_data_type data_type = texture_data_type::RGBA_U8_NORM, const starlib_stdint::u8 mipmap_levels = 1)
        {
            return {.data_type = data_type, .shape = texture_shape::CUBE_MAP, .width = width, .height = height, .layers = num_cubemaps * 6, .mipmap_levels = mipmap_levels};
        }
    };

    ///Level of anisotropic filtering to use on a texture.
    ///Requires that filtering must be able to account for at least an aspect ratio of 1/X where X is the anistropy level.
    enum class texture_anisotropy_level : starlib_stdint::u8
    {
        NONE = 1, X2 = 2, X4 = 4, X8 = 8, X16 = 16,
    };

    ///Type of interpolation to use when sampling inbetween pixels / layers / mipmaps.
    enum class texture_filtering_mode : starlib_stdint::u8
    {
        NEAREST = 0, LINEAR = 1,
    };

    ///Type of wrapping to perform when sampling outside the texture boundary
    enum class texture_wrapping_mode : starlib_stdint::u8
    {
        CLAMP, REPEAT, MIRROR, BORDER
    };

    ///Color that will be returned when sampling outside the texture boundary if the wrapping mode is BORDER.
    enum class texture_border_color : starlib_stdint::u8
    {
        OPAUQE_BLACK, //0, 0, 0, 1
        OPAQUE_WHITE, //1, 1, 1, 1
        TRANSPARENT_BLACK, //0, 0, 0, 0
        TRANSPARENT_WHITE, //1, 1, 1, 0
    };

    ///Channel identifiers for texture swizzling
    enum class texture_swizzle
    {
        RED, GREEN, BLUE, ALPHA
    };

    ///Configuratoin for how to swizzle texture channels.
    ///Mappings are from sampled channel -> original channel, as in:
    ///swizzle_red = texture_swizzle::GREEN, causes reads to the red channel to return data from the (original) green channel.
    struct texture_swizzle_config
    {
        texture_swizzle swizzle_red = texture_swizzle::RED;
        texture_swizzle swizzle_green = texture_swizzle::GREEN;
        texture_swizzle swizzle_blue = texture_swizzle::BLUE;
        texture_swizzle swizzle_alpha = texture_swizzle::ALPHA;
    };

    ///Collection of options required for configuring texture sampling
    struct texture_sampling_conifg
    {
        texture_filtering_mode downscale_filter = texture_filtering_mode::LINEAR;
        texture_filtering_mode upscale_filter = texture_filtering_mode::LINEAR;

        texture_wrapping_mode wrapping_mode_x = texture_wrapping_mode::REPEAT;
        texture_wrapping_mode wrapping_mode_y = texture_wrapping_mode::REPEAT;
        texture_wrapping_mode wrapping_mode_z = texture_wrapping_mode::REPEAT;
        texture_border_color border_color = texture_border_color::TRANSPARENT_BLACK;

        ///NOTE: Anisotropy is not *strictly* supported in all implementations without extensions, but availablity is near universal.
        ///If anisotropy is not supported, this setting will be ignored.
        texture_anisotropy_level anisotropy_level = texture_anisotropy_level::NONE;

        ///NOTE: Swizzling mode is ingored by samplers and can only be used on textures directly.
        texture_swizzle_config swizzling = {};

        texture_filtering_mode mipmap_filter = texture_filtering_mode::NEAREST;
        starlib_stdint::u32 mipmap_min_level = 0;
        starlib_stdint::u32 mipmap_max_level = 99;
        starlib_stdint::f32 mipmap_bias = 0;
    };

    ///Default texture sampling configs
    namespace texture_sampling_configs
    {
        constexpr texture_sampling_conifg linear = {.anisotropy_level = texture_anisotropy_level::X16};
        constexpr texture_sampling_conifg nearest = {.downscale_filter = texture_filtering_mode::NEAREST, .upscale_filter = texture_filtering_mode::NEAREST};
        constexpr texture_sampling_conifg none = {.downscale_filter = texture_filtering_mode::NEAREST, .upscale_filter = texture_filtering_mode::NEAREST, .mipmap_max_level = 0};
    }

    ///Describes a texture with some format and default sampling config. Optionally, it can be a view into an existing texture.
    struct texture final : descriptor
    {
        explicit texture(const std::string_view& name, const texture_format& format, const texture_sampling_conifg& default_sampling_config, const std::string_view& as_view_of_texture = "") : descriptor(name), format(format), default_sampling_config(default_sampling_config), as_view_of(as_view_of_texture.empty() ? std::nullopt : std::optional(as_view_of_texture)) {}

        [[nodiscard]] descriptor_type type() const override
        {
            return descriptor_type::TEXTURE;
        }

        texture_format format;
        texture_sampling_conifg default_sampling_config;
        std::optional<object_identifier> as_view_of;
    };

    ///Describes a texture sampling config that can be applied to a specific sampler via shader parameters.
    struct sampler final : descriptor
    {
        sampler(const std::string_view& name, const texture_sampling_conifg& smapler_config, const bool integer_texture_sampler) : descriptor(name), sampler_config(smapler_config), integer_texture_sampler(integer_texture_sampler) {}

        [[nodiscard]] descriptor_type type() const override
        {
            return descriptor_type::SAMPLER;
        }

        texture_sampling_conifg sampler_config;
        bool integer_texture_sampler;
    };

    ///Information requierd for framebuffer attachments
    struct framebuffer_attachment_info
    {
        object_identifier texture;
        starlib_stdint::u32 mipmap_level;
        starlib_stdint::u32 layer;
        bool layered;
    };

    ///Describes a framebuffer with some number of color attachments, and optionally, depth/stencil attachments.
    ///It is allowed to provide the same texture for both the depth and stencil attachments, if the texture is a combined depth-stencil format.
    struct framebuffer final : descriptor
    {
        framebuffer(const std::string_view& name, const std::initializer_list<framebuffer_attachment_info> color_attachments, const std::optional<framebuffer_attachment_info>& depth_attachment = std::nullopt, const std::optional<framebuffer_attachment_info>& stencil_attachment = std::nullopt) : descriptor(name), color_attachments(color_attachments), depth_attachment(depth_attachment), stencil_attachment(stencil_attachment) {}
        framebuffer(const std::string_view& name, const std::vector<framebuffer_attachment_info>& color_attachments, const std::optional<framebuffer_attachment_info>& depth_attachment = std::nullopt, const std::optional<framebuffer_attachment_info>& stencil_attachment = std::nullopt) : descriptor(name), color_attachments(color_attachments), depth_attachment(depth_attachment), stencil_attachment(stencil_attachment) {}

        [[nodiscard]] descriptor_type type() const override
        {
            return descriptor_type::FRAMEBUFFER;
        }

        std::vector<framebuffer_attachment_info> color_attachments;
        std::optional<framebuffer_attachment_info> depth_attachment;
        std::optional<framebuffer_attachment_info> stencil_attachment;
    };
}
