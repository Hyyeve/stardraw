#pragma once
#include <functional>
#include <string>

#include "render_context.hpp"
#include "types.hpp"

namespace stardraw
{
    using namespace starlib_stdint;
    enum class cursor_mode
    {
        NORMAL, HIDDEN, CAPTURED, CONFINED,
    };

    struct window_config
    {
        graphics_api api;

        std::string title = "Stardraw Window";

        u32 width = 1280;
        u32 height = 720;

        bool transparent_framebuffer = false;
        bool debug_graphics_context = false;
    };

    struct fullscreen_window_config
    {
        u32 width;
        u32 height;
        u32 refresh_rate;
    };

    struct display_info
    {
        std::string name;
        u32 index;
        u32 width;
        u32 height;
        i32 position_x;
        i32 positoin_y;
    };

    class window
    {
    public:
        virtual ~window() = default;

        [[nodiscard]] static status create(const window_config& config, window** out_window);
        virtual render_context* get_render_context() = 0;

        //Global functionality
        virtual status set_title(const std::string& title) = 0;
        virtual status set_icon(const u32 width, const u32 height, void* rgba8_pixels) = 0;
        virtual status set_cursor_mode(const cursor_mode mode) = 0;
        virtual status set_vsync(const bool sync) = 0; //Enabled should pick the lowest latency non-tearing mode available.
        virtual status set_visible(const bool visible) = 0;
        virtual status set_floating(const bool floating) = 0;
        virtual status set_opacity(const f32 opacity) = 0; //Opacity of the window as a whole; not framebuffer.
        virtual status steal_focus() = 0;
        virtual status request_focus() = 0;

        //Windowed mode functionality
        virtual status set_size(const u32 width, const u32 height) = 0;
        virtual status set_position(const i32 x, const i32 y) = 0;
        virtual status set_decorated(const bool decorations) = 0;
        virtual status set_resizable(const bool resizable) = 0;

        virtual status set_resizing_limit(const u32 min_width, const u32 min_height, const u32 max_width, const u32 max_height) = 0;
        virtual status clear_resizing_limit() = 0;
        virtual status set_aspect_ratio_limit(const u32 width, const u32 height) = 0;
        virtual status clear_aspect_ratio_limit() = 0;

        //Mode switching
        virtual status to_exclusive_fullscreen(const u32 display_index, const fullscreen_window_config config) = 0;
        virtual status to_windowed_fullscreen(const u32 display_index) = 0; //Window is set to windowed mode and resized/positioned to cover the display.
        virtual status to_windowed(const u32 width, const u32 height, const i32 x, const i32 y) = 0;

        //Size switching
        virtual status maximise() = 0;
        virtual status minimise() = 0;
        virtual status restore() = 0;

        //Display information
        [[nodiscard]] virtual display_info get_primary_display() const = 0;
        [[nodiscard]] virtual std::vector<display_info> get_available_displays() const = 0;
        [[nodiscard]] virtual std::vector<fullscreen_window_config> get_supported_fullscreen_configs(const u32 display_index) const = 0;

        //Window information
        [[nodiscard]] virtual bool is_close_requested() const = 0;
        [[nodiscard]] virtual bool is_focused() const = 0;

        //Window callbacks
        virtual void set_close_requested_callback(std::function<void(window* window)> func) = 0;
        virtual void set_resized_callback(std::function<void(window* window, const u32 width, const u32 height)> func) = 0;
        virtual void set_repositioned_callback(std::function<void(window* window, const u32 x, const u32 y)> func) = 0;
        virtual void set_minimise_restore_callback(std::function<void(window* window, const bool minimized)> func) = 0;
        virtual void set_maximise_restore_callback(std::function<void(window* window, const bool maximised)> func) = 0;
        virtual void set_focus_callback(std::function<void(window* window, const bool focused)> func) = 0;
        virtual void set_redraw_callback(std::function<void(window* window)>) = 0;

        virtual void TEMP_UPDATE_WINDOW() = 0;

    protected:

        std::unique_ptr<render_context> context;
    };
}
