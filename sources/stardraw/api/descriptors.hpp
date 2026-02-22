#pragma once
#include <string>
#include <utility>

#include "shaders.hpp"
#include "types.hpp"

namespace stardraw
{
    enum class descriptor_type : uint8_t
    {
        BUFFER, SHADER, VERTEX_SPECIFICATION, SHADER_SPECIFICATION, DRAW_SPECIFICATION,
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

    typedef std::vector<polymorphic_ptr<descriptor>> descriptor_list;

    ///NOTE: Buffer memory storage cannot be guarenteed on OpenGL, but SYSRAM guarentees it will be possible to write into the buffer directly.
    enum class buffer_memory_storage : uint8_t
    {
        SYSRAM, VRAM,
    };

    struct buffer_descriptor final : descriptor
    {
        explicit buffer_descriptor(const std::string_view& name, const uint64_t size, const buffer_memory_storage memory = buffer_memory_storage::VRAM) : descriptor(name), size(size), memory(memory) {}

        [[nodiscard]] descriptor_type type() const override
        {
            return descriptor_type::BUFFER;
        }

        uint64_t size;
        buffer_memory_storage memory;
    };

    enum class vertex_data_type : uint8_t
    {
        UINT_U8, UNT2_U8, UINT3_U8, UINT4_U8,
        UINT_U16, UINT2_U16, UINT3_U16, UINT4_U16,
        UINT_U32, UINT2_U32, UINT3_U32, UINT4_U32,

        INT_I8, INT2_I8, INT3_I8, INT4_I8,
        INT_I16, INT2_I16, INT3_I16, INT4_I16,
        INT_I32, INT2_I32, INT3_I32, INT4_I32,

        FLOAT_U8_NORM, FLOAT2_U8_NORM, FLOAT3_U8_NORM, FLOAT4_U8_NORM,
        FLOAT_I8_NORM, FLOAT2_I8_NORM, FLOAT3_I8_NORM, FLOAT4_I8_NORM,

        FLOAT_U16_NORM, FLOAT2_U16_NORM, FLOAT3_U16_NORM, FLOAT4_U16_NORM,
        FLOAT_I16_NORM, FLOAT2_I16_NORM, FLOAT3_I16_NORM, FLOAT4_I16_NORM,
        FLOAT_F16, FLOAT2_F16, FLOAT3_F16, FLOAT4_F16,

        FLOAT_F32, FLOAT2_F32, FLOAT3_F32, FLOAT4_F32
    };

    struct vertex_data_binding
    {
        constexpr vertex_data_binding(const std::string_view& buffer, const vertex_data_type& type, const uint32_t instance_divisor = 0) : type(type), instance_divisor(instance_divisor), buffer(buffer) {}

        vertex_data_type type;
        uint32_t instance_divisor;
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

    struct shader_data_binding
    {
        std::string buffer;
        std::string binding;
    };

    struct shader_specification_descriptor final : descriptor
    {
        shader_specification_descriptor(const std::string_view& name, const std::string_view& shader, const std::vector<shader_data_binding>& shader_bindings = {}) : descriptor(name), shader(shader), shader_bindings(shader_bindings) {}

        [[nodiscard]] descriptor_type type() const override
        {
            return descriptor_type::SHADER_SPECIFICATION;
        }

        std::string shader;
        std::vector<shader_data_binding> shader_bindings;
    };

    struct draw_specification_descriptor final : descriptor
    {
        draw_specification_descriptor(const std::string_view& name, const std::string_view& vertex_specification, const std::string_view& shader_specification) : descriptor(name), vertex_specification(vertex_specification), shader_specification(shader_specification) {}
        [[nodiscard]] descriptor_type type() const override
        {
            return descriptor_type::DRAW_SPECIFICATION;
        }

        std::string vertex_specification;
        std::string shader_specification;
    };

    struct shader_descriptor final : descriptor
    {
        shader_descriptor(const std::string_view& name, const std::vector<shader_stage>& stages) : descriptor(name), stages(stages), cache_ptr(nullptr), cache_size(0) {}
        shader_descriptor(const std::string_view& name, const void* cache_ptr, const uint64_t cache_size) : descriptor(name), stages({}), cache_ptr(cache_ptr), cache_size(cache_size) {}

        [[nodiscard]] descriptor_type type() const override
        {
            return descriptor_type::SHADER;
        }

        std::vector<shader_stage> stages;
        const void* cache_ptr;
        const uint64_t cache_size;
    };
}
