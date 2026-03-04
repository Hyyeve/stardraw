#pragma once
#include <string_view>

#include "shaders.hpp"
#include "shader_parameter_value.hpp"
#include "stardraw/api/types.hpp"
#include "starlib/types/polymorphic.hpp"
#include "starlib/types/starlib_stdint.hpp"

namespace stardraw
{
    using namespace starlib_stdint;
    enum class command_type : u8
    {
        DRAW, DRAW_INDIRECT, DRAW_INDEXED, DRAW_INDEXED_INDIRECT,
        CONFIG_BLENDING, CONFIG_STENCIL, CONFIG_SCISSOR, CONFIG_FACE_CULL, CONFIG_DEPTH_TEST, CONFIG_DEPTH_RANGE, CONFIG_DRAW,
        BUFFER_COPY, TEXTURE_COPY,
        CLEAR_WINDOW, CLEAR_BUFFER,
        CONFIG_SHADER,
        SIGNAL,
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

    struct draw_command final : command
    {
        draw_command(const draw_mode mode, const u32 count, const u32 start_vertex = 0, const u32 instances = 1, const u32 start_instance = 0) : mode(mode), count(count), start_vertex(start_vertex), instances(instances), start_instance(start_instance) {}

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

    struct draw_indirect_command final : command
    {
        draw_indirect_command(const draw_mode mode, const u32 draw_count, const u32 indirect_source_offset = 0) : mode(mode), draw_count(draw_count), indirect_offset(indirect_source_offset) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::DRAW_INDIRECT;
        }

        draw_mode mode;
        u32 draw_count;
        u32 indirect_offset;
    };

    struct draw_indexed_indirect_command final : command
    {
        draw_indexed_indirect_command(const draw_mode mode, const u32 draw_count, const u32 indirect_source_offset = 0, const draw_indexed_index_type index_type = draw_indexed_index_type::UINT_32) : mode(mode), index_type(index_type), draw_count(draw_count), indirect_offset(indirect_source_offset) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::DRAW_INDIRECT;
        }

        draw_mode mode;
        draw_indexed_index_type index_type;
        u32 draw_count;
        u32 indirect_offset;
    };

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

    struct depth_test_config_command final : command
    {
        explicit depth_test_config_command(const depth_test_config& config) : config(config) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_DEPTH_TEST;
        }

        depth_test_config config;
    };

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

    struct buffer_copy_command final : command
    {
        explicit buffer_copy_command(const std::string_view& source_buffer, const std::string_view& dest_buffer, const u64 from_address, const u64 to_address, const u64 bytes) : source_buffer(source_buffer), dest_buffer(dest_buffer), source_address(from_address), dest_address(to_address), bytes(bytes) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::BUFFER_COPY;
        }

        object_identifier source_buffer;
        object_identifier dest_buffer;
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

    struct signal_command final : command
    {
        explicit signal_command(const std::string_view& signal_name) : signal_name(signal_name) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::SIGNAL;
        }

        std::string signal_name;
    };
}
