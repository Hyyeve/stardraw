#pragma once
#include "gl_headers.hpp"
#include "stardraw/api/commands.hpp"
#include "stardraw/api/descriptors.hpp"
#include "stardraw/api/memory_transfer.hpp"
#include "stardraw/api/shaders.hpp"


namespace stardraw::gl45
{
    inline GLenum to_gl_shader_type(const shader_stage_type stage)
    {
        switch (stage)
        {
            case shader_stage_type::VERTEX: return GL_VERTEX_SHADER;
            case shader_stage_type::TESSELATION_CONTROL: return GL_TESS_CONTROL_SHADER;
            case shader_stage_type::TESSELATION_EVAL: return GL_TESS_EVALUATION_SHADER;
            case shader_stage_type::GEOMETRY: return GL_GEOMETRY_SHADER;
            case shader_stage_type::FRAGMENT: return GL_FRAGMENT_SHADER;
            case shader_stage_type::COMPUTE: return GL_COMPUTE_SHADER;
        }

        return 0;
    }

    inline GLenum to_gl_texture_target(const texture_shape type, const bool multisample, const bool is_array)
    {
        switch (type)
        {
            case texture_shape::_1D: return is_array ? GL_TEXTURE_1D_ARRAY : GL_TEXTURE_1D;
            case texture_shape::_2D: return multisample ? (is_array ? GL_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_MULTISAMPLE) : (is_array ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D);
            case texture_shape::_3D: return GL_TEXTURE_3D;
            case texture_shape::CUBE_MAP: return is_array ? GL_TEXTURE_CUBE_MAP_ARRAY : GL_TEXTURE_CUBE_MAP;
        }
        return 0;
    }

    inline GLenum to_gl_texture_format(const texture_data_type type)
    {
        switch (type)
        {
            case texture_data_type::DEPTH_U32_NORM: return GL_DEPTH_COMPONENT32;
            case texture_data_type::DEPTH_F32: return GL_DEPTH_COMPONENT32F;
            case texture_data_type::DEPTH_U24_NORM: return GL_DEPTH_COMPONENT24;
            case texture_data_type::DEPTH_U16_NORM: return GL_DEPTH_COMPONENT16;
            case texture_data_type::DEPTH_U32_NORM_STENCIL_U8: return GL_DEPTH32F_STENCIL8;
            case texture_data_type::DEPTH_U24_NORM_STENCIL_U8: return GL_DEPTH24_STENCIL8;
            case texture_data_type::STENCIL_U8: return GL_STENCIL_INDEX8;
            case texture_data_type::R_U8_NORM: return GL_R8;
            case texture_data_type::RG_U8_NORM: return GL_RG8;
            case texture_data_type::RGB_U8_NORM: return GL_RGB8;
            case texture_data_type::RGBA_U8_NORM: return GL_RGBA8;
            case texture_data_type::SRGB_U8_NORM: return GL_SRGB8;
            case texture_data_type::SRGBA_U8_NORM: return GL_SRGB8_ALPHA8;
            case texture_data_type::R_U8: return GL_R8UI;
            case texture_data_type::RG_U8: return GL_RG8UI;
            case texture_data_type::RGB_U8: return GL_RGB8UI;
            case texture_data_type::RGBA_U8: return GL_RGBA8UI;
            case texture_data_type::R_U16: return GL_R16UI;
            case texture_data_type::RG_U16: return GL_RG16UI;
            case texture_data_type::RGB_U16: return GL_RGB16UI;
            case texture_data_type::RGBA_U16: return GL_RGBA16UI;
            case texture_data_type::R_U32: return GL_R32UI;
            case texture_data_type::RG_U32: return GL_RG32UI;
            case texture_data_type::RGB_U32: return GL_RGB32UI;
            case texture_data_type::RGBA_U32: return GL_RGBA32UI;
            case texture_data_type::R_I8: return GL_R8I;
            case texture_data_type::RG_I8: return GL_RG8I;
            case texture_data_type::RGB_I8: return GL_RGB8I;
            case texture_data_type::RGBA_I8: return GL_RGBA8I;
            case texture_data_type::R_I16: return GL_R16I;
            case texture_data_type::RG_I16: return GL_RG16I;
            case texture_data_type::RGB_I16: return GL_RGB16I;
            case texture_data_type::RGBA_I16: return GL_RGBA16I;
            case texture_data_type::R_I32: return GL_R32I;
            case texture_data_type::RG_I32: return GL_RG32I;
            case texture_data_type::RGB_I32: return GL_RGB32I;
            case texture_data_type::RGBA_I32: return GL_RGBA32I;
            case texture_data_type::R_F16: return GL_R16F;
            case texture_data_type::RG_F16: return GL_RG16F;
            case texture_data_type::RGB_F16: return GL_RGB16F;
            case texture_data_type::RGBA_F16: return GL_RGBA16F;
            case texture_data_type::R_F32: return GL_R32F;
            case texture_data_type::RG_F32: return GL_RG32F;
            case texture_data_type::RGB_F32: return GL_RGB32F;
            case texture_data_type::RGBA_F32: return GL_RGBA32F;
        }

        return 0;
    }

    inline bool is_texture_data_type_integer(const texture_data_type type)
    {
        switch (type)
        {
            case texture_data_type::DEPTH_U32_NORM_STENCIL_U8:
            case texture_data_type::DEPTH_U24_NORM_STENCIL_U8:
            case texture_data_type::STENCIL_U8:
            case texture_data_type::R_U8:
            case texture_data_type::RG_U8:
            case texture_data_type::RGB_U8:
            case texture_data_type::RGBA_U8:
            case texture_data_type::R_U16:
            case texture_data_type::RG_U16:
            case texture_data_type::RGB_U16:
            case texture_data_type::RGBA_U16:
            case texture_data_type::R_U32:
            case texture_data_type::RG_U32:
            case texture_data_type::RGB_U32:
            case texture_data_type::RGBA_U32:
            case texture_data_type::R_I8:
            case texture_data_type::RG_I8:
            case texture_data_type::RGB_I8:
            case texture_data_type::RGBA_I8:
            case texture_data_type::R_I16:
            case texture_data_type::RG_I16:
            case texture_data_type::RGB_I16:
            case texture_data_type::RGBA_I16:
            case texture_data_type::R_I32:
            case texture_data_type::RG_I32:
            case texture_data_type::RGB_I32:
            case texture_data_type::RGBA_I32:
            {
                return true;
            }

            default:
            {
                return false;
            }
        }
    }

    inline u32 bytes_per_texture_data_element(const texture_data_type type)
    {
        switch (type)
        {
            case texture_data_type::R_U8:
            case texture_data_type::R_I8:
            case texture_data_type::STENCIL_U8:
            case texture_data_type::R_U8_NORM: return 1;
            case texture_data_type::RG_U8:
            case texture_data_type::RG_U8_NORM:
            case texture_data_type::R_U16:
            case texture_data_type::RG_I8:
            case texture_data_type::R_I16:
            case texture_data_type::DEPTH_U16_NORM:
            case texture_data_type::R_F16: return 2;
            case texture_data_type::RGB_U8:
            case texture_data_type::RGB_I8:
            case texture_data_type::DEPTH_U24_NORM:
            case texture_data_type::RGB_U8_NORM:
            case texture_data_type::SRGB_U8_NORM: return 3;
            case texture_data_type::RGBA_U8_NORM:
            case texture_data_type::SRGBA_U8_NORM:
            case texture_data_type::RG_F16:
            case texture_data_type::R_F32:
            case texture_data_type::RGBA_I8:
            case texture_data_type::RGBA_U8:
            case texture_data_type::RG_U16:
            case texture_data_type::R_U32:
            case texture_data_type::RG_I16:
            case texture_data_type::R_I32:
            case texture_data_type::DEPTH_U32_NORM:
            case texture_data_type::DEPTH_F32:
            case texture_data_type::DEPTH_U24_NORM_STENCIL_U8: return 4;
            case texture_data_type::DEPTH_U32_NORM_STENCIL_U8: return 5;
            case texture_data_type::RGB_F16:
            case texture_data_type::RGB_U16:
            case texture_data_type::RGB_I16: return 6;
            case texture_data_type::RG_U32:
            case texture_data_type::RGBA_U16:
            case texture_data_type::RGBA_I16:
            case texture_data_type::RGBA_F16:
            case texture_data_type::RG_F32:
            case texture_data_type::RG_I32: return 8;
            case texture_data_type::RGB_U32:
            case texture_data_type::RGB_F32:
            case texture_data_type::RGB_I32: return 12;
            case texture_data_type::RGBA_U32:
            case texture_data_type::RGBA_F32:
            case texture_data_type::RGBA_I32: return 16;
            default: return -1;
        }
    }

    inline bool does_texture_data_type_have_depth(const texture_data_type& texture_data_type)
    {
        switch (texture_data_type)
        {
            case texture_data_type::DEPTH_F32:
            case texture_data_type::DEPTH_U32_NORM:
            case texture_data_type::DEPTH_U24_NORM:
            case texture_data_type::DEPTH_U16_NORM:
            case texture_data_type::DEPTH_U32_NORM_STENCIL_U8:
            case texture_data_type::DEPTH_U24_NORM_STENCIL_U8:
            {
                return true;
            }
            default: return false;
        }
    }

    inline bool does_texture_data_type_have_stencil(const texture_data_type& texture_data_type)
    {
        switch (texture_data_type)
        {
            case texture_data_type::DEPTH_U32_NORM_STENCIL_U8:
            case texture_data_type::DEPTH_U24_NORM_STENCIL_U8:
            case texture_data_type::STENCIL_U8:
            {
                return true;
            }
            default: return false;
        }
    }

    inline bool is_texture_data_type_color_renderable(const texture_data_type& texture_data_type)
    {
        return !does_texture_data_type_have_depth(texture_data_type) && !does_texture_data_type_have_stencil(texture_data_type);
    }

    inline bool is_texture_data_type_depth_renderable(const texture_data_type& texture_data_type)
    {
        return does_texture_data_type_have_depth(texture_data_type);
    }

    inline bool is_texture_data_type_stencil_renderable(const texture_data_type& texture_data_type)
    {
        return does_texture_data_type_have_stencil(texture_data_type);
    }

    inline GLenum to_gl_memory_transfer_data_type(const texture_memory_transfer_info::pixel_data_type& data_type)
    {
        switch (data_type)
        {
            case texture_memory_transfer_info::pixel_data_type::U8: return GL_UNSIGNED_BYTE;
            case texture_memory_transfer_info::pixel_data_type::U32: return GL_UNSIGNED_INT;
            case texture_memory_transfer_info::pixel_data_type::I8: return GL_BYTE;
            case texture_memory_transfer_info::pixel_data_type::I32: return GL_INT;
            case texture_memory_transfer_info::pixel_data_type::F32: return GL_FLOAT;
        }
        return -1;
    }

    inline GLenum to_gl_channels_format(const texture_memory_transfer_info::pixel_channels& channels)
    {
        switch (channels)
        {
            case texture_memory_transfer_info::pixel_channels::R: return GL_RED;
            case texture_memory_transfer_info::pixel_channels::RG: return GL_RG;
            case texture_memory_transfer_info::pixel_channels::RGB: return GL_RGB;
            case texture_memory_transfer_info::pixel_channels::RGBA: return GL_RGBA;
            case texture_memory_transfer_info::pixel_channels::DEPTH: return GL_DEPTH_COMPONENT;
            case texture_memory_transfer_info::pixel_channels::STENCIL: return GL_STENCIL_INDEX;
        }
        return -1;
    }

    inline GLenum to_gl_filter_mode_from_modes(const texture_filtering_mode filter, const texture_filtering_mode mipmap_selection, const bool with_mipmaps)
    {
        if (!with_mipmaps)
        {
            switch (filter)
            {
                case texture_filtering_mode::NEAREST: return GL_NEAREST;
                case texture_filtering_mode::LINEAR: return GL_LINEAR;
                default: return GL_NEAREST;
            }
        }

        const u8 option = static_cast<u8>(filter) + static_cast<u8>(mipmap_selection) * 2;
        switch (option)
        {
            case 0: return GL_NEAREST_MIPMAP_NEAREST; //Nearest nearest
            case 1: return GL_LINEAR_MIPMAP_NEAREST;  //Linear nearest
            case 2: return GL_NEAREST_MIPMAP_LINEAR;  //Nearest linear
            case 3: return GL_LINEAR_MIPMAP_LINEAR;   //Linear linear
            default: return -1;
        }
    }

    inline GLenum to_gl_wrapping_mode(const texture_wrapping_mode mode)
    {
        switch (mode)
        {
            case texture_wrapping_mode::CLAMP: return GL_CLAMP;
            case texture_wrapping_mode::REPEAT: return GL_REPEAT;
            case texture_wrapping_mode::MIRROR: return GL_MIRRORED_REPEAT;
            case texture_wrapping_mode::BORDER: return GL_CLAMP_TO_BORDER;
        }
        return -1;
    }

    inline GLenum to_gl_swizzle_mode(const texture_swizzle swizzle)
    {
        switch (swizzle)
        {
            case texture_swizzle::RED: return GL_RED;
            case texture_swizzle::GREEN: return GL_GREEN;
            case texture_swizzle::BLUE: return GL_BLUE;
            case texture_swizzle::ALPHA: return GL_ALPHA;
        }
        return -1;
    }

    inline GLenum to_gl_draw_mode(const draw_mode mode)
    {
        switch (mode)
        {
            case draw_mode::TRIANGLES: return GL_TRIANGLES;
            case draw_mode::TRIANGLE_FAN: return GL_TRIANGLE_FAN;
            case draw_mode::TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
        }

        return -1;
    }

    inline GLenum to_gl_index_size(const draw_indexed_index_type type)
    {
        switch (type)
        {
            case draw_indexed_index_type::UINT_32: return GL_UNSIGNED_INT;
            case draw_indexed_index_type::UINT_16: return GL_UNSIGNED_SHORT;
            case draw_indexed_index_type::UINT_8: return GL_UNSIGNED_BYTE;
        }

        return -1;
    }

    inline u32 to_gl_type_size(const GLenum type)
    {
        switch (type)
        {
            case GL_INT: return sizeof(GLint);
            case GL_SHORT: return sizeof(GLshort);
            case GL_BYTE: return sizeof(GLbyte);
            case GL_UNSIGNED_INT: return sizeof(GLuint);
            case GL_UNSIGNED_SHORT: return sizeof(GLushort);
            case GL_UNSIGNED_BYTE: return sizeof(GLubyte);
            case GL_FLOAT: return sizeof(GLfloat);
            case GL_DOUBLE: return sizeof(GLdouble);
            default: return -1;
        }
    }

    inline GLenum to_gl_face_cull_mode(const face_cull_mode& mode)
    {
        switch (mode)
        {
            case BACK: return GL_BACK;
            case FRONT: return GL_FRONT;
            case BOTH: return GL_FRONT_AND_BACK;
            default: return -1;
        }
    }

    inline GLenum to_gl_depth_test_func(const depth_test_func& func)
    {
        switch (func)
        {
            case depth_test_func::ALWAYS: return GL_ALWAYS;
            case depth_test_func::NEVER: return GL_NEVER;
            case depth_test_func::LESS: return GL_LESS;
            case depth_test_func::LESS_EQUAL: return GL_LEQUAL;
            case depth_test_func::GREATER: return GL_GREATER;
            case depth_test_func::GREATER_EQUAL: return GL_GEQUAL;
            case depth_test_func::EQUAL: return GL_EQUAL;
            case depth_test_func::NOT_EQUAL: return GL_NOTEQUAL;
            default: return -1;
        }
    }

    inline GLbitfield to_gl_clear_mask(const clear_window_mode& mode)
    {
        switch (mode)
        {
            case clear_window_mode::COLOR: return GL_COLOR_BUFFER_BIT;
            case clear_window_mode::DEPTH: return GL_DEPTH_BUFFER_BIT;
            case clear_window_mode::STENCIL: return GL_STENCIL_BUFFER_BIT;
            case clear_window_mode::COLOR_AND_DEPTH: return GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
            case clear_window_mode::COLOR_AND_STENCIL: return GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
            case clear_window_mode::DEPTH_AND_STENCIL: return GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
            case clear_window_mode::ALL: return GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
        }
        return -1;
    }

    inline GLenum to_gl_blend_factor(const blending_factor factor)
    {
        switch (factor)
        {
            case blending_factor::ZERO: return GL_ZERO;
            case blending_factor::ONE: return GL_ONE;
            case blending_factor::CONSTANT_COLOR: return GL_CONSTANT_COLOR;
            case blending_factor::CONSTANT_ALPHA: return GL_CONSTANT_ALPHA;
            case blending_factor::ONE_MINUS_CONSTANT_COLOR: return GL_ONE_MINUS_CONSTANT_COLOR;
            case blending_factor::ONE_MINUS_CONSTANT_ALPHA: return GL_ONE_MINUS_CONSTANT_ALPHA;
            case blending_factor::SOURCE_COLOR: return GL_SRC_COLOR;
            case blending_factor::DEST_COLOR: return GL_DST_COLOR;
            case blending_factor::ONE_MINUS_SOURCE_COLOR: return GL_ONE_MINUS_SRC_COLOR;
            case blending_factor::ONE_MINUS_DEST_COLOR: return GL_ONE_MINUS_DST_COLOR;
            case blending_factor::SOURCE_ALPHA: return GL_SRC_ALPHA;
            case blending_factor::DEST_ALPHA: return GL_DST_ALPHA;
            case blending_factor::ONE_MINUS_SOURCE_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
            case blending_factor::ONE_MINUS_DEST_ALPHA: return GL_ONE_MINUS_DST_ALPHA;
            case blending_factor::SOURCE_ALPHA_SATURATE: return GL_SRC_ALPHA_SATURATE;
            case blending_factor::SECONDARY_SOURCE_COLOR: return GL_SRC1_COLOR;
            case blending_factor::SECONDARY_SOURCE_ALPHA: return GL_SRC1_ALPHA;
        }
        return -1;
    }

    inline GLenum to_gl_blend_func(const blending_func func)
    {
        switch (func)
        {
            case blending_func::ADD: return GL_FUNC_ADD;
            case blending_func::SUBTRACT: return GL_FUNC_SUBTRACT;
            case blending_func::REVERSE_SUBTRACT: return GL_FUNC_REVERSE_SUBTRACT;
            case blending_func::MIN: return GL_MIN;
            case blending_func::MAX: return GL_MAX;
        }

        return -1;
    }

    inline GLenum to_gl_stencil_facing(const stencil_facing facing)
    {
        switch (facing)
        {
            case stencil_facing::FRONT: return GL_FRONT;
            case stencil_facing::BACK: return GL_BACK;
            case stencil_facing::BOTH: return GL_FRONT_AND_BACK;
        }

        return -1;
    }

    inline GLenum to_gl_stencil_test_func(const stencil_test_func test_func)
    {
        switch (test_func)
        {
            case stencil_test_func::ALWAYS: return GL_ALWAYS;
            case stencil_test_func::NEVER: return GL_NEVER;
            case stencil_test_func::LESS: return GL_LESS;
            case stencil_test_func::LESS_EQUAL: return GL_LEQUAL;
            case stencil_test_func::GREATER: return GL_GREATER;
            case stencil_test_func::GREATER_EQUAL: return GL_GEQUAL;
            case stencil_test_func::EQUAL: return GL_EQUAL;
            case stencil_test_func::NOT_EQUAL: return GL_NOTEQUAL;
        }
        return -1;
    }

    inline GLenum to_gl_stencil_test_op(const stencil_result_op stencil_op)
    {
        switch (stencil_op)
        {
            case stencil_result_op::KEEP: return GL_KEEP;
            case stencil_result_op::ZERO: return GL_ZERO;
            case stencil_result_op::REPLACE: return GL_REPLACE;
            case stencil_result_op::INCREMENT: return GL_INCR;
            case stencil_result_op::INCREMENT_WRAP: return GL_INCR_WRAP;
            case stencil_result_op::DECREMENT: return GL_DECR;
            case stencil_result_op::DECREMENT_WRAP: return GL_DECR_WRAP;
            case stencil_result_op::INVERT: return GL_INVERT;
        }
        return -1;
    }

    [[nodiscard]] constexpr u32 vertex_element_size(const vertex_data_type type)
    {
        switch (type)
        {
            case vertex_data_type::UINT_U8:
            case vertex_data_type::INT_I8:
            case vertex_data_type::FLOAT_U8_NORM:
            case vertex_data_type::FLOAT_I8_NORM: return 1;

            case vertex_data_type::UINT2_U8:
            case vertex_data_type::UINT_U16:
            case vertex_data_type::INT2_I8:
            case vertex_data_type::INT_I16:
            case vertex_data_type::FLOAT2_U8_NORM:
            case vertex_data_type::FLOAT2_I8_NORM:
            case vertex_data_type::FLOAT_U16_NORM:
            case vertex_data_type::FLOAT_I16_NORM:
            case vertex_data_type::FLOAT_F16: return 2;

            case vertex_data_type::UINT3_U8:
            case vertex_data_type::INT3_I8:
            case vertex_data_type::FLOAT3_U8_NORM:
            case vertex_data_type::FLOAT3_I8_NORM: return 3;

            case vertex_data_type::UINT4_U8:
            case vertex_data_type::UINT2_U16:
            case vertex_data_type::UINT_U32:
            case vertex_data_type::INT4_I8:
            case vertex_data_type::INT2_I16:
            case vertex_data_type::INT_I32:
            case vertex_data_type::FLOAT4_U8_NORM:
            case vertex_data_type::FLOAT4_I8_NORM:
            case vertex_data_type::FLOAT2_U16_NORM:
            case vertex_data_type::FLOAT2_I16_NORM:
            case vertex_data_type::FLOAT2_F16:
            case vertex_data_type::FLOAT_F32: return 4;

            case vertex_data_type::UINT3_U16:
            case vertex_data_type::INT3_I16:
            case vertex_data_type::FLOAT3_U16_NORM:
            case vertex_data_type::FLOAT3_I16_NORM:
            case vertex_data_type::FLOAT3_F16: return 6;

            case vertex_data_type::UINT4_U16:
            case vertex_data_type::UINT2_U32:
            case vertex_data_type::INT4_I16:
            case vertex_data_type::INT2_I32:
            case vertex_data_type::FLOAT4_U16_NORM:
            case vertex_data_type::FLOAT4_I16_NORM:
            case vertex_data_type::FLOAT4_F16:
            case vertex_data_type::FLOAT2_F32: return 8;

            case vertex_data_type::UINT3_U32:
            case vertex_data_type::INT3_I32:
            case vertex_data_type::FLOAT3_F32: return 12;

            case vertex_data_type::UINT4_U32:
            case vertex_data_type::INT4_I32:
            case vertex_data_type::FLOAT4_F32: return 16;
        }
        return 0;
    }

    inline std::tuple<GLenum, GLuint, bool, bool> gl_vertex_element_data_type(const vertex_data_type& type)
    {
        switch (type)
        {
            case vertex_data_type::UINT_U8: return {GL_UNSIGNED_BYTE, 1, false, true};
            case vertex_data_type::UINT2_U8: return {GL_UNSIGNED_BYTE, 2, false, true};
            case vertex_data_type::UINT3_U8: return {GL_UNSIGNED_BYTE, 3, false, true};
            case vertex_data_type::UINT4_U8: return {GL_UNSIGNED_BYTE, 4, false, true};
            case vertex_data_type::UINT_U16: return {GL_UNSIGNED_SHORT, 1, false, true};
            case vertex_data_type::UINT2_U16: return {GL_UNSIGNED_SHORT, 2, false, true};
            case vertex_data_type::UINT3_U16: return {GL_UNSIGNED_SHORT, 3, false, true};
            case vertex_data_type::UINT4_U16: return {GL_UNSIGNED_SHORT, 4, false, true};
            case vertex_data_type::UINT_U32: return {GL_UNSIGNED_INT, 1, false, true};
            case vertex_data_type::UINT2_U32: return {GL_UNSIGNED_INT, 2, false, true};
            case vertex_data_type::UINT3_U32: return {GL_UNSIGNED_INT, 3, false, true};
            case vertex_data_type::UINT4_U32: return {GL_UNSIGNED_INT, 4, false, true};
            case vertex_data_type::INT_I8: return {GL_BYTE, 1, false, true};
            case vertex_data_type::INT2_I8: return {GL_BYTE, 2, false, true};
            case vertex_data_type::INT3_I8: return {GL_BYTE, 3, false, true};
            case vertex_data_type::INT4_I8: return {GL_BYTE, 4, false, true};
            case vertex_data_type::INT_I16: return {GL_SHORT, 1, false, true};
            case vertex_data_type::INT2_I16: return {GL_SHORT, 2, false, true};
            case vertex_data_type::INT3_I16: return {GL_SHORT, 3, false, true};
            case vertex_data_type::INT4_I16: return {GL_SHORT, 4, false, true};
            case vertex_data_type::INT_I32: return {GL_INT, 1, false, true};
            case vertex_data_type::INT2_I32: return {GL_INT, 2, false, true};
            case vertex_data_type::INT3_I32: return {GL_INT, 3, false, true};
            case vertex_data_type::INT4_I32: return {GL_INT, 4, false, true};
            case vertex_data_type::FLOAT_U8_NORM: return {GL_UNSIGNED_BYTE, 1, true, false};
            case vertex_data_type::FLOAT2_U8_NORM: return {GL_UNSIGNED_BYTE, 2, true, false};
            case vertex_data_type::FLOAT3_U8_NORM: return {GL_UNSIGNED_BYTE, 3, true, false};
            case vertex_data_type::FLOAT4_U8_NORM: return {GL_UNSIGNED_BYTE, 4, true, false};
            case vertex_data_type::FLOAT_I8_NORM: return {GL_BYTE, 1, true, false};
            case vertex_data_type::FLOAT2_I8_NORM: return {GL_BYTE, 2, true, false};
            case vertex_data_type::FLOAT3_I8_NORM: return {GL_BYTE, 3, true, false};
            case vertex_data_type::FLOAT4_I8_NORM: return {GL_BYTE, 4, true, false};
            case vertex_data_type::FLOAT_U16_NORM: return {GL_UNSIGNED_SHORT, 1, true, false};
            case vertex_data_type::FLOAT2_U16_NORM: return {GL_UNSIGNED_SHORT, 2, true, false};
            case vertex_data_type::FLOAT3_U16_NORM: return {GL_UNSIGNED_SHORT, 3, true, false};
            case vertex_data_type::FLOAT4_U16_NORM: return {GL_UNSIGNED_SHORT, 4, true, false};
            case vertex_data_type::FLOAT_I16_NORM: return {GL_SHORT, 1, true, false};
            case vertex_data_type::FLOAT2_I16_NORM: return {GL_SHORT, 2, true, false};
            case vertex_data_type::FLOAT3_I16_NORM: return {GL_SHORT, 3, true, false};
            case vertex_data_type::FLOAT4_I16_NORM: return {GL_SHORT, 4, true, false};
            case vertex_data_type::FLOAT_F16: return {GL_HALF_FLOAT, 1, false, false};
            case vertex_data_type::FLOAT2_F16: return {GL_HALF_FLOAT, 2, false, false};
            case vertex_data_type::FLOAT3_F16: return {GL_HALF_FLOAT, 3, false, false};
            case vertex_data_type::FLOAT4_F16: return {GL_HALF_FLOAT, 4, false, false};
            case vertex_data_type::FLOAT_F32: return {GL_FLOAT, 1, false, false};
            case vertex_data_type::FLOAT2_F32: return {GL_FLOAT, 2, false, false};
            case vertex_data_type::FLOAT3_F32: return {GL_FLOAT, 3, false, false};
            case vertex_data_type::FLOAT4_F32: return {GL_FLOAT, 4, false, false};
        }
        return {-1, -1, false, false};
    }
}
