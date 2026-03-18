#include "glfw_window.hpp"

#include <format>
#include <tracy/Tracy.hpp>

namespace starwin
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

    status glfw_window::set_icon(const u32 width, const u32 height, void* rgba8_pixels)
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
        i32 mode_id = GLFW_CURSOR_NORMAL;
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

    status glfw_window::set_opacity(const f32 opacity)
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

    status glfw_window::set_size(const u32 width, const u32 height)
    {
        ZoneScoped;
        glfwSetWindowSize(handle, width, height);
        return status_from_last_glfw_error();
    }

    status glfw_window::set_position(const i32 x, const i32 y)
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

    status glfw_window::set_resizing_limit(const u32 min_width, const u32 min_height, const u32 max_width, const u32 max_height)
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

    status glfw_window::set_aspect_ratio_limit(const u32 width, const u32 height)
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

    status glfw_window::to_exclusive_fullscreen(const u32 display_index, const fullscreen_window_config config)
    {
        ZoneScoped;
        const std::vector<GLFWmonitor*> monitors = get_monitors();
        if (display_index >= monitors.size()) return {status_type::RANGE_OVERFLOW, std::format("Display index {0} is out of range", display_index)};
        GLFWmonitor* desired_monitor = monitors[display_index];
        glfwSetWindowMonitor(handle, desired_monitor, 0, 0, config.width, config.height, config.refresh_rate);
        return status_from_last_glfw_error();
    }

    status glfw_window::to_windowed_fullscreen(const u32 display_index)
    {
        ZoneScoped;
        const std::vector<GLFWmonitor*> monitors = get_monitors();
        if (display_index >= monitors.size()) return {status_type::RANGE_OVERFLOW, std::format("Display index {0} is out of range", display_index)};
        GLFWmonitor* desired_monitor = monitors[display_index];

        i32 monitor_x, monitor_y, monitor_width, monitor_height;
        glfwGetMonitorWorkarea(desired_monitor, &monitor_x, &monitor_y, &monitor_width, &monitor_height);
        glfwSetWindowMonitor(handle, nullptr, monitor_x, monitor_y, monitor_width, monitor_height, GLFW_DONT_CARE);
        return status_from_last_glfw_error();
    }

    status glfw_window::to_windowed(const u32 width, const u32 height, const i32 x, const i32 y)
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

        for (u32 idx = 0; idx < monitors.size(); idx++)
        {
            if (primary != monitors[idx]) continue;
            return get_display_info(primary, idx);
        }

        return get_display_info(primary, 0);
    }

    std::vector<fullscreen_window_config> glfw_window::get_supported_fullscreen_configs(const u32 display_index) const
    {
        ZoneScoped;
        std::vector<fullscreen_window_config> results;

        const std::vector<GLFWmonitor*> monitors = get_monitors();
        if (display_index >= monitors.size()) return results;
        GLFWmonitor* desired_monitor = monitors[display_index];

        i32 num_modes;
        const GLFWvidmode* modes = glfwGetVideoModes(desired_monitor, &num_modes);

        for (u32 idx = 0; idx < num_modes; idx++)
        {
            const GLFWvidmode mode = modes[idx];
            results.push_back(fullscreen_window_config {static_cast<u32>(mode.width), static_cast<u32>(mode.height), static_cast<u32>(mode.refreshRate)});
        }

        return results;
    }

    std::vector<display_info> glfw_window::get_available_displays() const
    {
        ZoneScoped;
        std::vector<display_info> display_infos;
        const std::vector<GLFWmonitor*> monitors = get_monitors();
        for (u32 idx = 0; idx < monitors.size(); idx++)
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

    gl_loader_func glfw_window::gl_get_loader_func()
    {
        return reinterpret_cast<gl_loader_func>(glfwGetProcAddress);
    }

    status glfw_window::gl_apply_context()
    {
        ZoneScoped;
        glfwMakeContextCurrent(handle);
        return status_from_last_glfw_error();
    }

    status glfw_window::poll()
    {
        ZoneScoped;
        glfwPollEvents();
        return status_from_last_glfw_error();
    }

    glfw_window::glfw_window()
    {
        check_load_glfw();
    }

    status glfw_window::initialize_window(const window_config& config)
    {
        ZoneScoped;
        glfwWindowHint(GLFW_AUTO_ICONIFY, false);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, config.transparent_framebuffer ? GLFW_TRUE : GLFW_FALSE);
        handle = glfwCreateWindow(config.width, config.height, config.title.c_str(), nullptr, nullptr);

        if (handle == nullptr)
        {
            return status_from_last_glfw_error();
        }

        glfwSetWindowUserPointer(handle, this);

        glfwSetWindowCloseCallback(handle, close_requested_event);
        glfwSetWindowFocusCallback(handle, focused_event);
        glfwSetWindowSizeCallback(handle, resized_event);
        glfwSetWindowPosCallback(handle, repositioned_event);
        glfwSetWindowRefreshCallback(handle, redraw_event);
        glfwSetWindowIconifyCallback(handle, minimized_restored_event);
        glfwSetWindowMaximizeCallback(handle, maximized_restored_event);
        glfwSetFramebufferSizeCallback(handle, framebuffer_resize_event);

        return status_type::SUCCESS;
    }

    status glfw_window::status_from_last_glfw_error()
    {
        ZoneScoped;
        const char* description;
        const i32 error_code = glfwGetError(&description);
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

    display_info glfw_window::get_display_info(GLFWmonitor* monitor, const u32 monitor_idx)
    {
        ZoneScoped;
        i32 x, y, width, height;
        glfwGetMonitorWorkarea(monitor, &x, &y, &width, &height);
        const char* display_name = glfwGetMonitorName(monitor);
        return display_info {std::string(display_name), monitor_idx, static_cast<u32>(width), static_cast<u32>(height), x, y};
    }

    void glfw_window::close_requested_event(GLFWwindow* window)
    {
        ZoneScoped;
        glfw_window* _this = static_cast<glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle && _this->callbacks.on_close_requested != nullptr) _this->callbacks.on_close_requested(_this);
    }

    void glfw_window::resized_event(GLFWwindow* window, const i32 width, const i32 height)
    {
        ZoneScoped;
        glfw_window* _this = static_cast<glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle && _this->callbacks.on_resized != nullptr) _this->callbacks.on_resized(_this, width, height);
    }

    void glfw_window::repositioned_event(GLFWwindow* window, const i32 x, const i32 y)
    {
        ZoneScoped;
        glfw_window* _this = static_cast<glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle && _this->callbacks.on_repositioned != nullptr) _this->callbacks.on_repositioned(_this, x, y);
    }

    void glfw_window::minimized_restored_event(GLFWwindow* window, const i32 minimized)
    {
        ZoneScoped;
        glfw_window* _this = static_cast<glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle && _this->callbacks.on_minimization_change != nullptr) _this->callbacks.on_minimization_change(_this, minimized);
    }

    void glfw_window::maximized_restored_event(GLFWwindow* window, const i32 maximized)
    {
        ZoneScoped;
        glfw_window* _this = static_cast<glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle && _this->callbacks.on_maximization_change != nullptr) _this->callbacks.on_maximization_change(_this, maximized);
    }

    void glfw_window::focused_event(GLFWwindow* window, const i32 focused)
    {
        ZoneScoped;
        glfw_window* _this = static_cast<glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle && _this->callbacks.on_focus_change != nullptr) _this->callbacks.on_focus_change(_this, focused);
    }

    void glfw_window::redraw_event(GLFWwindow* window)
    {
        ZoneScoped;
        glfw_window* _this = static_cast<glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle && _this->callbacks.on_redraw != nullptr) _this->callbacks.on_redraw(_this);
    }

    void glfw_window::framebuffer_resize_event(GLFWwindow* window, const i32 width, const i32 height)
    {
        ZoneScoped;
        glfw_window* _this = static_cast<glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle) _this->callbacks.on_framebuffer_recreate(_this, width, height);
    }
}
