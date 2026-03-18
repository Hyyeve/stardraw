#pragma once
#include <string_view>
#include <unordered_map>

#include "object_states/buffer_state.hpp"
#include "object_states/draw_specification_state.hpp"
#include "object_states/shader_state.hpp"
#include "object_states/texture_state.hpp"
#include "object_states/vertex_specification_state.hpp"
#include "stardraw/api/render_context.hpp"

namespace stardraw::gl45
{
    class window;

    class render_context final : public stardraw::render_context
    {
    public:
        explicit render_context(const render_context_config& config, status& out_status);
        [[nodiscard]] status execute_command_buffer(const std::string_view& name) override;
        [[nodiscard]] status execute_temp_command_buffer(const command_list&& commands) override;
        [[nodiscard]] status create_command_buffer(const std::string_view& name, const command_list&& commands) override;
        [[nodiscard]] status delete_command_buffer(const std::string_view& name) override;
        [[nodiscard]] status create_objects(const descriptor_list&& descriptors) override;
        [[nodiscard]] status delete_object(const descriptor_type type, const std::string_view& name) override;

        [[nodiscard]] signal_status check_signal(const std::string_view& name) override;
        [[nodiscard]] signal_status wait_signal(const std::string_view& name, u64 timeout) override;

        [[nodiscard]] status prepare_buffer_memory_transfer(const buffer_memory_transfer_info& info, memory_transfer_handle*& out_handle) override;
        [[nodiscard]] status flush_buffer_memory_transfer(memory_transfer_handle* handle) override;

        [[nodiscard]] status prepare_texture_memory_transfer(const texture_memory_transfer_info& info, memory_transfer_handle*& out_handle) override;
        [[nodiscard]] status flush_texture_memory_transfer(memory_transfer_handle* handle) override;
    private:
        [[nodiscard]] status execute_command(const command* cmd);
        [[nodiscard]] status execute_draw(const draw_command* cmd);
        [[nodiscard]] status execute_draw_indexed(const draw_indexed_command* cmd);
        [[nodiscard]] status execute_draw_indirect(const draw_indirect_command* cmd);
        [[nodiscard]] status execute_draw_indexed_indirect(const draw_indexed_indirect_command* cmd);
        [[nodiscard]] status execute_buffer_copy(const buffer_copy_command* cmd);
        [[nodiscard]] status execute_texture_copy(const texture_copy_command* cmd);
        [[nodiscard]] status execute_draw_config(const draw_config_command* cmd);
        [[nodiscard]] static status execute_config_blending(const blending_config_command* cmd);
        [[nodiscard]] static status execute_config_stencil(const stencil_config_command* cmd);
        [[nodiscard]] static status execute_config_scissor(const scissor_config_command* cmd);
        [[nodiscard]] static status execute_config_face_cull(const face_cull_config_command* cmd);
        [[nodiscard]] static status execute_config_depth_test(const depth_test_config_command* cmd);
        [[nodiscard]] static status execute_config_depth_range(const depth_range_config_command* cmd);
        [[nodiscard]] static status execute_clear_window(const clear_window_command* cmd);
        [[nodiscard]] status execute_present(const present_command* cmd) const;
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
        [[nodiscard]] status bind_shader_texture_parameter(shader_state* shader, const shader_parameter_location& location, const shader_parameter_value& value, bool as_image);
        [[nodiscard]] status bind_shader_buffer_parameter(shader_state* shader, const shader_parameter_location& location, const shader_parameter_value& value);
        [[nodiscard]] status bind_shader_data_parameter(shader_state* shader, const shader_parameter_location& location, shader_parameter_value& value);

        [[nodiscard]] status record_object_state(const object_identifier& identifier, object_state* state);
        [[nodiscard]] status status_from_last_gl_error() const;

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

        [[nodiscard]] inline status find_buffer_state(const object_identifier& identifier, buffer_state** out_state)
        {
            *out_state = find_object_state<buffer_state, descriptor_type::BUFFER>(identifier);
            if (*out_state == nullptr) return { status_type::UNKNOWN, std::format("No buffer with name '{0}' in context", identifier.name) };
            if (!(*out_state)->is_valid()) return{ status_type::INVALID, std::format("Buffer '{0}' is in an invalid state", identifier.name) };
            return status_type::SUCCESS;
        }

        [[nodiscard]] inline status find_shader_state(const object_identifier& identifier, shader_state** out_state)
        {
            *out_state = find_object_state<shader_state, descriptor_type::SHADER>(identifier);
            if (*out_state == nullptr) return { status_type::UNKNOWN, std::format("No shader with name '{0}' in context", identifier.name) };
            if (!(*out_state)->is_valid()) return{ status_type::INVALID, std::format("Shader '{0}' is in an invalid state", identifier.name) };
            return status_type::SUCCESS;
        }

        [[nodiscard]] inline status find_texture_state(const object_identifier& identifier, texture_state** out_state)
        {
            *out_state = find_object_state<texture_state, descriptor_type::TEXTURE>(identifier);
            if (*out_state == nullptr) return { status_type::UNKNOWN, std::format("No texture with name '{0}' in context", identifier.name) };
            if (!(*out_state)->is_valid()) return{ status_type::INVALID, std::format("Texture '{0}' is in an invalid state", identifier.name) };
            if ((*out_state)->root_data_store_texture != identifier)
            {
                texture_state* root_state;
                const status root_status = find_texture_state((*out_state)->root_data_store_texture, &root_state);
                if (root_status.is_error()) return root_status;
            }

            return status_type::SUCCESS;
        }

        [[nodiscard]] inline status find_vertex_specification_state(const object_identifier& identifier, vertex_specification_state** out_state)
        {
            *out_state = find_object_state<vertex_specification_state, descriptor_type::VERTEX_SPECIFICATION>(identifier);
            if (*out_state == nullptr) return {status_type::UNKNOWN, std::format("No vertex specification with name '{0}' exists in context", identifier.name)};
            if (!(*out_state)->is_valid()) return {status_type::INVALID, std::format("Vertex specification object '{0}' is in an invalid state", identifier.name)};
            return status_type::SUCCESS;
        }

        [[nodiscard]] inline status find_draw_specification_state(const object_identifier& identifier, draw_specification_state** out_state)
        {
            *out_state = find_object_state<draw_specification_state, descriptor_type::DRAW_SPECIFICATION>(identifier);
            if (*out_state == nullptr) return {status_type::UNKNOWN, std::format("No vertex specification with name '{0}' exists in context", identifier.name)};
            return status_type::SUCCESS;
        }

        static void on_gl_error_static(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param);
        void on_gl_error(GLenum lenum, GLenum type, GLenum severity, const GLchar* message) const;

        std::unordered_map<std::string, command_list> command_lists;
        std::unordered_map<descriptor_type, std::unordered_map<u64, object_state*>> objects;
        std::unordered_map<std::string, signal_state> signals;
        std::unordered_map<memory_transfer_handle*, buffer_memory_transfer_info> buffer_transfers;
        std::unordered_map<memory_transfer_handle*, texture_memory_transfer_info> texture_transfers;
        memory_barrier_controller mem_barrier_controller;
        const draw_specification_state* active_draw_specification = nullptr;
        bool backend_validation_enabled;
        std::function<void(const std::string message)> validation_message_callback;
    };
}

