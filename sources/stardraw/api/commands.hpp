#pragma once
#include <string_view>

#include "stardraw/api/polymorphic_ptr.hpp"
#include "stardraw/api/types.hpp"

namespace stardraw
{
    enum class command_type : uint8_t
    {
        DRAW, DRAW_INDIRECT, DRAW_INDEXED, DRAW_INDEXED_INDIRECT,
        CONFIG_BLENDING, CONFIG_STENCIL, CONFIG_SCISSOR, CONFIG_FACE_CULL, CONFIG_DEPTH_TEST, CONFIG_DEPTH_RANGE,
        BUFFER_UPLOAD, BUFFER_COPY, BUFFER_DOWNLOAD,
        CLEAR_WINDOW, CLEAR_BUFFER
    };

    struct command
    {
        virtual ~command() = default;
        [[nodiscard]] virtual command_type type() const = 0;
    };

    typedef std::vector<polymorphic_ptr<command>> command_list;

    enum class draw_mode : uint8_t
    {
        TRIANGLES, TRIANGLE_STRIP, TRIANGLE_FAN,
    };

    enum class draw_indexed_index_type : uint8_t
    {
        UINT_32, UINT_16, UINT_8
    };

    struct draw_command final : command
    {
        draw_command(const std::string_view& draw_specification, const draw_mode mode, const uint32_t count, const uint32_t start_vertex = 0, const uint32_t instances = 1, const uint32_t start_instance = 0) : draw_specification_identifier(draw_specification), mode(mode), count(count), start_vertex(start_vertex), instances(instances), start_instance(start_instance) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::DRAW;
        }

        object_identifier draw_specification_identifier;
        draw_mode mode;
        uint32_t count;

        ///Starting index for vertices
        uint32_t start_vertex;

        uint32_t instances;
        ///Starting index for instanced attributes
        uint32_t start_instance = 0;
    };

    struct draw_indexed_command final : command
    {
        draw_indexed_command(const std::string_view& draw_specification, const draw_mode mode, const uint32_t count, const int32_t vertex_index_offset = 0, const uint32_t start_index = 0, const uint32_t instances = 1, const uint32_t start_instance = 0, const draw_indexed_index_type index_type = draw_indexed_index_type::UINT_32) : draw_specification_identifier(draw_specification), mode(mode), index_type(index_type), count(count), vertex_index_offset(vertex_index_offset), start_index(start_index), instances(instances), start_instance(start_instance) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::DRAW_INDEXED;
        }

        object_identifier draw_specification_identifier;

        draw_mode mode;
        draw_indexed_index_type index_type;
        uint32_t count;

        ///Offset applied to all vertex indices
        int32_t vertex_index_offset;

        ///Starting index for indices
        uint32_t start_index;

        uint32_t instances;
        ///Starting index for instanced attributes
        uint32_t start_instance;
    };

    struct draw_indirect_command final : command
    {
        draw_indirect_command(const std::string_view& draw_specification, const draw_mode mode, const uint32_t draw_count, const uint32_t indirect_source_offset = 0) : draw_specification_identifier(draw_specification), mode(mode), draw_count(draw_count), indirect_offset(indirect_source_offset) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::DRAW_INDIRECT;
        }

        object_identifier draw_specification_identifier;

        draw_mode mode;
        uint32_t draw_count;
        uint32_t indirect_offset;
    };

    struct draw_indexed_indirect_command final : command
    {
        draw_indexed_indirect_command(const std::string_view& draw_specification, const draw_mode mode, const uint32_t draw_count, const uint32_t indirect_source_offset = 0, const draw_indexed_index_type index_type = draw_indexed_index_type::UINT_32) : draw_specification_identifier(draw_specification), mode(mode), index_type(index_type), draw_count(draw_count), indirect_offset(indirect_source_offset) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::DRAW_INDIRECT;
        }

        object_identifier draw_specification_identifier;

        draw_mode mode;
        draw_indexed_index_type index_type;
        uint32_t draw_count;
        uint32_t indirect_offset;
    };

    enum class stencil_test_func : uint8_t
    {
        ALWAYS, NEVER, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL, EQUAL, NOT_EQUAL
    };

    enum class stencil_result_op : uint8_t
    {
        KEEP, ZERO, REPLACE, INCREMENT, INCREMENT_WRAP, DECREMENT, DECREMENT_WRAP, INVERT
    };

    enum class stencil_facing : uint8_t
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

    struct config_stencil_command final : command
    {
        explicit config_stencil_command(const stencil_config& config, const stencil_facing faces = stencil_facing::BOTH) : config(config), for_facing(faces) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_STENCIL;
        }

        stencil_config config;
        stencil_facing for_facing;
    };

    enum class blending_func : uint8_t
    {
        ADD, SUBTRACT, REVERSE_SUBTRACT, MIN, MAX
    };

    enum class blending_factor : uint8_t
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

        float constant_blend_r = 1.0f;
        float constant_blend_g = 1.0f;
        float constant_blend_b = 1.0f;
        float constant_blend_a = 1.0f;

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

    struct config_blending_command final : command
    {
        explicit config_blending_command(const blending_config& config, const uint32_t draw_buffer_index = 0) : config(config), draw_buffer_index(draw_buffer_index) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_BLENDING;
        }

        blending_config config;
        uint32_t draw_buffer_index;
    };

    enum class depth_test_func : uint8_t
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

    struct config_depth_test_command final : command
    {
        explicit config_depth_test_command(const depth_test_config& config) : config(config) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_DEPTH_TEST;
        }

        depth_test_config config;
    };

    struct config_depth_range_command final : command
    {
        explicit config_depth_range_command(const double near, const double far, const uint32_t viewport_index = 0) : near(near), far(far), viewport_index(viewport_index) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_DEPTH_RANGE;
        }

        double near;
        double far;
        uint32_t viewport_index;
    };

    enum face_cull_mode
    {
        DISABLED, BACK, FRONT, BOTH
    };

    struct config_face_cull_command final : command
    {
        explicit config_face_cull_command(const face_cull_mode& mode) : mode(mode) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_FACE_CULL;
        }

        face_cull_mode mode;
    };

    struct scissor_test_config
    {
        int32_t left = std::numeric_limits<int32_t>::min();
        int32_t bottom = std::numeric_limits<int32_t>::min();

        int32_t width = std::numeric_limits<int32_t>::max();
        int32_t height = std::numeric_limits<int32_t>::max();

        bool enabled = true;
    };

    namespace scissor_test_configs
    {
        constexpr scissor_test_config DISABLED = {.enabled = false };
    }

    struct config_scissor_command final : command
    {
        explicit config_scissor_command(const scissor_test_config& config, const uint32_t viewport_index = 0) : config(config), viewport_index(viewport_index) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_SCISSOR;
        }

        scissor_test_config config;
        uint32_t viewport_index;
    };

    ///UNSAFE_DIRECT: Fastest, least memory cost, but can overwrite in-use data (does no syncing check)
    ///SAFE_STREAMING: Slower, may increase buffer memory use (up to 2x base size max)
    ///SAFE_ONE_TIME: Slowest, allocates temporary copy buffer for transferring
    enum class buffer_upload_type : uint8_t
    {
        UNSAFE_DIRECT, SAFE_STREAMING, SAFE_ONE_TIME
    };

    struct buffer_upload_command final : command
    {
        explicit buffer_upload_command(const std::string_view& buffer, const uint64_t address, const uint64_t bytes, const void* const data, const buffer_upload_type upload_type = buffer_upload_type::SAFE_ONE_TIME) : buffer_identifier(buffer), upload_address(address), upload_bytes(bytes), upload_data(data), upload_type(upload_type) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::BUFFER_UPLOAD;
        }

        object_identifier buffer_identifier;
        uint64_t upload_address;
        uint64_t upload_bytes;
        const void* const upload_data;
        buffer_upload_type upload_type;
    };

    struct buffer_copy_command final : command
    {
        explicit buffer_copy_command(const std::string_view& source_buffer, const std::string_view& dest_buffer, const uint64_t from_address, const uint64_t to_address, const uint64_t bytes) : source_identifier(source_buffer), dest_identifier(dest_buffer), source_address(from_address), dest_address(to_address), bytes(bytes) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::BUFFER_COPY;
        }

        object_identifier source_identifier;
        object_identifier dest_identifier;
        uint64_t source_address;
        uint64_t dest_address;
        uint64_t bytes;
    };

    enum class clear_window_mode
    {
        COLOR, DEPTH, STENCIL,
        COLOR_AND_DEPTH, COLOR_AND_STENCIL, DEPTH_AND_STENCIL,
        ALL
    };

    struct clear_values_config
    {
        float color_r = 0.0f;
        float color_g = 0.0f;
        float color_b = 0.0f;
        float color_a = 1.0f;

        double depth = 1.0f;

        int32_t stencil = 0;
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

}
