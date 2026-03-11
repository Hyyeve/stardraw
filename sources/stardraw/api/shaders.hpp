#pragma once
#include <string_view>
#include <vector>

#include "common.hpp"
#include "starlib/types/starlib_stdint.hpp"

namespace stardraw
{
    using namespace starlib_stdint;
    enum class shader_stage_type
    {
        VERTEX, TESSELATION_CONTROL, TESSELATION_EVAL, GEOMETRY, FRAGMENT, COMPUTE
        //todo: raytracing stages for other backends?
    };

    struct shader_buffer_layout
    {
        struct pad
        {
            u64 address;
            u64 size;
        };

        u64 packed_size = 0;
        u64 padded_size = 0;
        std::vector<pad> pads;
    };

    struct shader_parameter_location
    {
        [[nodiscard]] shader_parameter_location index(u32 index) const;
        [[nodiscard]] shader_parameter_location field(const std::string_view& name) const;
        [[nodiscard]] bool operator==(const shader_parameter_location& other) const = default;

        void* root_ptr = nullptr;
        void* offset_ptr = nullptr;
        u32 root_idx = 0;
        u32 byte_address = 0;
        u32 binding_range = 0;
        u32 binding_range_index = 0;
    };

    struct shader_program
    {
        [[nodiscard]] shader_parameter_location locate(const std::string_view& name) const;

        //Number of bytes required to store all variables of a shader buffer. -1 indicates an unknown or unsized buffer.
        [[nodiscard]] i64 buffer_size(const std::string_view& name) const;

        void* data;
        u32 data_size;
        void* internal_ptr;
        graphics_api api;
    };

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

    struct shader_entry_point
    {
        std::string module_name;
        std::string entry_point_name;
        bool operator==(const shader_entry_point& key) const = default;
    };

    [[nodiscard]] status setup_shader_compiler(const std::vector<shader_macro>& macro_defines = {});
    [[nodiscard]] status load_shader_module(const std::string_view& module_name, const std::string_view& source);
    [[nodiscard]] status load_shader_module(const std::string_view& module_name, const void* cache_ptr, const u64 cache_size);
    [[nodiscard]] status cache_shader_module(const std::string& module_name, void** out_cache_ptr, u64& out_cache_size);
    [[nodiscard]] status link_shader_modules(const std::string& linked_set_name, const std::vector<shader_entry_point>& entry_points, const std::vector<std::string>& additional_modules = {});

    [[nodiscard]] status create_shader_program(const std::string& linked_set_name, const shader_entry_point& entry_point, const graphics_api& api, shader_program** out_shader_program);
    [[nodiscard]] status delete_shader_program(shader_program** shader_program);

    [[nodiscard]] status create_shader_buffer_layout(const shader_program* program, const std::string_view& buffer_name, shader_buffer_layout** out_buffer_layout);
    [[nodiscard]] status delete_shader_buffer_layout(shader_buffer_layout** buffer_layout);

    /*
    Allocates and returns a new block of memory with the data laid out according to api-specific memory layout rules for the given buffer layout.
    Input data is assumed to be tightly packed.
    Only 32 bit integer types, 32 bit floating point types, and composite types made of those are guarenteed to be supported.
    Support for other types is dependent on the api, and attempting to use types an API does not support is undefined behaviour.
    */
    [[nodiscard]] void* layout_shader_buffer_memory(const shader_buffer_layout* layout, const void* data, const u64 data_size);
}
