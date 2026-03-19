#include "glfw_window.hpp"

#include <format>
#include <ranges>
#include <unordered_set>
#include <tracy/Tracy.hpp>

namespace starwin
{
    using namespace starlib_keycodes;

    constexpr static u32 non_keyboard_id_bit = (1ul << 31ul);
    constexpr static u32 mouse_button_id_offset = 0;
    constexpr static u32 controller_button_id_offset = 20;
    constexpr static u32 controller_axis_id_offset = 40;

    static std::unordered_map<u32, std::string> nonprintable_keyname_map;
    static bool has_init_nonprintable_map;

    static bool has_loaded_glfw;

    void check_load_glfw()
    {
        if (!has_loaded_glfw)
        {
            has_loaded_glfw = (glfwInit() == GLFW_TRUE);
        }
    }

    void init_nonprintable_keyname_map(window_input* input)
    {
        const std::vector<std::tuple<starlib_keycodes::keycode, std::string>> unprintables = {
            {keycode::SPACE, "space"}, {keycode::LEFT_ALT, "left_alt"},
            {keycode::RIGHT_ALT, "right_alt"}, {keycode::ESCAPE, "esc"},
            {keycode::ENTER, "enter"}, {keycode::TAB, "tab"},
            {keycode::BACKSPACE, "backspace"}, {keycode::INSERT, "insert"},
            {keycode::DEL, "delete"}, {keycode::LEFT, "left_arrow"},
            {keycode::RIGHT, "right_arrow"}, {keycode::UP, "up_arrow"},
            {keycode::DOWN, "down_arrow"}, {keycode::PAGE_UP, "page_up"},
            {keycode::PAGE_DOWN, "page_down"}, {keycode::HOME, "home"},
            {keycode::END, "end"}, {keycode::CAPS_LOCK, "capslock"},
            {keycode::SCROLL_LOCK, "scroll_lock"},
            {keycode::NUM_LOCK, "numlock"}, {keycode::PRINT_SCREEN, "printscreen"},
            {keycode::PAUSE, "pause"}, {keycode::F1, "f1"},
            {keycode::F2, "f2"}, {keycode::F3, "f3"},
            {keycode::F4, "f4"}, {keycode::F5, "f5"},
            {keycode::F6, "f6"}, {keycode::F7, "f7"},
            {keycode::F8, "f8"}, {keycode::F9, "f9"},
            {keycode::F10, "f10"}, {keycode::F11, "f11"},
            {keycode::F12, "f12"}, {keycode::F13, "f13"},
            {keycode::F14, "f14"}, {keycode::F15, "f15"},
            {keycode::F16, "f16"}, {keycode::F17, "f17"},
            {keycode::F18, "f18"}, {keycode::F19, "f19"},
            {keycode::F20, "f20"}, {keycode::F21, "f21"},
            {keycode::F22, "f22"}, {keycode::F23, "f23"},
            {keycode::F24, "f24"}, {keycode::F25, "f25"},
            {keycode::NUMPAD_0, "numpad_0"}, {keycode::NUMPAD_1, "numpad_1"},
            {keycode::NUMPAD_2, "numpad_2"}, {keycode::NUMPAD_3, "numpad_3"},
            {keycode::NUMPAD_4, "numpad_4"}, {keycode::NUMPAD_5, "numpad_5"},
            {keycode::NUMPAD_6, "numpad_6"}, {keycode::NUMPAD_7, "numpad_7"},
            {keycode::NUMPAD_8, "numpad_8"}, {keycode::NUMPAD_9, "numpad_9"},
            {keycode::NUMPAD_DECIMAL, "numpad_decimal"}, {keycode::NUMPAD_DIVIDE, "numpad_divide"},
            {keycode::NUMPAD_MULTIPLY, "numpad_multiply"}, {keycode::NUMPAD_SUBTRACT, "numpad_subtract"},
            {keycode::NUMPAD_ADD, "numpad_add"}, {keycode::NUMPAD_ENTER, "numpad_enter"},
            {keycode::NUMPAD_EQUAL, "numpad_equal"}, {keycode::LEFT_SHIFT, "left_shift"},
            {keycode::LEFT_CONTROL, "left_control"}, {keycode::LEFT_ALT, "left_alt"},
            {keycode::LEFT_SUPER, "left_super"}, {keycode::RIGHT_SHIFT, "right_shift"},
            {keycode::RIGHT_CONTROL, "right_control"}, {keycode::RIGHT_ALT, "right_alt"},
            {keycode::RIGHT_SUPER, "right_super"}, {keycode::MENU, "menu"},
        };

        for (const auto& [keycode, name] : unprintables)
        {
            nonprintable_keyname_map[input->keycode_to_id(keycode)] = name;
        }
    }

    static std::unordered_set<glfw_window*> all_windows;

    glfw_window_input::~glfw_window_input() {}

    keyboard_input* glfw_window_input::keyboard()
    {
        return &keyboard_hardware;
    }

    u32 glfw_window_input::keycode_to_id(starlib_keycodes::keycode keycode)
    {
        if (static_cast<u32>(keycode) < 60) return static_cast<u32>(keycode) | non_keyboard_id_bit; //Mouse buttons, gamepad buttons
        return glfwGetKeyScancode(static_cast<int>(keycode));
    }

    std::string glfw_window_input::id_to_name(const u32 id)
    {
        if (!(id & non_keyboard_id_bit))
        {
            if (!has_init_nonprintable_map) init_nonprintable_keyname_map(this);
            if (nonprintable_keyname_map.contains(id)) return nonprintable_keyname_map[id];

            const char* str = glfwGetKeyName(GLFW_KEY_UNKNOWN, id);
            return str != nullptr ? std::string(str) : "unknown";
        }

        switch (id)
        {
            case 0: return "mouse_left";
            case 1: return "mouse_right";
            case 2: return "mouse_middle";
            case 3: return "mouse_side_1";
            case 4: return "mouse_side_2";
            case 5: return "mouse_side_3";
            case 6: return "mouse_side_4";
            case 7: return "mouse_side_5";

            case 20: return "gamepad_a";
            case 21: return "gamepad_b";
            case 22: return "gamepad_x";
            case 23: return "gamepad_y";

            case 24: return "left_bumper";
            case 25: return "right_bumper";

            case 26: return "select";
            case 27: return "start";
            case 28: return "home";

            case 29: return "left_joystick_click";
            case 30: return "right_joystick_click";

            case 31: return "dpad_up";
            case 32: return "dpad_right";
            case 33: return "dpad_down";
            case 34: return "dpad_left";
            default: return "unknown";
        }
    }

    mouse_input* glfw_window_input::mouse()
    {
        return &mouse_hardware;
    }

    controller_input* glfw_window_input::controller(const u32 player_index)
    {
        if (player_to_controller_map[player_index] >= 0) return &controller_hardware[player_index];
        return nullptr;
    }

    void glfw_window_input::reset_controllers()
    {
        ZoneScoped;
        for (i32 i = 0; i < 16; i++)
        {
            player_to_controller_map[i] = -1;
        }

        for (i32 i = 0; i < 16; i++)
        {
            if (glfwJoystickPresent(i)) connect_controller(i);
        }
    }

    void glfw_window_input::swap_players(const u32 player_index_a, const u32 player_index_b)
    {
        std::swap(player_to_controller_map[std::min(player_index_a, 15u)], player_to_controller_map[std::min(player_index_b, 15u)]);
    }

    bool glfw_window_input::controller_connected(const u32 player_index) const
    {
        if (player_index >= 16) return false;
        return player_to_controller_map[player_index] >= 0;
    }

    std::string glfw_window_input::controller_name(const u32 player_index) const
    {
        ZoneScoped;
        const i32 controller_id = player_to_controller_map[player_index];
        if (controller_id < 0) return "Disconnected Controller";
        if (glfwJoystickIsGamepad(controller_id)) return glfwGetGamepadName(controller_id);
        return glfwGetJoystickName(controller_id);
    }

    bool glfw_window_input::controller_has_mappings(const u32 player_index) const
    {
        ZoneScoped;
        if (player_to_controller_map[player_index] >= 0) return glfwJoystickIsGamepad(player_to_controller_map[player_index]);
        return false;
    }

    u32 glfw_window_input::connect_controller(const i32 id)
    {
        ZoneScoped;
        for (i32 i = 0; i < 16; i++)
        {
            if (player_to_controller_map[i] == -1)
            {
                player_to_controller_map[i] = id;
                return i;
            }
        }

        return -1;
    }

    u32 glfw_window_input::disconnect_controller(const i32 id)
    {
        for (i32 i = 0; i < 16; i++)
        {
            if (player_to_controller_map[i] == id)
            {
                player_to_controller_map[i] = -1;
                return i;
            }
        }

        return -1;
    }

    void glfw_window_input::poll_controllers()
    {
        ZoneScoped;

        const u32 dpad_left_id = keycode_to_id(starlib_keycodes::keycode::DPAD_LEFT);
        const u32 dpad_right_id = keycode_to_id(starlib_keycodes::keycode::DPAD_RIGHT);
        const u32 dpad_down_id = keycode_to_id(starlib_keycodes::keycode::DPAD_DOWN);
        const u32 dpad_up_id = keycode_to_id(starlib_keycodes::keycode::DPAD_UP);

        for (i32 idx = 0; idx < 16; idx++)
        {
            if (!glfwJoystickPresent(idx)) continue;
            virtual_controller_input& controller = controller_hardware[idx];

            GLFWgamepadstate gamepad_state;

            if (glfwJoystickIsGamepad(idx) && glfwGetGamepadState(idx, &gamepad_state) == GLFW_TRUE)
            {
                for (i32 axis = 0; axis <= GLFW_GAMEPAD_AXIS_LAST; axis++)
                {
                    const u32 axis_id = axis + controller_axis_id_offset | non_keyboard_id_bit;
                    controller.set_axis(axis_id, gamepad_state.axes[axis]);
                }

                for (i32 button = 0; button <= GLFW_GAMEPAD_BUTTON_LAST; button++)
                {
                    const bool pressed = gamepad_state.buttons[button] == GLFW_PRESS;
                    const u32 button_id = button + controller_button_id_offset | non_keyboard_id_bit; //Offset to align with keycodes enum

                    if (pressed && !controller.is_pressed(button_id)) controller.press_button(button_id);
                    else if (!pressed && controller.is_pressed(button_id)) controller.release_button(button_id);

                    const bool is_dpad = button >= GLFW_GAMEPAD_BUTTON_DPAD_UP;

                    if (is_dpad)
                    {
                        if (pressed && !controller.is_dpad_pressed(button_id)) controller.press_dpad(button_id);
                        else if (!pressed && controller.is_dpad_pressed(button_id)) controller.press_dpad(button_id);
                    }
                }
            }
            else
            {
                i32 buttons_count;
                const unsigned char* buttons = glfwGetJoystickButtons(idx, &buttons_count);
                i32 axes_count;
                const float* axes = glfwGetJoystickAxes(idx, &axes_count);
                i32 hats_count;
                const unsigned char* hats = glfwGetJoystickHats(idx, &hats_count);

                for (i32 i = 0; i < buttons_count; i++)
                {
                    const bool pressed = buttons[i] == GLFW_PRESS;
                    if (pressed && !controller.is_pressed(i)) controller.press_button(i);
                    else if (!pressed && controller.is_pressed(i)) controller.release_button(i);
                }

                for (i32 i = 0; i < axes_count; i++) controller.set_axis(i, axes[i]);

                for (i32 i = 0; i < hats_count; i++)
                {
                    const bool hat_up = hats[i] & GLFW_HAT_UP;
                    const bool hat_down = hats[i] & GLFW_HAT_DOWN;
                    const bool hat_left = hats[i] & GLFW_HAT_LEFT;
                    const bool hat_right = hats[i] & GLFW_HAT_RIGHT;

                    if (hat_up && !controller.is_dpad_pressed(dpad_up_id, i)) controller.press_dpad(dpad_up_id, i);
                    else if (!hat_up && controller.is_dpad_pressed(dpad_up_id, i)) controller.release_dpad(dpad_up_id, i);

                    if (hat_down && !controller.is_dpad_pressed(dpad_down_id, i)) controller.press_dpad(dpad_down_id, i);
                    else if (!hat_down && controller.is_dpad_pressed(dpad_down_id, i)) controller.release_dpad(dpad_down_id, i);

                    if (hat_left && !controller.is_dpad_pressed(dpad_left_id, i)) controller.press_dpad(dpad_left_id, i);
                    else if (!hat_left && controller.is_dpad_pressed(dpad_left_id, i)) controller.release_dpad(dpad_left_id, i);

                    if (hat_right && !controller.is_dpad_pressed(dpad_right_id, i)) controller.press_dpad(dpad_right_id, i);
                    else if (!hat_right && controller.is_dpad_pressed(dpad_right_id, i)) controller.release_dpad(dpad_right_id, i);
                }
            }
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
        input_advance_frame();
        glfwPollEvents();
        poll_controllers();

        return status_from_last_glfw_error();
    }

    glfw_window::glfw_window()
    {
        check_load_glfw();
        all_windows.emplace(this);
        input_devices = new glfw_window_input();
        input_devices->reset_controllers();
    }

    glfw_window::~glfw_window()
    {
        all_windows.erase(this);
        delete input_devices;
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

        glfwSetKeyCallback(handle, key_event);
        glfwSetCharCallback(handle, char_event);
        glfwSetMouseButtonCallback(handle, mouse_button_event);
        glfwSetScrollCallback(handle, mouse_scroll_event);
        glfwSetCursorPosCallback(handle, mouse_position_event);
        glfwSetJoystickCallback(joystick_connection_event);

        return status_type::SUCCESS;
    }

    status glfw_window::status_from_last_glfw_error()
    {
        ZoneScoped;
        const char* description;
        const i32 error_code = glfwGetError(&description);
        if (error_code == GLFW_NO_ERROR) return status_type::SUCCESS;
        return status {status_type::BACKEND_ERROR, std::string(description)};
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

    void glfw_window::key_event(GLFWwindow* window, const i32 key, const i32 scancode, const i32 action, const i32 mods)
    {
        ZoneScoped;
        const glfw_window* _this = static_cast<glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle) _this->on_key_event(scancode, action, mods);
    }

    void glfw_window::char_event(GLFWwindow* window, const u32 codepoint)
    {
        ZoneScoped;
        const glfw_window* _this = static_cast<glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle) _this->on_char_event(codepoint);
    }

    void glfw_window::mouse_button_event(GLFWwindow* window, const i32 button, const i32 action, const i32 mods)
    {
        ZoneScoped;
        const glfw_window* _this = static_cast<glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle) _this->on_mouse_button_event(button, action, mods);
    }

    void glfw_window::mouse_scroll_event(GLFWwindow* window, const f64 x_offset, const f64 y_offset)
    {
        ZoneScoped;
        const glfw_window* _this = static_cast<glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle) _this->on_mouse_scroll_event(x_offset, y_offset);
    }

    void glfw_window::mouse_position_event(GLFWwindow* window, const f64 x, const f64 y)
    {
        ZoneScoped;
        const glfw_window* _this = static_cast<glfw_window*>(glfwGetWindowUserPointer(window));
        if (window == _this->handle) _this->on_mouse_position_event(x, y);
    }

    void glfw_window::joystick_connection_event(const i32 id, const i32 event)
    {
        ZoneScoped;
        for (glfw_window* window : all_windows)
        {
            window->on_joystick_connection_event(id, event);
        }
    }

    void glfw_window::on_key_event(const i32 scancode, const i32 action, const i32 mods) const
    {
        ZoneScoped;
        virtual_keyboard_input* keyboard = get_keyboard_hardware();
        switch (action)
        {
            case GLFW_PRESS:
            {
                keyboard->press_key(scancode);
                break;
            }
            case GLFW_RELEASE:
            {
                keyboard->release_key(scancode);
                break;
            }
            default: break;
        }
    }

    void glfw_window::on_char_event(const u32 codepoint) const
    {
        ZoneScoped;
        virtual_keyboard_input* keyboard = get_keyboard_hardware();
        keyboard->type_character(codepoint);
    }

    void glfw_window::on_mouse_button_event(const i32 button, const i32 action, i32 mods) const
    {
        ZoneScoped;
        virtual_mouse_input* mouse = get_mouse_hardware();
        const u32 id = button + mouse_button_id_offset | non_keyboard_id_bit;
        switch (action)
        {
            case GLFW_PRESS:
            {
                mouse->click(id);
                break;
            }
            case GLFW_RELEASE:
            {
                mouse->release(id);
                break;
            }
            default: break;
        }
    }

    void glfw_window::on_mouse_scroll_event(const f64 x_offset, const f64 y_offset) const
    {
        ZoneScoped;
        virtual_mouse_input* mouse = get_mouse_hardware();
        mouse->scroll_by({x_offset, y_offset});
    }

    void glfw_window::on_mouse_position_event(const f64 x, const f64 y) const
    {
        ZoneScoped;
        virtual_mouse_input* mouse = get_mouse_hardware();
        i32 win_height = 0;
        glfwGetWindowSize(handle, nullptr, &win_height); //doing this every single time the mouse is updated probably isn't great, but..
        mouse->move_to({x, win_height - y});
    }

    void glfw_window::on_joystick_connection_event(const i32 id, const i32 event)
    {
        ZoneScoped;
        if (event == GLFW_CONNECTED)
        {
            const u32 player_id = connect_controller(id);
            if (player_id != -1) callbacks.on_controller_connect(this, player_id);
        }
        else if (event == GLFW_DISCONNECTED)
        {
            const u32 player_id = disconnect_controller(id);
            if (player_id != -1) callbacks.on_controller_disconnect(this, player_id);
        }
    }
}
