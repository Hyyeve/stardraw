#pragma once
#include <array>
#include <functional>
#include <string>

#include "input.hpp"
#include "starlib/general/keycode.hpp"
#include "starlib/types/graphics.hpp"
#include "starlib/types/status.hpp"

namespace starwin
{
    enum class cursor_mode
    {
        NORMAL, HIDDEN, CAPTURED, CONFINED,
    };

    struct window_config
    {
        starlib::graphics_api api;

        std::string title = "Stardraw Window";

        starlib_stdint::u32 width = 1280;
        starlib_stdint::u32 height = 720;

        bool transparent_framebuffer = false;

        ///OPENGL ONLY: Enable opengl debug context support
        bool gl_debug_context = false;
    };

    class window;
    struct window_callbacks
    {
        //General window callbacks

        std::function<void(window* window)> on_close_requested;
        std::function<void(window* window, const starlib_stdint::u32 width, const starlib_stdint::u32 height)> on_resized;
        std::function<void(window* window, const starlib_stdint::u32 x, const starlib_stdint::u32 y)> on_repositioned;
        std::function<void(window* window, const bool minimized)> on_minimization_change;
        std::function<void(window* window, const bool maximised)> on_maximization_change;
        std::function<void(window* window, const bool focused)> on_focus_change;

        //Input handling

        std::function<void(window* window, const starlib_stdint::u32 player_id)> on_controller_connect;
        std::function<void(window* window, const starlib_stdint::u32 player_id)> on_controller_disconnect;

        //Graphics callbacks

        ///NOTE: You should not use this for your main render loop.
        ///This function is triggered when a redraw is requested due to input events such as the window being repositioned/resized by the user.
        ///Use this function ONLY to make sure your window continues rendering correctly during those events.
        std::function<void(window* window)> on_redraw;

        ///NOTE: This may or may not be called at the same time as an actual window resize.
        ///You should rely on this event for triggering graphics framebuffer resizes instead of on_resized
        std::function<void(window* window, const starlib_stdint::u32 width, const starlib_stdint::u32 height)> on_framebuffer_recreate;
    };

    struct fullscreen_window_config
    {
        starlib_stdint::u32 width;
        starlib_stdint::u32 height;
        starlib_stdint::u32 refresh_rate;
    };

    struct display_info
    {
        std::string name;
        starlib_stdint::u32 index;
        starlib_stdint::u32 width;
        starlib_stdint::u32 height;
        starlib_stdint::i32 position_x;
        starlib_stdint::i32 positoin_y;
    };

    class window_input
    {
    public:
        virtual ~window_input() = default;

        [[nodiscard]] virtual keyboard_input* keyboard() = 0;

        ///Convert a named keycode into an ID to use with device input functions
        ///Note: Controllers with no mappings have arbitrary IDs for all buttons/axes/dpads, but IDs for these always start from 0.
        [[nodiscard]] virtual starlib_stdint::u32 keycode_to_id(starlib_keycodes::keycode keycode) = 0;

        ///Convert an ID to a string name. For printable characters, this is usually the character itself.
        ///For other ids, this is a lowercase identifier
        ///Ex: dpad_left, mouse_middle, gamepad_b ...
        [[nodiscard]] virtual std::string id_to_name(starlib_stdint::u32 id) = 0;

        [[nodiscard]] virtual mouse_input* mouse() = 0;
        [[nodiscard]] virtual controller_input* controller(const starlib_stdint::u32 player_index) = 0;

        virtual void reset_controllers() = 0;
        virtual void swap_players(const starlib_stdint::u32 player_index_a, const starlib_stdint::u32 player_index_b) = 0;
        [[nodiscard]] virtual bool controller_connected(const starlib_stdint::u32 player_index) const = 0;
        [[nodiscard]] virtual std::string controller_name(const starlib_stdint::u32 player_index) const = 0;
        [[nodiscard]] virtual bool controller_has_mappings(const starlib_stdint::u32 player_index) const = 0;

    protected:
        window_input() = default;

        virtual starlib_stdint::u32 connect_controller(starlib_stdint::i32 id) = 0;
        virtual starlib_stdint::u32 disconnect_controller(starlib_stdint::i32 id) = 0;
        virtual void poll_controllers() = 0;

        std::array<virtual_controller_input, 16> controller_hardware;
        std::array<starlib_stdint::i32, 16> player_to_controller_map {};
        virtual_mouse_input mouse_hardware;
        virtual_keyboard_input keyboard_hardware;

        friend class window;
    };

    class window
    {
    public:
        static starlib::status create(const window_config& config, window*& out_window);
        virtual ~window() = default;

        //Global functionality

        virtual starlib::status set_title(const std::string& title) = 0;
        virtual starlib::status set_icon(const starlib_stdint::u32 width, const starlib_stdint::u32 height, void* rgba8_pixels) = 0;
        virtual starlib::status set_cursor_mode(const cursor_mode mode) = 0;
        virtual starlib::status set_vsync(const bool sync) = 0; //Enabled should pick the lowest latency non-tearing mode available.
        virtual starlib::status set_visible(const bool visible) = 0;
        virtual starlib::status set_floating(const bool floating) = 0;
        virtual starlib::status set_opacity(const starlib_stdint::f32 opacity) = 0; //Opacity of the window as a whole; not framebuffer.
        virtual starlib::status steal_focus() = 0;
        virtual starlib::status request_focus() = 0;

        //Windowed mode functionality

        virtual starlib::status set_size(const starlib_stdint::u32 width, const starlib_stdint::u32 height) = 0;
        virtual starlib::status set_position(const starlib_stdint::i32 x, const starlib_stdint::i32 y) = 0;
        virtual starlib::status set_decorated(const bool decorations) = 0;
        virtual starlib::status set_resizable(const bool resizable) = 0;

        virtual starlib::status set_resizing_limit(const starlib_stdint::u32 min_width, const starlib_stdint::u32 min_height, const starlib_stdint::u32 max_width, const starlib_stdint::u32 max_height) = 0;
        virtual starlib::status clear_resizing_limit() = 0;
        virtual starlib::status set_aspect_ratio_limit(const starlib_stdint::u32 width, const starlib_stdint::u32 height) = 0;
        virtual starlib::status clear_aspect_ratio_limit() = 0;

        //Mode switching

        virtual starlib::status to_exclusive_fullscreen(const starlib_stdint::u32 display_index, const fullscreen_window_config config) = 0;
        virtual starlib::status to_windowed_fullscreen(const starlib_stdint::u32 display_index) = 0; //Window is set to windowed mode and resized/positioned to cover the display.
        virtual starlib::status to_windowed(const starlib_stdint::u32 width, const starlib_stdint::u32 height, const starlib_stdint::i32 x, const starlib_stdint::i32 y) = 0;

        //Size switching

        virtual starlib::status maximise() = 0;
        virtual starlib::status minimise() = 0;
        virtual starlib::status restore() = 0;

        ///Refreshes the window. Should be called at the start of your rendering.
        virtual starlib::status refresh() = 0;

        ///Polls inputs/window events. Should be called at the start of your update.
        virtual starlib::status poll() = 0;

        //Display information

        [[nodiscard]] virtual display_info get_primary_display() const = 0;
        [[nodiscard]] virtual std::vector<display_info> get_available_displays() const = 0;
        [[nodiscard]] virtual std::vector<fullscreen_window_config> get_supported_fullscreen_configs(const starlib_stdint::u32 display_index) const = 0;

        //Window information

        [[nodiscard]] virtual bool is_close_requested() const = 0;
        [[nodiscard]] virtual bool is_focused() const = 0;

        [[nodiscard]] virtual window_input* get_input();

        ///OPENGL Only: Get the GL api loader function
        [[nodiscard]] virtual starlib::gl_loader_func gl_get_loader_func() = 0;

        ///OPENGL Only: Set the current opengl context to this window's context.
        ///If you're only using a single context/window, you never need to call this.
        ///If you're using multiple windows, you need to call this before doing any rendering to this window.
        [[nodiscard]] virtual starlib::status gl_apply_context() = 0;

        window_callbacks callbacks;
    protected:

        virtual_keyboard_input* get_keyboard_hardware() const;
        virtual_mouse_input* get_mouse_hardware() const;
        starlib_stdint::u32 connect_controller(starlib_stdint::i32 id) const;
        starlib_stdint::u32 disconnect_controller(starlib_stdint::i32 id) const;
        void poll_controllers() const;
        void input_advance_frame() const;

        window_input* input_devices = nullptr;
    };
}
