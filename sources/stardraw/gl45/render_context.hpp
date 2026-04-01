#pragma once
#include <string_view>
#include <unordered_map>

#include "object_states/buffer_state.hpp"
#include "object_states/draw_specification_state.hpp"
#include "object_states/framebuffer_state.hpp"
#include "object_states/shader_state.hpp"
#include "object_states/texture_sampler_state.hpp"
#include "object_states/texture_state.hpp"
#include "object_states/vertex_specification_state.hpp"
#include "stardraw/api/render_context.hpp"

namespace stardraw::gl45
{
    class window;

    using namespace starlib;

    class render_context final : public stardraw::render_context
    {
    public:
        explicit render_context(const render_context_config& config, status& out_status);
        [[nodiscard]] status execute_command_buffer(const std::string_view& name) override;
        [[nodiscard]] status execute_command_buffer(const command_list&& commands) override;
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
        [[nodiscard]] status execute_draw(const draw* cmd);
        [[nodiscard]] status execute_draw_indexed(const draw_indexed* cmd);
        [[nodiscard]] status execute_draw_indirect(const draw_indirect* cmd);
        [[nodiscard]] status execute_draw_indexed_indirect(const draw_indexed_indirect* cmd);
        [[nodiscard]] status execute_buffer_copy(const buffer_copy* cmd);
        [[nodiscard]] status execute_texture_copy(const texture_copy* cmd);
        [[nodiscard]] status execute_framebuffer_copy(const framebuffer_copy* cmd);
        [[nodiscard]] status execute_config_draw(const configure_draw* cmd);
        [[nodiscard]] static status execute_config_blending(const configure_blending* cmd);
        [[nodiscard]] static status execute_config_stencil(const configure_stencil* cmd);
        [[nodiscard]] static status execute_config_scissor(const configure_scissor_test* cmd);
        [[nodiscard]] static status execute_config_face_cull(const configure_face_cull* cmd);
        [[nodiscard]] static status execute_config_depth_test(const configure_depth_test* cmd);
        [[nodiscard]] static status execute_config_depth_range(const configure_depth_range* cmd);
        [[nodiscard]] status execute_config_viewports(const comnfigure_viewports* cmd) const;
        [[nodiscard]] static status execute_clear_window(const clear_window* cmd);
        [[nodiscard]] status execute_clear_framebuffer(const clear_framebuffer* cmd);
        [[nodiscard]] status execute_clear_texture(const clear_texture* cmd);
        [[nodiscard]] status execute_compute_dispatch(const dispatch_compute* cmd);
        [[nodiscard]] status execute_compute_dispatch_indirect(const dispatch_compute_indirect* cmd);
        [[nodiscard]] status execute_present(const present* cmd) const;
        [[nodiscard]] status execute_aquire(const aquire* cmd) const;
        [[nodiscard]] status execute_shader_parameters_upload(const configure_shader* cmd);
        [[nodiscard]] status execute_signal(const signal* cmd);

        [[nodiscard]] status create_object(const descriptor* descriptor);
        [[nodiscard]] status create_buffer_state(const buffer* descriptor);
        [[nodiscard]] status create_shader_state(const shader* descriptor);
        [[nodiscard]] status create_texture_state(const texture* descriptor);
        [[nodiscard]] status create_texture_sampler_state(const sampler* descriptor);
        [[nodiscard]] status create_framebuffer_state(const framebuffer* descriptor);
        [[nodiscard]] status create_vertex_specification_state(const vertex_configuration* descriptor);
        [[nodiscard]] status create_draw_specification_state(const draw_configuration* descriptor);
        [[nodiscard]] status create_transfer_buffer_state(const transfer_buffer* descriptor);

        [[nodiscard]] status bind_vertex_specification_state(const object_identifier& source);
        [[nodiscard]] status bind_draw_specification_state(const object_identifier& source);
        [[nodiscard]] status bind_buffer(const object_identifier& source, GLenum target);
        [[nodiscard]] status bind_shader(const object_identifier& source);
        [[nodiscard]] status bind_shader_sampler_parameter(const shader_state* shader, const shader_parameter_location& location, const shader_parameter_value& value);
        [[nodiscard]] status bind_shader_texture_parameter(shader_state* shader, const shader_parameter_location& location, const shader_parameter_value& value, bool as_image);
        [[nodiscard]] status bind_shader_buffer_parameter(shader_state* shader, const shader_parameter_location& location, const shader_parameter_value& value);
        [[nodiscard]] status bind_shader_data_parameter(shader_state* shader, const shader_parameter_location& location, shader_parameter_value& value);

        [[nodiscard]] status find_and_validate_attachment_texture(const framebuffer_attachment_info& attachment, u32& lowest_msaa_level, u32& highest_msaa_level, bool& any_texture_layered, bool& any_texture_not_layered, texture_state** texture_out);

        [[nodiscard]] status record_object_state(const object_identifier& identifier, object_state* state);
        [[nodiscard]] status status_from_last_gl_error() const;

        template <typename state_type, descriptor_type object_type>
        [[nodiscard]] state_type* find_object_state(const object_identifier& identifier)
        {
            ZoneScoped;
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

        [[nodiscard]] status find_buffer_state(const object_identifier& identifier, buffer_state** out_state);
        [[nodiscard]] status find_shader_state(const object_identifier& identifier, shader_state** out_state);
        [[nodiscard]] status find_root_texture_state(const object_identifier& identifier, texture_state** out_state);
        [[nodiscard]] status find_texture_state(const object_identifier& identifier, texture_state** out_state);
        [[nodiscard]] status find_texture_sampler_state(const object_identifier& identifier, texture_sampler_state** out_state);
        [[nodiscard]] status find_framebuffer_state(const object_identifier& identifier, framebuffer_state** out_state);
        [[nodiscard]] status find_vertex_specification_state(const object_identifier& identifier, vertex_specification_state** out_state);
        [[nodiscard]] status find_draw_specification_state(const object_identifier& identifier, draw_specification_state** out_state);
        [[nodiscard]] status find_transfer_buffer_state(const object_identifier& identifier, transfer_buffer_state** out_state);

        static void on_gl_error_static(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_ptr);
        void on_gl_error(GLenum source, GLenum type, GLenum severity, const GLchar* message) const;

        std::unordered_map<std::string, command_list> command_lists;
        std::unordered_map<descriptor_type, std::unordered_map<u64, object_state*>> objects;
        std::unordered_map<std::string, signal_state> signals;
        std::unordered_map<memory_transfer_handle*, buffer_memory_transfer_info> buffer_transfers;
        std::unordered_map<memory_transfer_handle*, texture_memory_transfer_info> texture_transfers;
        memory_barrier_controller mem_barrier_controller;
        draw_specification_state* active_draw_specification = nullptr;
        bool backend_validation_enabled;
        std::function<void(const std::string message)> validation_message_callback;
    };
}

