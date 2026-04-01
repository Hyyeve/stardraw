#pragma once
#include <slang.h>

#include "internal.hpp"
#include "stardraw/api/shaders.hpp"

namespace stardraw
{
    using namespace starlib;

    struct binding_location_info
    {
        i64 set;
        i64 slot;
        //The binding type that the location exists inside
        //May not always be the same as the actual variable the location references -
        //for instance, for plain data, it will be the containing buffer variable.
        slang::TypeLayoutReflection* binding_type_ptr;
    };

    struct shader_parameter_location::shader_parameter_location_internal
    {
        slang::VariableLayoutReflection* root_ptr = nullptr;
        slang::TypeLayoutReflection* offset_ptr = nullptr;
        u64 root_idx = 0;
        u64 byte_address = 0;
        u64 binding_range = 0;
        u64 binding_range_index = 0;
        std::string path_string = "???";
        bool is_valid = true;
        bool operator==(const shader_parameter_location_internal&) const = default;
    };

    struct shader_stage::shader_stage_internal
    {
        void* data;
        slang::ShaderReflection* reflection;
        u32 data_size;
        shader_stage_type type;
        graphics_api api;

        ~shader_stage_internal()
        {
            free(data);
        }
    };

    binding_location_info vk_binding_for_location(const shader_parameter_location& location);
}

template <>
struct std::hash<stardraw::shader_parameter_location>
{
    std::size_t operator()(const stardraw::shader_parameter_location& key) const noexcept
    {
        return hash<starlib::u32>()(key.internal->binding_range_index + key.internal->binding_range + key.internal->byte_address + key.internal->root_idx);
    }
};

