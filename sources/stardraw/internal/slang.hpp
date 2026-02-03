#pragma once

#include "../api/types.hpp"
#include <slang.h>

namespace stardraw
{
    struct shader_macro
    {
        std::string name;
        std::string value;
    };

    struct shader_data
    {
        void* data;
        uint32_t data_size;
        slang::ShaderReflection* reflection_data;
    };

    [[nodiscard]] status init_slang_session();
    [[nodiscard]] status load_slang_module(const std::string_view& name, const std::string_view& source);
    [[nodiscard]] status link_slang_shader(const std::string& shader_name, const std::string& entry_point_module, const std::string& entry_point_name, const std::vector<std::string>& additional_modules);
    [[nodiscard]] status link_slang_shader(const std::string& shader_name, const std::string& entry_point_module, const std::string& entry_point_name);
    [[nodiscard]] status get_shader_data(const std::string& shader_name, const graphics_api& api, shader_data& out_shader_data);

    struct shader_param_location
    {
        slang::TypeLayoutReflection* type_layout = nullptr;
        int32_t byte_address = 0;
        uint32_t binding_range = 0;
        uint32_t binding_index = 0;
    };

    constexpr shader_param_location invalid_location = {
        nullptr, -1, 0, 0
    };

    [[nodiscard]] shader_param_location locate_shader_param(slang::ShaderReflection* shader_layout, const std::string_view& name, const uint32_t index = -1u);
}
