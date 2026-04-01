#pragma once
#include <memory>
#include <string_view>
#include <vector>

#include "memory_transfer.hpp"
#include "starlib/types/graphics.hpp"
#include "starlib/types/starlib_stdint.hpp"
#include "starlib/types/status.hpp"

namespace stardraw
{
    ///Options for shader stages
    enum class shader_stage_type
    {
        UNKNOWN, VERTEX, GEOMETRY, FRAGMENT, COMPUTE
        //todo: raytracing stages?
    };

    ///Opaque type that represents a 'shader module' as Slang uses the term (usually a single loaded shader file that may or may not contain entry points)
    struct shader_module
    {
        bool operator==(const shader_module& other) const = default;
        struct shader_module_internal;
        std::shared_ptr<shader_module_internal> internal;
        ~shader_module();
    };

    ///Opaque type that represents a linked 'shader progran' - a collection of entry points with their code and dependencies resolved.
    struct shader_program
    {
        bool operator==(const shader_program& other) const = default;
        struct shader_program_internal;
        std::shared_ptr<shader_program_internal> internal;
        ~shader_program();
    };

    ///Opaque type that represents a 'shader parameter location' - a memory location in a shader (usually a field/index into a field) that data can be written to.
    struct shader_parameter_location
    {
        ///Navigate to an index within the located array-like field or buffer
        [[nodiscard]] shader_parameter_location index(starlib::u32 index) const;
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

    ///Opaque type that represents a 'shader stage' - a single shader entry point for a specific type of stage, with associated data.
    struct shader_stage
    {
        ///Locates a uniform or other exposed type by name
        [[nodiscard]] shader_parameter_location locate(const std::string_view& name) const;

        ///Number of bytes required to store all variables of a shader buffer. -1 indicates an unknown or unsized buffer.
        [[nodiscard]] starlib::i64 buffer_size(const std::string_view& name) const;

        ///Get the type of shader stage
        [[nodiscard]] shader_stage_type get_stage_type() const;

        struct shader_stage_internal;
        std::shared_ptr<shader_stage_internal> internal;
        ~shader_stage();
    };

    ///A shader macro definition, of the form '#define {name} {value}'
    struct shader_macro
    {
        std::string name;
        std::string value;
    };

    ///A shader entry point, specified by module and entry point name
    struct shader_entry_point
    {
        shader_module module;
        std::string entry_point_name;
        bool operator==(const shader_entry_point& key) const = default;
    };

    ///Initialize the Slang shader compiler and related data.
    [[nodiscard]] starlib::status setup_shader_compiler(const std::vector<shader_macro>& macro_defines = {});

    ///Uninitialize the Slang shader compiler and related data.
    ///NOTE: Calling this function will invalidate the internal pointers that are relied upon for the opaque types defined above.
    ///Be careful not to call this while you are still relying on extracting data from any of those! Doing so may result in unexpected behaviour or crashes.
    [[nodiscard]] starlib::status cleanup_shader_compiler();

    ///Load a shader module from Slang source code.
    [[nodiscard]] starlib::status load_shader_module(const std::string_view& source, shader_module& out_shader_module);

    ///Load a shader module from cache data previously generated.
    [[nodiscard]] starlib::status load_shader_module(const void* cache_ptr, const starlib::u64 cache_size, shader_module& out_shader_module);

    ///Create cache data for a shader module.
    [[nodiscard]] starlib::status cache_shader_module(const shader_module& module, void*& out_cache_ptr, starlib::u64& out_cache_size);

    ///Link some numnber of entry points into a shader program
    [[nodiscard]] starlib::status link_shader_program(const std::vector<shader_entry_point>& entry_points, shader_program& out_linked_shader, const std::vector<shader_module>& additional_modules = {});

    ///Extract the API-specifc shader stage data for a particular entry point from a linked shader program.
    [[nodiscard]] starlib::status create_shader_stage(const shader_program& linked_shader, const shader_entry_point& entry_point, const starlib::graphics_api& api, shader_stage& out_shader_stage);

    ///Determines memory layout for a buffer given the API-specific rules on buffer layouts.
    ///Only 32 bit integer types, 32 bit floating point types, and composite types made of those are guarenteed to be supported.
    ///Support for other types is dependent on the api, and attempting to use types an API does not support is undefined behaviour.
    [[nodiscard]] starlib::status determine_shader_buffer_layout(const shader_stage& program, const std::string_view& buffer_name, memory_layout_info& out_buffer_layout);

}
