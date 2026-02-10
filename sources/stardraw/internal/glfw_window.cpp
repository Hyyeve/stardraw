#include "glfw_window.hpp"

#include <format>
#include <tracy/Tracy.hpp>


namespace stardraw
{
    static bool has_loaded_glfw;

    void check_load_glfw()
    {
        if (!has_loaded_glfw)
        {
            has_loaded_glfw = (glfwInit() == GLFW_TRUE);
        }
    }

    status glfw_window::set_title(const std::string& title)
    {
        ZoneScoped;
        glfwSetWindowTitle(handle, title.c_str());
        return status_from_last_glfw_error();
    }

    status glfw_window::set_icon(const uint32_t width, const uint32_t height, void* rgba8_pixels)
    {
        ZoneScoped;
        GLFWimage image;
        image.width = width;
        image.height = height;
        image.pixels = static_cast<unsigned char*>(rgba8_pixels);
        glfwSetWindowIcon(handle, 1, &image);
        return status_from_last_glfw_error();
    }

    status glfw_window::set_cursor_mode(const cursor_mode mode)
    {
        ZoneScoped;
        int32_t mode_id = GLFW_CURSOR_NORMAL;
        switch (mode)
        {
            case cursor_mode::NORMAL:
            {
                mode_id = GLFW_CURSOR_NORMAL;
                break;
            }
            case cursor_mode::HIDDEN:
            {
                mode_id = GLFW_CURSOR_HIDDEN;
                break;
            }
            case cursor_mode::CAPTURED:
            {
                mode_id = GLFW_CURSOR_DISABLED;
                break;
            }
            case cursor_mode::CONFINED:
            {
                mode_id = GLFW_CURSOR_CAPTURED;
                break;
            }
        }

        glfwSetInputMode(handle, GLFW_CURSOR, mode_id);
        return status_from_last_glfw_error();
    }

    status glfw_window::set_visible(const bool visible)
    {
        ZoneScoped;
        if (visible) glfwShowWindow(handle);
        else glfwHideWindow(handle);
        return status_from_last_glfw_error();
    }

    status glfw_window::set_floating(const bool floating)
    {
        ZoneScoped;
        glfwSetWindowAttrib(handle, GLFW_FLOATING, floating);
        return status_from_last_glfw_error();
    }

    status glfw_window::set_opacity(const float opacity)
    {
        ZoneScoped;
        glfwSetWindowOpacity(handle, opacity);
        return status_from_last_glfw_error();
    }

    status glfw_window::steal_focus()
    {
        ZoneScoped;
        glfwFocusWindow(handle);
        return status_from_last_glfw_error();
    }

    status glfw_window::request_focus()
    {
        ZoneScoped;
        glfwRequestWindowAttention(handle);
        return status_from_last_glfw_error();
    }

    status glfw_window::set_size(const uint32_t width, const uint32_t height)
    {
        ZoneScoped;
        glfwSetWindowSize(handle, width, height);
        return status_from_last_glfw_error();
    }

    status glfw_window::set_position(const int32_t x, const int32_t y)
    {
        ZoneScoped;
        glfwSetWindowPos(handle, x, y);
        return status_from_last_glfw_error();
    }

    status glfw_window::set_decorated(const bool decorations)
    {
        ZoneScoped;
        glfwSetWindowAttrib(handle, GLFW_DECORATED, decorations);
        return status_from_last_glfw_error();
    }

    status glfw_window::set_resizable(const bool resizable)
    {
        ZoneScoped;
        glfwSetWindowAttrib(handle, GLFW_RESIZABLE, resizable);
        return status_from_last_glfw_error();
    }

    status glfw_window::set_resizing_limit(const uint32_t min_width, const uint32_t min_height, const uint32_t max_width, const uint32_t max_height)
    {
        ZoneScoped;
        glfwSetWindowSizeLimits(handle, min_width, min_height, max_width, max_height);
        return status_from_last_glfw_error();
    }

    status glfw_window::clear_resizing_limit()
    {
        ZoneScoped;
        glfwSetWindowSizeLimits(handle, GLFW_DONT_CARE, GLFW_DONT_CARE, GLFW_DONT_CARE, GLFW_DONT_CARE);
        return status_from_last_glfw_error();
    }

    status glfw_window::set_aspect_ratio_limit(const uint16_t width, const uint32_t height)
    {
        ZoneScoped;
        glfwSetWindowAspectRatio(handle, width, height);
        return status_from_last_glfw_error();
    }

    status glfw_window::clear_aspect_ratio_limit()
    {
        ZoneScoped;
        glfwSetWindowAspectRatio(handle, GLFW_DONT_CARE, GLFW_DONT_CARE);
        return status_from_last_glfw_error();
    }

    status glfw_window::to_exclusive_fullscreen(const uint32_t display_index, const fullscreen_window_config config)
    {
        ZoneScoped;
        const std::vector<GLFWmonitor*> monitors = get_monitors();
        if (display_index >= monitors.size()) return {status_type::RANGE_OVERFLOW, std::format("Display index {0} is out of range", display_index)};
        GLFWmonitor* desired_monitor = monitors[display_index];
        glfwSetWindowMonitor(handle, desired_monitor, 0, 0, config.width, config.height, config.refresh_rate);
        return status_from_last_glfw_error();
    }

    status glfw_window::to_windowed_fullscreen(const uint32_t display_index)
    {
        ZoneScoped;
        const std::vector<GLFWmonitor*> monitors = get_monitors();
        if (display_index >= monitors.size()) return {status_type::RANGE_OVERFLOW, std::format("Display index {0} is out of range", display_index)};
        GLFWmonitor* desired_monitor = monitors[display_index];

        int32_t monitor_x, monitor_y, monitor_width, monitor_height;
        glfwGetMonitorWorkarea(desired_monitor, &monitor_x, &monitor_y, &monitor_width, &monitor_height);
        glfwSetWindowMonitor(handle, nullptr, monitor_x, monitor_y, monitor_width, monitor_height, GLFW_DONT_CARE);
        return status_from_last_glfw_error();
    }

    status glfw_window::to_windowed(const uint32_t width, const uint32_t height, const int32_t x, const int32_t y)
    {
        ZoneScoped;
        glfwSetWindowMonitor(handle, nullptr, x, y, width, height, GLFW_DONT_CARE);
        return status_from_last_glfw_error();
    }

    status glfw_window::maximise()
    {
        ZoneScoped;
        glfwMaximizeWindow(handle);
        return status_from_last_glfw_error();
    }

    status glfw_window::minimise()
    {
        ZoneScoped;
        glfwIconifyWindow(handle);
        return status_from_last_glfw_error();
    }

    status glfw_window::restore()
    {
        ZoneScoped;
        glfwRestoreWindow(handle);
        return status_from_last_glfw_error();
    }

    display_info glfw_window::get_primary_display() const
    {
        ZoneScoped;
        GLFWmonitor* primary = glfwGetPrimaryMonitor();
        const std::vector<GLFWmonitor*> monitors = get_monitors();

        for (uint32_t idx = 0; idx < monitors.size(); idx++)
        {
            if (primary != monitors[idx]) continue;
            return get_display_info(primary, idx);
        }

        return get_display_info(primary, 0);
    }

    std::vector<fullscreen_window_config> glfw_window::get_supported_fullscreen_configs(const uint32_t display_index) const
    {
        ZoneScoped;
        std::vector<fullscreen_window_config> results;

        const std::vector<GLFWmonitor*> monitors = get_monitors();
        if (display_index >= monitors.size()) return results;
        GLFWmonitor* desired_monitor = monitors[display_index];

        int32_t num_modes;
        const GLFWvidmode* modes = glfwGetVideoModes(desired_monitor, &num_modes);

        for (uint32_t idx = 0; idx < num_modes; idx++)
        {
            const GLFWvidmode mode = modes[idx];
            results.push_back(fullscreen_window_config {static_cast<uint32_t>(mode.width), static_cast<uint32_t>(mode.height), static_cast<uint32_t>(mode.refreshRate)});
        }

        return results;
    }

    std::vector<display_info> glfw_window::get_available_displays() const
    {
        ZoneScoped;
        std::vector<display_info> display_infos;
        const std::vector<GLFWmonitor*> monitors = get_monitors();
        for (uint32_t idx = 0; idx < monitors.size(); idx++)
        {
            display_infos.push_back(get_display_info(monitors[idx], idx));
        }

        return display_infos;
    }

    bool glfw_window::is_close_requested() const
    {
        ZoneScoped;
        return glfwWindowShouldClose(handle);
    }

    bool glfw_window::is_focused() const
    {
        ZoneScoped;
        return glfwGetWindowAttrib(handle, GLFW_FOCUSED);
    }

    void glfw_window::set_close_requested_callback(const std::function<void(window* window)> func)
    {
        close_request_calback = func;
    }

    void glfw_window::set_resized_callback(const std::function<void(window* window, const uint32_t width, const uint32_t height)> func)
    {
        resize_callback = func;
    }

    void glfw_window::set_repositioned_callback(const std::function<void(window* window, const uint32_t x, const uint32_t y)> func)
    {
        reposition_callback = func;
    }

    void glfw_window::set_minimise_restore_callback(const std::function<void(window* window, const bool minimized)> func)
    {
        minimise_restore_callback = func;
    }

    void glfw_window::set_maximise_restore_callback(const std::function<void(window* window, const bool maximised)> func)
    {
        maximise_restore_callback = func;
    }

    void glfw_window::set_focus_callback(const std::function<void(window* window, const bool focused)> func)
    {
        focus_callback = func;
    }

    void glfw_window::set_redraw_callback(const std::function<void(window* window)> func)
    {
        redraw_callback = func;
    }

    glfw_window::glfw_window()
    {
        check_load_glfw();
    }

    void glfw_window::create_window(const window_config& config)
    {
        ZoneScoped;
        glfwWindowHint(GLFW_AUTO_ICONIFY, false);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, config.transparent_framebuffer ? GLFW_TRUE : GLFW_FALSE);
        handle = glfwCreateWindow(config.width, config.height, config.title.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(handle, this);

        glfwSetWindowCloseCallback(handle, close_requested_event);
        glfwSetWindowFocusCallback(handle, focused_event);
        glfwSetWindowSizeCallback(handle, resized_event);
        glfwSetWindowPosCallback(handle, repositioned_event);
        glfwSetWindowRefreshCallback(handle, redraw_event);
        glfwSetWindowIconifyCallback(handle, minimized_restored_event);
        glfwSetWindowMaximizeCallback(handle, maximized_restored_event);
        glfwSetFramebufferSizeCallback(handle, framebuffer_resize_event);
    }

    status glfw_window::status_from_last_glfw_error()
    {
        ZoneScoped;
        const char* description;
        const int32_t error_code = glfwGetError(&description);
        if (error_code == GLFW_NO_ERROR) return status_type::SUCCESS;
        return status { status_type::BACKEND_ERROR, std::string(description) };
    }

    std::vector<GLFWmonitor*> glfw_window::get_monitors()
    {
        ZoneScoped;
        std::vector<GLFWmonitor*> monitors;
        int count;
        GLFWmonitor** pointer = glfwGetMonitors(&count);
        monitors.reserve(count);
        for (int i = 0; i < count; i++)
        {
            monitors.push_back(pointer[i]);
        }
        return monitors;
    }

    display_info glfw_window::get_display_info(GLFWmonitor* monitor, const uint32_t monitor_idx)
    {
        ZoneScoped;
        int32_t x, y, width, height;
        glfwGetMonitorWorkarea(monitor, &x, &y, &width, &height);
        const char* display_name = glfwGetMonitorName(monitor);
        return display_info {std::string(display_name), monitor_idx, static_cast<uint32_t>(width), static_cast<uint32_t>(height), x, y};
    }

    void glfw_window::close_requested_event(GLFWwindow* window)
    {
        ZoneScoped;
        stardraw::glfw_window* _this = static_cast<stardraw::glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle && _this->close_request_calback != nullptr) _this->close_request_calback(_this);
    }

    void glfw_window::resized_event(GLFWwindow* window, const int32_t width, const int32_t height)
    {
        ZoneScoped;
        stardraw::glfw_window* _this = static_cast<stardraw::glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle && _this->resize_callback != nullptr) _this->resize_callback(_this, width, height);
    }

    void glfw_window::repositioned_event(GLFWwindow* window, const int32_t x, const int32_t y)
    {
        ZoneScoped;
        stardraw::glfw_window* _this = static_cast<stardraw::glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle && _this->reposition_callback != nullptr) _this->reposition_callback(_this, x, y);
    }

    void glfw_window::minimized_restored_event(GLFWwindow* window, const int32_t minimized)
    {
        ZoneScoped;
        stardraw::glfw_window* _this = static_cast<stardraw::glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle && _this->minimise_restore_callback != nullptr) _this->minimise_restore_callback(_this, minimized);
    }

    void glfw_window::maximized_restored_event(GLFWwindow* window, const int32_t maximized)
    {
        ZoneScoped;
        stardraw::glfw_window* _this = static_cast<stardraw::glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle && _this->maximise_restore_callback != nullptr) _this->maximise_restore_callback(_this, maximized);
    }

    void glfw_window::focused_event(GLFWwindow* window, const int32_t focused)
    {
        ZoneScoped;
        stardraw::glfw_window* _this = static_cast<stardraw::glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle && _this->focus_callback != nullptr) _this->focus_callback(_this, focused);
    }

    void glfw_window::redraw_event(GLFWwindow* window)
    {
        ZoneScoped;
        stardraw::glfw_window* _this = static_cast<stardraw::glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle && _this->redraw_callback != nullptr) _this->redraw_callback(_this);
    }

    void glfw_window::framebuffer_resize_event(GLFWwindow* window, const int32_t width, const int32_t height)
    {
        ZoneScoped;
        stardraw::glfw_window* _this = static_cast<stardraw::glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle) _this->on_framebuffer_resize(width, height);
    }
}
