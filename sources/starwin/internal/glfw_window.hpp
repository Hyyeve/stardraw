#pragma once
#include "GLFW/glfw3.h"
#include "starlib/types/status.hpp"
#include "starwin/api/window.hpp"

namespace starwin
{
    using namespace starlib;
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
    };
}
