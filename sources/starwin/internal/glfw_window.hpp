#pragma once

#include "GLFW/glfw3.h"
#include "starlib/types/status.hpp"
#include "starwin/api/window.hpp"

namespace starwin
{
    using namespace starlib;
    using namespace starlib_stdint;

    class glfw_window_input : public window_input
    {
    public:
        glfw_window_input() = default;
        ~glfw_window_input() override;

        [[nodiscard]] keyboard_input* keyboard() override;

        [[nodiscard]] u32 keycode_to_id(starlib_keycodes::keycode keycode) override;
        [[nodiscard]] std::string id_to_name(u32 id) override;

        [[nodiscard]] mouse_input* mouse() override;

        [[nodiscard]] controller_input* controller(const u32 player_index) override;

        void reset_controllers() override;
        void swap_players(const u32 player_index_a, const u32 player_index_b) override;
        [[nodiscard]] bool controller_connected(const u32 player_index) const override;
        [[nodiscard]] std::string controller_name(const u32 player_index) const override;
        [[nodiscard]] bool controller_has_mappings(const u32 player_index) const override;

    protected:
        u32 connect_controller(i32 id) override;
        u32 disconnect_controller(i32 id) override;
        void poll_controllers() override;
    };

    class glfw_window : public window
    {
    public:
        status set_title(const std::string& title) override;
        status set_icon(const u32 width, const u32 height, void* rgba8_pixels) override;
        status set_cursor_mode(const cursor_mode mode) override;
        status set_visible(const bool visible) override;
        status set_floating(const bool floating) override;
        status set_opacity(const f32 opacity) override;
        status steal_focus() override;
        status request_focus() override;

        status set_size(const u32 width, const u32 height) override;
        status set_position(const i32 x, const i32 y) override;
        status set_decorated(const bool decorations) override;
        status set_resizable(const bool resizable) override;

        status set_resizing_limit(const u32 min_width, const u32 min_height, const u32 max_width, const u32 max_height) override;
        status clear_resizing_limit() override;
        status set_aspect_ratio_limit(const u32 width, const u32 height) override;
        status clear_aspect_ratio_limit() override;

        status to_exclusive_fullscreen(const u32 display_index, const fullscreen_window_config config) override;
        status to_windowed_fullscreen(const u32 display_index) override;
        status to_windowed(const u32 width, const u32 height, const i32 x, const i32 y) override;

        status maximise() override;
        status minimise() override;
        status restore() override;

        [[nodiscard]] display_info get_primary_display() const override;
        [[nodiscard]] std::vector<fullscreen_window_config> get_supported_fullscreen_configs(u32 display_index) const override;
        [[nodiscard]] std::vector<display_info> get_available_displays() const override;

        [[nodiscard]] bool is_close_requested() const override;
        [[nodiscard]] bool is_focused() const override;

        [[nodiscard]] gl_loader_func gl_get_loader_func() override;
        [[nodiscard]] status gl_apply_context() override;

        status poll() override;

    protected:
        glfw_window();
        ~glfw_window() override;

        [[nodiscard]] status initialize_window(const window_config& config); //Handles non-graphics related config settings and initializes the handle and callbacks.

        [[nodiscard]] static status status_from_last_glfw_error();
        [[nodiscard]] static std::vector<GLFWmonitor*> get_monitors();
        [[nodiscard]] static display_info get_display_info(GLFWmonitor* monitor, u32 monitor_idx);

        GLFWwindow* handle = nullptr;

    private:
        static void close_requested_event(GLFWwindow* window);
        static void resized_event(GLFWwindow* window, i32 width, i32 height);
        static void repositioned_event(GLFWwindow* window, i32 x, i32 y);
        static void minimized_restored_event(GLFWwindow* window, i32 minimized);
        static void maximized_restored_event(GLFWwindow* window, i32 maximized);
        static void focused_event(GLFWwindow* window, i32 focused);
        static void redraw_event(GLFWwindow* window);
        static void framebuffer_resize_event(GLFWwindow* window, i32 width, i32 height);

        static void key_event(GLFWwindow* window, const i32 key, i32 scancode, const i32 action, i32 mods);
        static void char_event(GLFWwindow* window, const u32 codepoint);
        static void mouse_button_event(GLFWwindow* window, const i32 button, const i32 action, i32 mods);
        static void mouse_scroll_event(GLFWwindow* window, const f64 x_offset, const f64 y_offset);
        static void mouse_position_event(GLFWwindow* window, const f64 x, const f64 y);
        static void joystick_connection_event(const i32 id, const i32 event);

        void on_key_event(const i32 scancode, const i32 action, const i32 mods) const;
        void on_char_event(const u32 codepoint) const;
        void on_mouse_button_event(const i32 button, const i32 action, i32 mods) const;
        void on_mouse_scroll_event(const f64 x_offset, const f64 y_offset) const;
        void on_mouse_position_event(const f64 x, const f64 y) const;
        void on_joystick_connection_event(const i32 id, const i32 event);
    };
}
