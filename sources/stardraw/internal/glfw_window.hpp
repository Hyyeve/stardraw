#pragma once
#include "GLFW/glfw3.h"
#include "stardraw/api/window.hpp"

namespace stardraw
{
    class glfw_window : public window
    {
    public:

        status set_title(const std::string& title) override;
        status set_icon(const uint32_t width, const uint32_t height, void* rgba8_pixels) override;
        status set_cursor_mode(const cursor_mode mode) override;
        status set_visible(const bool visible) override;
        status set_floating(const bool floating) override;
        status set_opacity(const float opacity) override;
        status steal_focus() override;
        status request_focus() override;

        status set_size(const uint32_t width, const uint32_t height) override;
        status set_position(const int32_t x, const int32_t y) override;
        status set_decorated(const bool decorations) override;
        status set_resizable(const bool resizable) override;

        status set_resizing_limit(const uint32_t min_width, const uint32_t min_height, const uint32_t max_width, const uint32_t max_height) override;
        status clear_resizing_limit() override;
        status set_aspect_ratio_limit(const uint16_t width, const uint32_t height) override;
        status clear_aspect_ratio_limit() override;

        status to_exclusive_fullscreen(const uint32_t display_index, const fullscreen_window_config config) override;
        status to_windowed_fullscreen(const uint32_t display_index) override;
        status to_windowed(const uint32_t width, const uint32_t height, const int32_t x, const int32_t y) override;

        status maximise() override;
        status minimise() override;
        status restore() override;

        [[nodiscard]] display_info get_primary_display() const override;
        [[nodiscard]] std::vector<fullscreen_window_config> get_supported_fullscreen_configs(uint32_t display_index) const override;
        [[nodiscard]] std::vector<display_info> get_available_displays() const override;

        [[nodiscard]] bool is_close_requested() const override;
        [[nodiscard]] bool is_focused() const override;

        void set_close_requested_callback(const std::function<void(window* window)> func) override;
        void set_resized_callback(const std::function<void(window* window, const uint32_t width, const uint32_t height)> func) override;
        void set_repositioned_callback(const std::function<void(window* window, const uint32_t x, const uint32_t y)> func) override;
        void set_minimise_restore_callback(const std::function<void(window* window, const bool minimized)> func) override;
        void set_maximise_restore_callback(const std::function<void(window* window, const bool maximised)> func) override;
        void set_focus_callback(const std::function<void(window* window, const bool focused)> func) override;
        void set_redraw_callback(const std::function<void(window* window)> func) override;

    protected:
        glfw_window();

        [[nodiscard]] status create_window(const window_config& config); //Handles non-graphics related config settings and initializes the handle and callbacks.

        virtual void on_framebuffer_resize(const uint32_t width, const uint32_t height) = 0;

        [[nodiscard]] static status status_from_last_glfw_error();
        [[nodiscard]] static std::vector<GLFWmonitor*> get_monitors();
        [[nodiscard]] static display_info get_display_info(GLFWmonitor* monitor, uint32_t monitor_idx);

        GLFWwindow* handle = nullptr;

    private:
        static void close_requested_event(GLFWwindow* window);
        static void resized_event(GLFWwindow* window, int32_t width, int32_t height);
        static void repositioned_event(GLFWwindow* window, int32_t x, int32_t y);
        static void minimized_restored_event(GLFWwindow* window, int32_t minimized);
        static void maximized_restored_event(GLFWwindow* window, int32_t maximized);
        static void focused_event(GLFWwindow* window, int32_t focused);
        static void redraw_event(GLFWwindow* window);
        static void framebuffer_resize_event(GLFWwindow* window, int32_t width, int32_t height);

        std::function<void(window* window)> close_request_calback;
        std::function<void(window* window, const uint32_t width, const uint32_t height)> resize_callback;
        std::function<void(window* window, const uint32_t x, const uint32_t y)> reposition_callback;
        std::function<void(window* window, const bool minimized)> minimise_restore_callback;
        std::function<void(window* window, const bool maximised)> maximise_restore_callback;
        std::function<void(window* window, const bool focused)> focus_callback;
        std::function<void(window* window)> redraw_callback;
    };
}
