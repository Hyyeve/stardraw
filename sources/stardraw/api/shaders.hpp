#pragma once
#include <memory>
#include <string_view>
#include <vector>

#include "starlib/types/graphics.hpp"
#include "starlib/types/starlib_stdint.hpp"
#include "starlib/types/status.hpp"

namespace stardraw
{
    enum class shader_stage_type
    {
        UNKNOWN, VERTEX, GEOMETRY, FRAGMENT, COMPUTE
        //todo: raytracing stages?
    };

    struct shader_buffer_layout
    {
        struct pad
        {
            starlib_stdint::u64 address;
            starlib_stdint::u64 size;
        };

        starlib_stdint::u64 packed_size = 0;
        starlib_stdint::u64 padded_size = 0;
        std::vector<pad> pads;
    };

    struct shader_module
    {
        bool operator==(const shader_module& other) const = default;
        struct shader_module_internal;
        std::shared_ptr<shader_module_internal> internal;
        ~shader_module();
    };

    struct shader_program
    {
        bool operator==(const shader_program& other) const = default;
        struct shader_program_internal;
        std::shared_ptr<shader_program_internal> internal;
        ~shader_program();
    };

    struct shader_parameter_location
    {
        ///Navigate to an index within the located array-like field or buffer
        [[nodiscard]] shader_parameter_location index(starlib_stdint::u32 index) const;
        ///Navigate to a named field within the located structure
        [[nodiscard]] shader_parameter_location field(const std::string_view& name) const;

        shader_parameter_location(const shader_parameter_location& other);
        shader_parameter_location& operator=(const shader_parameter_location& other);
        shader_parameter_location(shader_parameter_location&& other) noexcept;
        shader_parameter_location& operator=(shader_parameter_location&& other) noexcept;

        bool operator==(const shader_parameter_location& other) const;
        struct shader_parameter_location_internal;
        std::unique_ptr<shader_parameter_location_internal> internal;

        explicit shader_parameter_location(std::unique_ptr<shader_parameter_location_internal>&& internal);
        ~shader_parameter_location();
    };

    struct shader_stage
    {
        ///Locates a uniform or other exposed type by name
        [[nodiscard]] shader_parameter_location locate(const std::string_view& name) const;

        ///Number of bytes required to store all variables of a shader buffer. -1 indicates an unknown or unsized buffer.
        ///Array type buffers return the size of a single array element.
        [[nodiscard]] starlib_stdint::i64 buffer_size(const std::string_view& name) const;

        ///Get the type of shader stage
        [[nodiscard]] shader_stage_type get_stage_type() const;

        struct shader_stage_internal;
        std::shared_ptr<shader_stage_internal> internal;
        ~shader_stage();
    };

    struct shader_macro
    {
        std::string name;
        std::string value;
    };

    struct shader_entry_point
    {
        shader_module module;
        std::string entry_point_name;
        bool operator==(const shader_entry_point& key) const = default;
    };

    [[nodiscard]] starlib::status setup_shader_compiler(const std::vector<shader_macro>& macro_defines = {});
    [[nodiscard]] starlib::status cleanup_shader_compiler();

    [[nodiscard]] starlib::status load_shader_module(const std::string_view& source, shader_module& out_shader_module);
    [[nodiscard]] starlib::status load_shader_module(const void* cache_ptr, const starlib_stdint::u64 cache_size, shader_module& out_shader_module);
    [[nodiscard]] starlib::status cache_shader_module(const shader_module& module, void*& out_cache_ptr, starlib_stdint::u64& out_cache_size);

    [[nodiscard]] starlib::status link_shader_program(const std::vector<shader_entry_point>& entry_points, shader_program& out_linked_shader, const std::vector<shader_module>& additional_modules = {});
    [[nodiscard]] starlib::status create_shader_stage(const shader_program& linked_shader, const shader_entry_point& entry_point, const starlib::graphics_api& api, shader_stage& out_shader_stage);
    [[nodiscard]] starlib::status create_shader_buffer_layout(const shader_stage& program, const std::string_view& buffer_name, shader_buffer_layout& out_buffer_layout);

    /*
    Allocates and returns a new block of memory with the data laid out according to api-specific memory layout rules for the given buffer layout.
    Input data is assumed to be tightly packed.
    Only 32 bit integer types, 32 bit floating point types, and composite types made of those are guarenteed to be supported.
    Support for other types is dependent on the api, and attempting to use types an API does not support is undefined behaviour.
    */
    [[nodiscard]] void* layout_shader_buffer_memory(const shader_buffer_layout& layout, const void* data, const starlib_stdint::u64 data_size);
}
