#pragma once
#include <string_view>

#include "common.hpp"
#include "shaders.hpp"
#include "shader_parameter_value.hpp"
#include "starlib/types/polymorphic.hpp"
#include "starlib/types/starlib_stdint.hpp"

namespace stardraw
{
    using namespace starlib_stdint;
    enum class command_type : u8
    {
        DRAW, DRAW_INDIRECT, DRAW_INDEXED, DRAW_INDEXED_INDIRECT,
        CONFIG_BLENDING, CONFIG_STENCIL, CONFIG_SCISSOR, CONFIG_FACE_CULL, CONFIG_DEPTH_TEST, CONFIG_DEPTH_RANGE, CONFIG_DRAW,
        CONFIG_VIEWPORTS,
        BUFFER_COPY, TEXTURE_COPY,
        CLEAR_WINDOW, CLEAR_BUFFER,
        CONFIG_SHADER, COMPUTE_DISPATCH, COMPUTE_DISPATCH_INDIRECT,
        SIGNAL,
        PRESENT,
    };

    struct command
    {
        virtual ~command() = default;
        [[nodiscard]] virtual command_type type() const = 0;
    };

    typedef std::vector<starlib::polymorphic<command>> command_list;

    enum class draw_mode : u8
    {
        TRIANGLES, TRIANGLE_STRIP, TRIANGLE_FAN,
    };

    enum class draw_indexed_index_type : u8
    {
        UINT_32, UINT_16, UINT_8
    };

    ///Draws some triangles! A valid draw specification must have been configured to provide the shader and vertex data
    ///Does *not* use index data. See draw_indexed_command for that.
    struct draw_command final : command
    {
        draw_command(const draw_mode mode, const u32 vertex_count, const u32 start_vertex = 0, const u32 instance_count = 1, const u32 start_instance = 0) : mode(mode), count(vertex_count), start_vertex(start_vertex), instances(instance_count), start_instance(start_instance) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::DRAW;
        }

        draw_mode mode;
        u32 count;

        ///Starting index for vertices
        u32 start_vertex;

        u32 instances;
        ///Starting index for instanced attributes
        u32 start_instance = 0;
    };

    ///Draws some triangles! A valid draw specification must have been configured to provide the shader and vertex/index data
    ///*Requires* index data. For non-indexed drawing, see draw_command.
    struct draw_indexed_command final : command
    {
        draw_indexed_command(const draw_mode mode, const u32 count, const i32 vertex_index_offset = 0, const u32 start_index = 0, const u32 instances = 1, const u32 start_instance = 0, const draw_indexed_index_type index_type = draw_indexed_index_type::UINT_32) : mode(mode), index_type(index_type), count(count), vertex_index_offset(vertex_index_offset), start_index(start_index), instances(instances), start_instance(start_instance) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::DRAW_INDEXED;
        }

        draw_mode mode;
        draw_indexed_index_type index_type;
        u32 count;

        ///Offset applied to all vertex indices
        i32 vertex_index_offset;

        ///Starting index for indices
        u32 start_index;

        u32 instances;
        ///Starting index for instanced attributes
        u32 start_instance;
    };

    ///Draws some triangles, using an indirect draw command buffer. A valid draw specification must have been configured to provide the shader and vertex data
    ///Does *not* use index data. See draw_indexed_indirect_command for that.
    struct draw_indirect_command final : command
    {
        draw_indirect_command(const std::string_view& indirect_buffer, const draw_mode mode, const u32 draw_count, const u32 indirect_index = 0) : indirect_buffer(indirect_buffer), mode(mode), draw_count(draw_count), indirect_index(indirect_index) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::DRAW_INDIRECT;
        }

        object_identifier indirect_buffer;
        draw_mode mode;
        u32 draw_count;
        u32 indirect_index;
    };

    ///Draws some triangles, using an indirect draw command buffer. A valid draw specification must have been configured to provide the shader and vertex data
    ///*Requires* index data. For non-indexed drawing, see draw_indirect_command.
    struct draw_indexed_indirect_command final : command
    {
        draw_indexed_indirect_command(const std::string_view& indirect_buffer, const draw_mode mode, const u32 draw_count, const u32 indirect_index = 0, const draw_indexed_index_type index_type = draw_indexed_index_type::UINT_32) : indirect_buffer(indirect_buffer), mode(mode), index_type(index_type), draw_count(draw_count), indirect_index(indirect_index) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::DRAW_INDIRECT;
        }

        object_identifier indirect_buffer;
        draw_mode mode;
        draw_indexed_index_type index_type;
        u32 draw_count;
        u32 indirect_index;
    };

    ///Sets the active draw specification. A valid draw specification must be set prior to calling any draw commands.
    struct draw_config_command final : command
    {
        explicit draw_config_command(const std::string& draw_specification) : draw_specification(draw_specification) {}
        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_DRAW;
        }

        object_identifier draw_specification;
    };

    enum class stencil_test_func : u8
    {
        ALWAYS, NEVER, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL, EQUAL, NOT_EQUAL
    };

    enum class stencil_result_op : u8
    {
        KEEP, ZERO, REPLACE, INCREMENT, INCREMENT_WRAP, DECREMENT, DECREMENT_WRAP, INVERT
    };

    enum class stencil_facing : u8
    {
        FRONT, BACK, BOTH
    };

    struct stencil_config
    {
        stencil_test_func test_func = stencil_test_func::ALWAYS;

        stencil_result_op stencil_fail_op = stencil_result_op::KEEP;
        stencil_result_op depth_fail_op = stencil_result_op::KEEP;
        stencil_result_op pixel_pass_op = stencil_result_op::KEEP;

        int reference = 0;
        int test_mask = std::numeric_limits<int>::max();
        int write_mask = std::numeric_limits<int>::max();

        bool enabled = true;
    };

    namespace stencil_configs
    {
        constexpr stencil_config DISABLED = {.enabled = false };
    }

    ///Configures the active stencil test state
    struct stencil_config_command final : command
    {
        explicit stencil_config_command(const stencil_config& config, const stencil_facing faces = stencil_facing::BOTH) : config(config), for_facing(faces) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_STENCIL;
        }

        stencil_config config;
        stencil_facing for_facing;
    };

    enum class blending_func : u8
    {
        ADD, SUBTRACT, REVERSE_SUBTRACT, MIN, MAX
    };

    enum class blending_factor : u8
    {
        ZERO,
        ONE,

        CONSTANT_COLOR,
        CONSTANT_ALPHA,

        ONE_MINUS_CONSTANT_COLOR,
        ONE_MINUS_CONSTANT_ALPHA,

        SOURCE_COLOR,
        DEST_COLOR,

        ONE_MINUS_SOURCE_COLOR,
        ONE_MINUS_DEST_COLOR,
        SOURCE_ALPHA,
        DEST_ALPHA,

        ONE_MINUS_SOURCE_ALPHA,
        ONE_MINUS_DEST_ALPHA,

        SOURCE_ALPHA_SATURATE,

        SECONDARY_SOURCE_COLOR,
        SECONDARY_SOURCE_ALPHA,
    };

    struct blending_config
    {
        blending_factor source_blend_rgb = blending_factor::SOURCE_ALPHA;
        blending_factor dest_blend_rgb = blending_factor::ONE_MINUS_SOURCE_ALPHA;
        blending_func rgb_equation = blending_func::ADD;

        blending_factor source_blend_alpha = blending_factor::SOURCE_ALPHA;
        blending_factor dest_blend_alpha = blending_factor::ONE_MINUS_SOURCE_ALPHA;
        blending_func alpha_equation = blending_func::ADD;

        f32 constant_blend_r = 1.0f;
        f32 constant_blend_g = 1.0f;
        f32 constant_blend_b = 1.0f;
        f32 constant_blend_a = 1.0f;

        bool enabled = true;
    };

    namespace blending_configs
    {
        constexpr blending_config DISABLED = {.enabled = false};
        constexpr blending_config ALPHA = {};
        constexpr blending_config OVERWRITE = {blending_factor::ONE, blending_factor::ZERO, blending_func::ADD, blending_factor::ONE, blending_factor::ZERO};
        constexpr blending_config ADDITIVE = {blending_factor::ONE, blending_factor::ONE};
        constexpr blending_config SUBTRACTIVE = {blending_factor::ONE, blending_factor::ONE, blending_func::REVERSE_SUBTRACT};
        constexpr blending_config MULTIPLY = {blending_factor::DEST_COLOR, blending_factor::ZERO};
        constexpr blending_config DARKEN = {blending_factor::ONE, blending_factor::ONE, blending_func::MIN};
        constexpr blending_config LIGHTEN = {blending_factor::ONE, blending_factor::ONE, blending_func::MAX};
    }

    ///Configures the active blending state
    struct blending_config_command final : command
    {
        explicit blending_config_command(const blending_config& config, const u32 draw_buffer_index = 0) : config(config), draw_buffer_index(draw_buffer_index) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_BLENDING;
        }

        blending_config config;
        u32 draw_buffer_index;
    };

    enum class depth_test_func : u8
    {
        ALWAYS, NEVER, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL, EQUAL, NOT_EQUAL
    };

    struct depth_test_config
    {
        depth_test_func test_func = depth_test_func::LESS;
        bool enable_depth_write = true;
        bool enabled = true;
    };

    namespace depth_test_configs
    {
        constexpr depth_test_config DISABLED = {.enabled = false};
        constexpr depth_test_config NORMAL = {};
        constexpr depth_test_config NORMAL_NO_WRITE = {.enable_depth_write = false};
        constexpr depth_test_config WRITE_UNCONDITIONALLY = {depth_test_func::ALWAYS};
    }

    ///Configures the active depth test state
    struct depth_test_config_command final : command
    {
        explicit depth_test_config_command(const depth_test_config& config) : config(config) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_DEPTH_TEST;
        }

        depth_test_config config;
    };

    ///Configures the active depth range
    struct depth_range_config_command final : command
    {
        explicit depth_range_config_command(const f64 near, const f64 far, const u32 viewport_index = 0) : near(near), far(far), viewport_index(viewport_index) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_DEPTH_RANGE;
        }

        f64 near;
        f64 far;
        u32 viewport_index;
    };

    enum face_cull_mode
    {
        DISABLED, BACK, FRONT, BOTH
    };

    ///Configures the active face culling
    struct face_cull_config_command final : command
    {
        explicit face_cull_config_command(const face_cull_mode& mode) : mode(mode) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_FACE_CULL;
        }

        face_cull_mode mode;
    };

    struct scissor_test_config
    {
        i32 left = std::numeric_limits<i32>::min();
        i32 bottom = std::numeric_limits<i32>::min();

        i32 width = std::numeric_limits<i32>::max();
        i32 height = std::numeric_limits<i32>::max();

        bool enabled = true;
    };

    namespace scissor_test_configs
    {
        constexpr scissor_test_config DISABLED = {.enabled = false };
    }

    ///Configures the active scissor test state
    struct scissor_config_command final : command
    {
        explicit scissor_config_command(const scissor_test_config& config, const u32 viewport_index = 0) : config(config), viewport_index(viewport_index) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_SCISSOR;
        }

        scissor_test_config config;
        u32 viewport_index;
    };

    ///Copies raw data between two buffers
    struct buffer_copy_command final : command
    {
        explicit buffer_copy_command(const std::string_view& source_buffer, const std::string_view& dest_buffer, const u64 from_address, const u64 to_address, const u64 bytes) : read_buffer(source_buffer), write_buffer(dest_buffer), source_address(from_address), dest_address(to_address), bytes(bytes) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::BUFFER_COPY;
        }

        object_identifier read_buffer;
        object_identifier write_buffer;
        u64 source_address;
        u64 dest_address;
        u64 bytes;
    };

    enum class clear_window_mode
    {
        COLOR, DEPTH, STENCIL,
        COLOR_AND_DEPTH, COLOR_AND_STENCIL, DEPTH_AND_STENCIL,
        ALL
    };

    struct clear_values_config
    {
        f32 color_r = 0.0f;
        f32 color_g = 0.0f;
        f32 color_b = 0.0f;
        f32 color_a = 1.0f;

        f64 depth = 1.0f;

        i32 stencil = 0;
    };

    namespace clear_values_configs
    {
        constexpr clear_values_config DEFAULT = {};
    }

    ///Clears the window/framebuffer to the given values. This command is NOT for clearing framebuffers.
    struct clear_window_command final : command
    {
        explicit clear_window_command(const clear_window_mode mode, const clear_values_config& config = clear_values_configs::DEFAULT) : mode(mode), config(config) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CLEAR_WINDOW;
        }

        clear_window_mode mode;
        clear_values_config config;
    };

    struct shader_parameter
    {
        shader_parameter_location location;
        shader_parameter_value value;
        bool operator==(const shader_parameter& parameter) const = default;
    };

    ///Updates shader parameter data
    ///Note: shader parameter data *overwrites* the relevant data in the buffers bound to the shader.
    struct shader_config_command final : command
    {
        explicit shader_config_command(const std::string_view& shader, const std::vector<shader_parameter>& parameters, const bool erase_previous = false) : shader(shader), parameters(parameters), erase_previous(erase_previous) {}
        explicit shader_config_command(const std::string_view& shader, const std::initializer_list<shader_parameter> parameters, const bool erase_previous = false) : shader(shader), parameters(parameters), erase_previous(erase_previous) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_SHADER;
        }

        object_identifier shader;
        std::vector<shader_parameter> parameters;
        bool erase_previous;
    };

    ///Set a fence ('signal') that can be checked via the render context to determine when previous commands are finished.
    struct signal_command final : command
    {
        explicit signal_command(const std::string_view& signal_name) : signal_name(signal_name) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::SIGNAL;
        }

        std::string signal_name;
    };

    struct texture_copy_info
    {
        u32 read_x;
        u32 read_y;
        u32 read_z;

        u32 read_mipmap_level = 0;
        u32 read_layer = 0;

        u32 write_x;
        u32 write_y;
        u32 write_z;

        u32 write_mipmap_level = 0;
        u32 write_layer = 0;

        u32 copy_width = 1;
        u32 copy_height = 1;
        u32 copy_depth = 1;
        u32 copy_layers = 1;
    };

    ///Copies data between textures. Type conversion is NOT performed, raw values are transferred directly.
    struct texture_copy_command final : command
    {
        texture_copy_command(const std::string_view& read_texture, const std::string_view& write_texture, const texture_copy_info& copy_info) : read_texture(read_texture), write_texture(write_texture), copy_info(copy_info) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::TEXTURE_COPY;
        }

        object_identifier read_texture;
        object_identifier write_texture;
        texture_copy_info copy_info;
    };

    ///Marks the end of the frame.
    ///It is undefined behaviour if you do not call this at the end of your frame rendering.
    struct present_command final : command
    {
        [[nodiscard]] command_type type() const override
        {
            return command_type::PRESENT;
        }
    };

    ///Dispatches a compute shader with the given group numbers
    struct compute_dispatch_command final : command
    {
        compute_dispatch_command(const std::string_view& shader, const u32 groups_x, const u32 groups_y, const u32 groups_z) : groups_x(groups_x), groups_y(groups_y), groups_z(groups_z), shader(shader) {}
        [[nodiscard]] command_type type() const override
        {
            return command_type::COMPUTE_DISPATCH;
        }

        u32 groups_x;
        u32 groups_y;
        u32 groups_z;
        object_identifier shader;
    };

    ///Dispatches a compute shader multiple times indirectly
    struct compute_dispatch_indirect_command final : command
    {
        compute_dispatch_indirect_command(const std::string_view& shader, const std::string_view& indirect_buffer, const u32 indirect_index = 0) : shader(shader), indirect_buffer(indirect_buffer), indirect_index(indirect_index) {}
        [[nodiscard]] command_type type() const override
        {
            return command_type::COMPUTE_DISPATCH_INDIRECT;
        }

        object_identifier shader;
        object_identifier indirect_buffer;
        u32 indirect_index;
    };

    struct viewport_config
    {
        float x;
        float y;
        float width;
        float height;
    };

    ///Configures the active drawing viewports
    struct configure_viewports_command final : command
    {
        explicit configure_viewports_command(const std::initializer_list<viewport_config> viewports, const u32 first_viewport_index = 0) : first_viewport_index(first_viewport_index), viewports(viewports) {}
        explicit configure_viewports_command(const std::vector<viewport_config>& viewports, const u32 first_viewport_index = 0) : first_viewport_index(first_viewport_index), viewports(viewports) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_VIEWPORTS;
        }

        u32 first_viewport_index;
        std::vector<viewport_config> viewports;
    };
}
