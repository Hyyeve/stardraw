#pragma once
#include <slang.h>

#include "stardraw/api/shaders.hpp"

namespace stardraw
{
    struct binding_location_info
    {
        int64_t set;
        int64_t slot;
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
