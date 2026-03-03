#pragma once
#include <string_view>
#include <unordered_map>

#include "object_states.hpp"
#include "types.hpp"

#include "stardraw/api/commands.hpp"
#include "stardraw/api/render_context.hpp"
#include "stardraw/api/types.hpp"

namespace stardraw::gl45
{
    class window;

    class render_context final : public stardraw::render_context
    {
    public:
        explicit render_context(window* window);

        [[nodiscard]] status execute_command_buffer(const std::string_view& name) override;
        [[nodiscard]] status execute_temp_command_buffer(const command_list&& commands) override;
        [[nodiscard]] status create_command_buffer(const std::string_view& name, const command_list&& commands) override;
        [[nodiscard]] status delete_command_buffer(const std::string_view& name) override;
        [[nodiscard]] status create_objects(const descriptor_list&& descriptors) override;
        [[nodiscard]] status delete_object(const descriptor_type type, const std::string_view& name) override;

        [[nodiscard]] signal_status check_signal(const std::string_view& name) override;
        [[nodiscard]] signal_status wait_signal(const std::string_view& name, uint64_t timeout) override;

        [[nodiscard]] status prepare_memory_transfer(const memory_transfer_info& info, memory_transfer_handle** out_handle) override;
        [[nodiscard]] status flush_memory_transfer(memory_transfer_handle* handle) override;
    private:
        [[nodiscard]] static status status_from_last_gl_error();


        [[nodiscard]] status execute_command(const command* cmd);
        [[nodiscard]] status execute_draw(const draw_command* cmd) const;
        [[nodiscard]] status execute_draw_indexed(const draw_indexed_command* cmd) const;
        [[nodiscard]] status execute_draw_indirect(const draw_indirect_command* cmd) const;
        [[nodiscard]] status execute_draw_indexed_indirect(const draw_indexed_indirect_command* cmd) const;
        [[nodiscard]] status execute_buffer_copy(const buffer_copy_command* cmd);
        [[nodiscard]] status execute_draw_config(const draw_config_command* cmd);
        [[nodiscard]] static status execute_config_blending(const blending_config_command* cmd);
        [[nodiscard]] static status execute_config_stencil(const stencil_config_command* cmd);
        [[nodiscard]] static status execute_config_scissor(const scissor_config_command* cmd);
        [[nodiscard]] static status execute_config_face_cull(const face_cull_config_command* cmd);
        [[nodiscard]] static status execute_config_depth_test(const depth_test_config_command* cmd);
        [[nodiscard]] static status execute_config_depth_range(const depth_range_config_command* cmd);
        [[nodiscard]] static status execute_clear_window(const clear_window_command* cmd);
        [[nodiscard]] status execute_shader_parameters_upload(const shader_config_command* cmd);
        [[nodiscard]] status execute_signal(const signal_command* cmd);

        [[nodiscard]] status create_object(const descriptor* descriptor);
        [[nodiscard]] status create_buffer_state(const buffer_descriptor* descriptor);
        [[nodiscard]] status create_shader_state(const shader_descriptor* descriptor);
        [[nodiscard]] status create_texture_state(const texture_descriptor* descriptor);
        [[nodiscard]] status create_texture_sampler_state(const texture_sampler_descriptor* descriptor);
        [[nodiscard]] status create_vertex_specification_state(const vertex_specification_descriptor* descriptor);
        [[nodiscard]] status create_draw_specification_state(const draw_specification_descriptor* descriptor);

        [[nodiscard]] status bind_vertex_specification_state(const object_identifier& source);
        [[nodiscard]] status bind_draw_specification_state(const object_identifier& source);
        [[nodiscard]] status bind_buffer(const object_identifier& source, GLenum target);
        [[nodiscard]] status bind_shader(const object_identifier& source);
        [[nodiscard]] status bind_shader_buffer_parameter(shader_state* shader, const shader_parameter_location& location, const shader_parameter_value& value);
        [[nodiscard]] status bind_shader_data_parameter(shader_state* shader, const shader_parameter_location& location, shader_parameter_value& value);

        status record_object_state(const object_identifier& identifier, object_state* state);

        template <typename state_type, descriptor_type object_type>
        [[nodiscard]] state_type* find_object_state(const object_identifier& identifier)
        {
            static_assert(std::is_base_of_v<object_state, state_type>);
            if (!objects.contains(object_type)) return nullptr;
            if (objects[object_type].contains(identifier.hash))
            {
                object_state* identified_state = objects[object_type][identifier.hash];
                if (identified_state->object_type() == object_type)
                {
                    return dynamic_cast<state_type*>(identified_state);
                }
            }

            return nullptr;
        }

        [[nodiscard]] inline buffer_state* find_buffer_state(const object_identifier& identifier)
        {
            return find_object_state<buffer_state, descriptor_type::BUFFER>(identifier);
        }

        [[nodiscard]] inline shader_state* find_shader_state(const object_identifier& identifier)
        {
            return find_object_state<shader_state, descriptor_type::SHADER>(identifier);
        }

        [[nodiscard]] inline texture_state* find_texture_state(const object_identifier& identifier)
        {
            return find_object_state<texture_state, descriptor_type::TEXTURE>(identifier);
        }

        [[nodiscard]] inline vertex_specification_state* find_vertex_specification_state(const object_identifier& identifier)
        {
            return find_object_state<vertex_specification_state, descriptor_type::VERTEX_SPECIFICATION>(identifier);
        }

        [[nodiscard]] inline draw_specification_state* find_draw_specification_state(const object_identifier& identifier)
        {
            return find_object_state<draw_specification_state, descriptor_type::DRAW_SPECIFICATION>(identifier);
        }

        window* parent_window;
        std::unordered_map<std::string, command_list> command_lists;
        std::unordered_map<descriptor_type, std::unordered_map<uint64_t, object_state*>> objects;
        std::unordered_map<std::string, signal_state> signals;
        std::unordered_map<memory_transfer_handle*, memory_transfer_info> memory_transfers;
        const draw_specification_state* active_draw_specification = nullptr;
    };
}

