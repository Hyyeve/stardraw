#pragma once
#include <bit>
#include <optional>
#include <string_view>

#include "common.hpp"
#include "shaders.hpp"
#include "shader_parameter_value.hpp"
#include "starlib/types/polymorphic.hpp"
#include "starlib/types/starlib_stdint.hpp"

namespace stardraw
{
    enum class command_type : starlib_stdint::u8
    {
        DRAW, DRAW_INDIRECT, DRAW_INDEXED, DRAW_INDEXED_INDIRECT,
        CONFIG_BLENDING, CONFIG_STENCIL, CONFIG_SCISSOR, CONFIG_FACE_CULL, CONFIG_DEPTH_TEST, CONFIG_DEPTH_RANGE, CONFIG_DRAW, CONFIG_VIEWPORTS,
        BUFFER_COPY, TEXTURE_COPY, FRAMEBUFFER_COPY,
        CLEAR_WINDOW, CLEAR_FRAMEBUFFER, CLEAR_TEXTURE,
        CONFIG_SHADER, COMPUTE_DISPATCH, COMPUTE_DISPATCH_INDIRECT,
        SIGNAL,
        AQUIRE, PRESENT,
    };

    struct command
    {
        virtual ~command() = default;
        [[nodiscard]] virtual command_type type() const = 0;
    };

    typedef std::vector<starlib::polymorphic<command>> command_list;

    enum class draw_mode : starlib_stdint::u8
    {
        TRIANGLES, TRIANGLE_STRIP, TRIANGLE_FAN,
    };

    enum class draw_indexed_index_type : starlib_stdint::u8
    {
        UINT_32, UINT_16, UINT_8
    };

    ///Draws some triangles! A valid draw specification must have been configured to provide the shader and vertex data
    ///Does *not* use index data. See draw_indexed_command for that.
    struct draw final : command
    {
        draw(const draw_mode mode, const starlib_stdint::u32 vertex_count, const starlib_stdint::u32 start_vertex = 0, const starlib_stdint::u32 instance_count = 1, const starlib_stdint::u32 start_instance = 0) : mode(mode), count(vertex_count), start_vertex(start_vertex), instances(instance_count), start_instance(start_instance) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::DRAW;
        }

        draw_mode mode;
        starlib_stdint::u32 count;

        ///Starting index for vertices
        starlib_stdint::u32 start_vertex;

        starlib_stdint::u32 instances;
        ///Starting index for instanced attributes
        starlib_stdint::u32 start_instance = 0;
    };

    ///Draws some triangles! A valid draw specification must have been configured to provide the shader and vertex/index data
    ///*Requires* index data. For non-indexed drawing, see draw_command.
    struct draw_indexed final : command
    {
        draw_indexed(const draw_mode mode, const starlib_stdint::u32 count, const starlib_stdint::i32 vertex_index_offset = 0, const starlib_stdint::u32 start_index = 0, const starlib_stdint::u32 instances = 1, const starlib_stdint::u32 start_instance = 0, const draw_indexed_index_type index_type = draw_indexed_index_type::UINT_32) : mode(mode), index_type(index_type), count(count), vertex_index_offset(vertex_index_offset), start_index(start_index), instances(instances), start_instance(start_instance) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::DRAW_INDEXED;
        }

        draw_mode mode;
        draw_indexed_index_type index_type;
        starlib_stdint::u32 count;

        ///Offset applied to all vertex indices
        starlib_stdint::i32 vertex_index_offset;

        ///Starting index for indices
        starlib_stdint::u32 start_index;

        starlib_stdint::u32 instances;
        ///Starting index for instanced attributes
        starlib_stdint::u32 start_instance;
    };

    ///Draws some triangles, using an indirect draw command buffer. A valid draw specification must have been configured to provide the shader and vertex data
    ///Does *not* use index data. See draw_indexed_indirect_command for that.
    struct draw_indirect final : command
    {
        draw_indirect(const std::string_view& indirect_buffer, const draw_mode mode, const starlib_stdint::u32 draw_count, const starlib_stdint::u32 indirect_index = 0) : indirect_buffer(indirect_buffer), mode(mode), draw_count(draw_count), indirect_index(indirect_index) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::DRAW_INDIRECT;
        }

        object_identifier indirect_buffer;
        draw_mode mode;
        starlib_stdint::u32 draw_count;
        starlib_stdint::u32 indirect_index;
    };

    ///Draws some triangles, using an indirect draw command buffer. A valid draw specification must have been configured to provide the shader and vertex data
    ///*Requires* index data. For non-indexed drawing, see draw_indirect_command.
    struct draw_indexed_indirect final : command
    {
        draw_indexed_indirect(const std::string_view& indirect_buffer, const draw_mode mode, const starlib_stdint::u32 draw_count, const starlib_stdint::u32 indirect_index = 0, const draw_indexed_index_type index_type = draw_indexed_index_type::UINT_32) : indirect_buffer(indirect_buffer), mode(mode), index_type(index_type), draw_count(draw_count), indirect_index(indirect_index) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::DRAW_INDIRECT;
        }

        object_identifier indirect_buffer;
        draw_mode mode;
        draw_indexed_index_type index_type;
        starlib_stdint::u32 draw_count;
        starlib_stdint::u32 indirect_index;
    };

    ///Sets the active draw specification. A valid draw specification must be set prior to calling any draw commands.
    struct configure_draw final : command
    {
        explicit configure_draw(const std::string& draw_specification) : draw_specification(draw_specification) {}
        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_DRAW;
        }

        object_identifier draw_specification;
    };

    enum class stencil_test_func : starlib_stdint::u8
    {
        ALWAYS, NEVER, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL, EQUAL, NOT_EQUAL
    };

    enum class stencil_result_op : starlib_stdint::u8
    {
        KEEP, ZERO, REPLACE, INCREMENT, INCREMENT_WRAP, DECREMENT, DECREMENT_WRAP, INVERT
    };

    enum class stencil_facing : starlib_stdint::u8
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
    struct configure_stencil final : command
    {
        explicit configure_stencil(const stencil_config& config, const stencil_facing faces = stencil_facing::BOTH) : config(config), for_facing(faces) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_STENCIL;
        }

        stencil_config config;
        stencil_facing for_facing;
    };

    enum class blending_func : starlib_stdint::u8
    {
        ADD, SUBTRACT, REVERSE_SUBTRACT, MIN, MAX
    };

    enum class blending_factor : starlib_stdint::u8
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

        starlib_stdint::f32 constant_blend_r = 1.0f;
        starlib_stdint::f32 constant_blend_g = 1.0f;
        starlib_stdint::f32 constant_blend_b = 1.0f;
        starlib_stdint::f32 constant_blend_a = 1.0f;

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
    struct configure_blending final : command
    {
        explicit configure_blending(const blending_config& config, const starlib_stdint::u32 draw_buffer_index = 0) : config(config), draw_buffer_index(draw_buffer_index) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_BLENDING;
        }

        blending_config config;
        starlib_stdint::u32 draw_buffer_index;
    };

    enum class depth_test_func : starlib_stdint::u8
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
    struct configure_depth_test final : command
    {
        explicit configure_depth_test(const depth_test_config& config) : config(config) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_DEPTH_TEST;
        }

        depth_test_config config;
    };

    ///Configures the active depth range
    struct configure_depth_range final : command
    {
        explicit configure_depth_range(const starlib_stdint::f64 near, const starlib_stdint::f64 far, const starlib_stdint::u32 viewport_index = 0) : near(near), far(far), viewport_index(viewport_index) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_DEPTH_RANGE;
        }

        starlib_stdint::f64 near;
        starlib_stdint::f64 far;
        starlib_stdint::u32 viewport_index;
    };

    enum face_cull_mode
    {
        DISABLED, BACK, FRONT, BOTH
    };

    ///Configures the active face culling
    struct configure_face_cull final : command
    {
        explicit configure_face_cull(const face_cull_mode& mode) : mode(mode) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_FACE_CULL;
        }

        face_cull_mode mode;
    };

    struct scissor_test_config
    {
        starlib_stdint::i32 left = std::numeric_limits<starlib_stdint::i32>::min();
        starlib_stdint::i32 bottom = std::numeric_limits<starlib_stdint::i32>::min();

        starlib_stdint::i32 width = std::numeric_limits<starlib_stdint::i32>::max();
        starlib_stdint::i32 height = std::numeric_limits<starlib_stdint::i32>::max();

        bool enabled = true;
    };

    namespace scissor_test_configs
    {
        constexpr scissor_test_config DISABLED = {.enabled = false };
    }

    ///Configures the active scissor test state
    struct configure_scissor_test final : command
    {
        explicit configure_scissor_test(const scissor_test_config& config, const starlib_stdint::u32 viewport_index = 0) : config(config), viewport_index(viewport_index) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_SCISSOR;
        }

        scissor_test_config config;
        starlib_stdint::u32 viewport_index;
    };

    ///Copies raw data between two buffers
    struct buffer_copy final : command
    {
        explicit buffer_copy(const std::string_view& source_buffer, const std::string_view& dest_buffer, const starlib_stdint::u64 from_address, const starlib_stdint::u64 to_address, const starlib_stdint::u64 bytes) : read_buffer(source_buffer), write_buffer(dest_buffer), source_address(from_address), dest_address(to_address), bytes(bytes) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::BUFFER_COPY;
        }

        object_identifier read_buffer;
        object_identifier write_buffer;
        starlib_stdint::u64 source_address;
        starlib_stdint::u64 dest_address;
        starlib_stdint::u64 bytes;
    };

    ///Stores a set of 4 values for clearing RGBA channels.
    ///The values will be assumed to be in the correct format for the format of the buffer being cleared (float, int, or uint)
    struct clear_channel_values
    {
        constexpr clear_channel_values(const starlib_stdint::f32 r, const starlib_stdint::f32 g, const starlib_stdint::f32 b, const starlib_stdint::f32 a) : data({std::bit_cast<const starlib_stdint::u32>(r), std::bit_cast<const starlib_stdint::u32>(g), std::bit_cast<const starlib_stdint::u32>(b), std::bit_cast<const starlib_stdint::u32>(a)}) {}
        constexpr clear_channel_values(const starlib_stdint::i32 r, const starlib_stdint::i32 g, const starlib_stdint::i32 b, const starlib_stdint::i32 a) : data({std::bit_cast<const starlib_stdint::u32>(r), std::bit_cast<const starlib_stdint::u32>(g), std::bit_cast<const starlib_stdint::u32>(b), std::bit_cast<const starlib_stdint::u32>(a)}) {}
        constexpr clear_channel_values(const starlib_stdint::u32 r, const starlib_stdint::u32 g, const starlib_stdint::u32 b, const starlib_stdint::u32 a) : data({r, g, b, a}) {}
        std::array<starlib_stdint::u32, 4> data;

        constexpr starlib_stdint::f32 as_f32(const starlib_stdint::u32 idx) const
        {
            return std::bit_cast<const starlib_stdint::f32>(data[idx]);
        }

        constexpr starlib_stdint::i32 as_i32(const starlib_stdint::u32 idx) const
        {
            return std::bit_cast<const starlib_stdint::i32>(data[idx]);
        }

        constexpr starlib_stdint::i32 as_u32(const starlib_stdint::u32 idx) const
        {
            return std::bit_cast<const starlib_stdint::u32>(data[idx]);
        }
    };

    struct clear_values
    {
        clear_channel_values channels;
        starlib_stdint::f64 depth = 1.0f;
        starlib_stdint::u32 stencil = 0;
    };

    namespace clear_info_defaults
    {
        constexpr clear_values FLOAT_DEFAULT = {.channels = {0.0f, 0.0f, 0.0f, 0.0f}};
        constexpr clear_values INT_DEFAULT = {.channels = {0, 0, 0, 0}};
        constexpr clear_values UINT_DEFAULT = {.channels = {0u, 0u, 0u, 0u}};
    }

    enum class attachment_components
    {
        COLOR, DEPTH, STENCIL,
        COLOR_AND_DEPTH, COLOR_AND_STENCIL, DEPTH_AND_STENCIL,
        ALL
    };

    ///Clears the window buffer to the given values.
    ///Window buffers always use floating point clear values.
    struct clear_window final : command
    {
        explicit clear_window(const attachment_components clear_components = attachment_components::ALL, const clear_values& config = clear_info_defaults::FLOAT_DEFAULT) : clear_components(clear_components), config(config) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CLEAR_WINDOW;
        }

        attachment_components clear_components;
        clear_values config;
    };

    ///Clears a texture to the given values.
    struct clear_texture final : command
    {
        explicit clear_texture(const std::string_view& texture, const clear_values& clear_values = clear_info_defaults::FLOAT_DEFAULT, const starlib_stdint::u32 mipmap_level = 0) : texture(texture), mipmap_level(mipmap_level), clear_vlaues(clear_values) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CLEAR_TEXTURE;
        }

        object_identifier texture;
        starlib_stdint::u32 mipmap_level;
        clear_values clear_vlaues;
    };

    struct framebuffer_color_clear_info
    {
        starlib_stdint::u32 attachment_index = 0;
        clear_values values = clear_info_defaults::FLOAT_DEFAULT;
    };

    ///Clears some number of framebuffer attachments to the given values.
    struct clear_framebuffer final : command
    {
        explicit clear_framebuffer(const std::string_view& framebuffer, const std::initializer_list<framebuffer_color_clear_info> color_clears, const std::optional<starlib_stdint::f32>& depth_clear = std::nullopt, const std::optional<starlib_stdint::u32>& stencil_clear = std::nullopt) : framebuffer(framebuffer), color_clears(color_clears), depth_clear(depth_clear), stencil_clear(stencil_clear) {}
        explicit clear_framebuffer(const std::string_view& framebuffer, const std::vector<framebuffer_color_clear_info>& color_clears, const std::optional<starlib_stdint::f32>& depth_clear = std::nullopt, const std::optional<starlib_stdint::u32>& stencil_clear = std::nullopt) : framebuffer(framebuffer), color_clears(color_clears), depth_clear(depth_clear), stencil_clear(stencil_clear) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CLEAR_FRAMEBUFFER;
        }

        object_identifier framebuffer;
        std::vector<framebuffer_color_clear_info> color_clears;
        std::optional<starlib_stdint::f32> depth_clear = std::nullopt;
        std::optional<starlib_stdint::u32> stencil_clear = std::nullopt;
    };

    struct shader_parameter
    {
        shader_parameter_location location;
        shader_parameter_value value;
        bool operator==(const shader_parameter& parameter) const = default;
    };

    ///Updates shader parameter data
    ///Note: shader parameter data *overwrites* the relevant data in the buffers bound to the shader.
    struct configure_shader final : command
    {
        explicit configure_shader(const std::string_view& shader, const std::vector<shader_parameter>& parameters, const bool erase_previous = false) : shader(shader), parameters(parameters), erase_previous(erase_previous) {}
        explicit configure_shader(const std::string_view& shader, const std::initializer_list<shader_parameter> parameters, const bool erase_previous = false) : shader(shader), parameters(parameters), erase_previous(erase_previous) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_SHADER;
        }

        object_identifier shader;
        std::vector<shader_parameter> parameters;
        bool erase_previous;
    };

    ///Set a fence ('signal') that can be checked via the render context to determine when previous commands are finished.
    struct signal final : command
    {
        explicit signal(const std::string_view& signal_name) : signal_name(signal_name) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::SIGNAL;
        }

        std::string signal_name;
    };

    struct texture_copy_info
    {
        starlib_stdint::u32 read_x;
        starlib_stdint::u32 read_y;
        starlib_stdint::u32 read_z;

        starlib_stdint::u32 read_mipmap_level = 0;
        starlib_stdint::u32 read_layer = 0;

        starlib_stdint::u32 write_x;
        starlib_stdint::u32 write_y;
        starlib_stdint::u32 write_z;

        starlib_stdint::u32 write_mipmap_level = 0;
        starlib_stdint::u32 write_layer = 0;

        starlib_stdint::u32 copy_width = 1;
        starlib_stdint::u32 copy_height = 1;
        starlib_stdint::u32 copy_depth = 1;
        starlib_stdint::u32 copy_layers = 1;

        inline texture_copy_info create_simple_1d(const starlib_stdint::u32 x, const starlib_stdint::u32 width, const starlib_stdint::u32 mipmap_level = 0, const starlib_stdint::u32 layer = 0, const starlib_stdint::u32 layers = 1) const
        {
            return {x, 0, 0, mipmap_level, layer, x, 0, 0, mipmap_level, layer, width, 1, 1, layers};
        }

        inline texture_copy_info create_simple_2d(const starlib_stdint::u32 x, const starlib_stdint::u32 y, const starlib_stdint::u32 width, const starlib_stdint::u32 height, const starlib_stdint::u32 mipmap_level = 0, const starlib_stdint::u32 layer = 0, const starlib_stdint::u32 layers = 1) const
        {
            return {x, y, 0, mipmap_level, layer, x, y, 0, mipmap_level, layer, width, height, 1, layers};
        }

        inline texture_copy_info create_simple_3d(const starlib_stdint::u32 x, const starlib_stdint::u32 y, const starlib_stdint::u32 z, const starlib_stdint::u32 width, const starlib_stdint::u32 height, const starlib_stdint::u32 depth, const starlib_stdint::u32 mipmap_level = 0, const starlib_stdint::u32 layer = 0, const starlib_stdint::u32 layers = 1) const
        {
            return {x, y, z, mipmap_level, layer, x, y, z, mipmap_level, layer, width, height, depth, layers};
        }
    };

    ///Copies data between textures. Type conversion is NOT performed, raw values are transferred directly.
    struct texture_copy final : command
    {
        texture_copy(const std::string_view& read_texture, const std::string_view& write_texture, const texture_copy_info& copy_info) : read_texture(read_texture), write_texture(write_texture), copy_info(copy_info) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::TEXTURE_COPY;
        }

        object_identifier read_texture;
        object_identifier write_texture;
        texture_copy_info copy_info;
    };

    ///Marks the start of the frame.
    ///It is undefined behaviour if you do not call this at the end of your frame rendering.
    struct aquire final : command
    {
        [[nodiscard]] command_type type() const override
        {
            return command_type::AQUIRE;
        }
    };

    ///Marks the end of the frame.
    ///It is undefined behaviour if you do not call this at the end of your frame rendering.
    struct present final : command
    {
        [[nodiscard]] command_type type() const override
        {
            return command_type::PRESENT;
        }
    };

    ///Dispatches a compute shader with the given group numbers
    struct dispatch_compute final : command
    {
        dispatch_compute(const std::string_view& shader, const starlib_stdint::u32 groups_x, const starlib_stdint::u32 groups_y, const starlib_stdint::u32 groups_z) : groups_x(groups_x), groups_y(groups_y), groups_z(groups_z), shader(shader) {}
        [[nodiscard]] command_type type() const override
        {
            return command_type::COMPUTE_DISPATCH;
        }

        starlib_stdint::u32 groups_x;
        starlib_stdint::u32 groups_y;
        starlib_stdint::u32 groups_z;
        object_identifier shader;
    };

    ///Dispatches a compute shader multiple times indirectly
    struct dispatch_compute_indirect final : command
    {
        dispatch_compute_indirect(const std::string_view& shader, const std::string_view& indirect_buffer, const starlib_stdint::u32 indirect_index = 0) : shader(shader), indirect_buffer(indirect_buffer), indirect_index(indirect_index) {}
        [[nodiscard]] command_type type() const override
        {
            return command_type::COMPUTE_DISPATCH_INDIRECT;
        }

        object_identifier shader;
        object_identifier indirect_buffer;
        starlib_stdint::u32 indirect_index;
    };

    struct viewport_config
    {
        float x;
        float y;
        float width;
        float height;
    };

    ///Configures the active drawing viewports
    struct comnfigure_viewports final : command
    {
        explicit comnfigure_viewports(const std::initializer_list<viewport_config> viewports, const starlib_stdint::u32 first_viewport_index = 0) : first_viewport_index(first_viewport_index), viewports(viewports) {}
        explicit comnfigure_viewports(const std::vector<viewport_config>& viewports, const starlib_stdint::u32 first_viewport_index = 0) : first_viewport_index(first_viewport_index), viewports(viewports) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::CONFIG_VIEWPORTS;
        }

        starlib_stdint::u32 first_viewport_index;
        std::vector<viewport_config> viewports;
    };

    enum class framebuffer_copy_filtering
    {
        NEAREST, LINEAR
    };

    ///Defines the source and destination areas to copy between framebuffers,
    ///and the interpolation to use if the areas are not the same size.
    ///NOTE: It is invalid to use linear interpolation when copying depth/stencil components or a non-floating-point color component.
    struct framebuffer_copy_info
    {
        starlib_stdint::u32 read_x;
        starlib_stdint::u32 read_y;
        starlib_stdint::u32 read_width;
        starlib_stdint::u32 read_height;

        starlib_stdint::u32 write_x;
        starlib_stdint::u32 write_y;
        starlib_stdint::u32 write_width;
        starlib_stdint::u32 write_height;

        framebuffer_copy_filtering filtering = framebuffer_copy_filtering::NEAREST;
        attachment_components components = attachment_components::ALL;

        starlib_stdint::u32 read_color_attachment_index = 0;
        starlib_stdint::u32 write_color_attachment_index = 0;

        inline framebuffer_copy_info create_simple(const starlib_stdint::u32 x, const starlib_stdint::u32 y, const starlib_stdint::u32 width, const starlib_stdint::u32 height, const attachment_components copy_components = attachment_components::ALL, const starlib_stdint::u32 color_attachment_index = 0) const
        {
            return {x, y, width, height, x, y, width, height, framebuffer_copy_filtering::NEAREST, copy_components, color_attachment_index, color_attachment_index};
        }
    };

    ///Copies a region between two framebuffers. If a write framebuffer is not provided, it will write to the default (window) framebuffer.
    struct framebuffer_copy final : command
    {
        explicit framebuffer_copy(const std::string_view& read_framebuffer, const std::optional<std::string_view>& write_framebuffer, const framebuffer_copy_info& copy_info) : copy_info(copy_info), read_framebuffer(read_framebuffer), write_framebuffer(write_framebuffer) {}

        [[nodiscard]] command_type type() const override
        {
            return command_type::FRAMEBUFFER_COPY;
        }

        framebuffer_copy_info copy_info;
        object_identifier read_framebuffer;
        std::optional<object_identifier> write_framebuffer;
    };
}
