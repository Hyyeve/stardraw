#pragma once

#include "stardraw/api/descriptors.hpp"
#include "stardraw/gl45/gl_headers.hpp"

namespace stardraw::gl45
{
    [[nodiscard]] constexpr uint32_t vertex_element_size(const vertex_data_type type)
    {
        switch (type)
        {
            case vertex_data_type::UINT_U8:
            case vertex_data_type::INT_I8:
            case vertex_data_type::FLOAT_U8_NORM:
            case vertex_data_type::FLOAT_I8_NORM: return 1;

            case vertex_data_type::UNT2_U8:
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

    #pragma pack(push, 1)
    struct draw_arrays_indirect_params
    {
        uint32_t vertex_count;
        uint32_t instance_count;
        uint32_t vertex_begin;
        uint32_t instance_begin;
    };

    struct draw_elements_indirect_params
    {
        uint32_t vertex_count;
        uint32_t instance_count;
        uint32_t index_begin;
        int32_t vertex_begin;
        uint32_t instance_begin;
    };
    #pragma pack(pop)

    class object_state
    {
    public:
        virtual ~object_state() = default;
        [[nodiscard]] virtual descriptor_type object_type() const = 0;
    };

    struct signal_state
    {
        GLsync sync_point;
    };
}
