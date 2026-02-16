#pragma once
#include <vector>

#include "types.hpp"

namespace stardraw
{
    enum class shader_stage_type
    {
        VERTEX, TESSELATION_CONTROL, TESSELATION_EVAL, GEOMETRY, FRAGMENT, COMPUTE
        //todo: raytracing stages for other backends?
    };

    struct shader_buffer_layout;
    struct shader_program;

    struct shader_macro
    {
        std::string name;
        std::string value;
    };

    struct shader_stage
    {
        shader_stage_type type;
        shader_program* program;
    };

    [[nodiscard]] status setup_shader_compiler(const std::vector<shader_macro>& macro_defines = {});
    [[nodiscard]] status load_shader_module(const std::string_view& module_name, const std::string_view& source);
    [[nodiscard]] status link_shader(const std::string& shader_name, const std::string& entry_point_module, const std::string& entry_point_name, const std::vector<std::string>& additional_modules = {});

    [[nodiscard]] status create_shader_program(const std::string& shader_name, const graphics_api& api, shader_program** out_shader_program);
    [[nodiscard]] status delete_shader_program(shader_program** shader_program);

    [[nodiscard]] status create_shader_buffer_layout(const shader_program* program, const std::string_view& buffer_name, shader_buffer_layout** out_buffer_layout);
    [[nodiscard]] status delete_shader_buffer_layout(shader_buffer_layout** buffer_layout);

    /*
    Allocates and returns a new block of memory with the data laid out according to api-specific memory layout rules for the given buffer layout.
    Input data is assumed to be tightly packed.
    Only 32 bit integer types, 32 bit floating point types, and composite types made of those are guarenteed to be supported.
    Support for other types is dependent on the api, and attempting to use types an API does not support is undefined behaviour.
    */
    [[nodiscard]] void* layout_shader_buffer_memory(const shader_buffer_layout* layout, const void* data, const uint64_t data_size);
}
