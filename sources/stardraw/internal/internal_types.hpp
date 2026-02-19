#pragma once
#include <slang.h>

#include "stardraw/api/shaders.hpp"

namespace stardraw
{
    const shader_parameter_location invalid_shader_paramter_location = {
        "???", nullptr, 0, 0, 0, 0
    };

    inline slang::TypeLayoutReflection* type_layout_of(const shader_parameter_location& location)
    {
        return static_cast<slang::TypeLayoutReflection*>(location.internal_ptr);
    }

    inline slang::ShaderReflection* shader_reflection_of(const shader_program* program)
    {
        return static_cast<slang::ShaderReflection*>(program->internal_ptr);
    }
}
