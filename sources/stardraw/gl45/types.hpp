#pragma once
#include <cstdint>

#include "glad/glad.h"
#include "stardraw/api/descriptors.hpp"

namespace stardraw::gl45
{
    [[nodiscard]] constexpr uint32_t vertex_element_size(const vertex_element_type type)
    {
        switch (type)
        {
            case UINT_U8:
            case INT_I8:
            case FLOAT_U8_NORM:
            case FLOAT_I8_NORM: return 1;

            case UNT2_U8:
            case UINT_U16:
            case INT2_I8:
            case INT_I16:
            case FLOAT2_U8_NORM:
            case FLOAT2_I8_NORM:
            case FLOAT_U16_NORM:
            case FLOAT_I16_NORM:
            case FLOAT_F16: return 2;

            case UINT3_U8:
            case INT3_I8:
            case FLOAT3_U8_NORM:
            case FLOAT3_I8_NORM: return 3;

            case UINT4_U8:
            case UINT2_U16:
            case UINT_U32:
            case INT4_I8:
            case INT2_I16:
            case INT_I32:
            case FLOAT4_U8_NORM:
            case FLOAT4_I8_NORM:
            case FLOAT2_U16_NORM:
            case FLOAT2_I16_NORM:
            case FLOAT2_F16:
            case FLOAT_F32: return 4;

            case UINT3_U16:
            case INT3_I16:
            case FLOAT3_U16_NORM:
            case FLOAT3_I16_NORM:
            case FLOAT3_F16: return 6;

            case UINT4_U16:
            case UINT2_U32:
            case INT4_I16:
            case INT2_I32:
            case FLOAT4_U16_NORM:
            case FLOAT4_I16_NORM:
            case FLOAT4_F16:
            case FLOAT2_F32: return 8;

            case UINT3_U32:
            case INT3_I32:
            case FLOAT3_F32: return 12;

            case UINT4_U32:
            case INT4_I32:
            case FLOAT4_F32: return 16;
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
