#pragma once
#include <slang.h>

#include "stardraw/api/shaders.hpp"

template <>
struct std::hash<stardraw::shader_parameter_location>
{
    std::size_t operator()(const stardraw::shader_parameter_location& key) const noexcept
    {
        return hash<starlib_stdint::u32>()(key.binding_range_index + key.binding_range + key.byte_address + key.root_idx);
    }
};

namespace stardraw
{
    using namespace starlib_stdint;

    struct binding_location_info
    {
        i64 set;
        i64 slot;
        //The binding type that the location exists inside
        //May not always be the same as the actual variable the location references -
        //for instance, for plain data, it will be the containing buffer variable.
        slang::TypeLayoutReflection* binding_type;
    };

    constexpr shader_parameter_location invalid_shader_paramter_location = {
        nullptr, nullptr, 0, 0, 0, 0
    };

    slang::TypeLayoutReflection* slang_type_reflection(const shader_parameter_location& location);
    slang::VariableLayoutReflection* slang_root_var_reflection(const shader_parameter_location& location);
    slang::ShaderReflection* slang_shader_reflection(const shader_program* program);
    binding_location_info vk_binding_for_location(const shader_parameter_location& location);
}
