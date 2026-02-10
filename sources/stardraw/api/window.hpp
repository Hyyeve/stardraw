#pragma once
#include <cstdint>
#include <functional>
#include <string>

#include "render_context.hpp"
#include "types.hpp"

namespace stardraw
{
    enum class cursor_mode
    {
        NORMAL, HIDDEN, CAPTURED, CONFINED,
    };

    struct window_config
    {
        graphics_api api;

        std::string title = "Stardraw Window";

        uint32_t width = 1280;
        uint32_t height = 720;

        bool transparent_framebuffer = false;
        bool debug_graphics_context = false;
    };

    struct fullscreen_window_config
    {
        uint32_t width;
        uint32_t height;
        uint32_t refresh_rate;
    };

    struct display_info
    {
        std::string name;
        uint32_t index;
        uint32_t width;
        uint32_t height;
        int32_t position_x;
        int32_t positoin_y;
    };

    class window
    {
    public:
        static window* create(const window_config& config);
        virtual render_context* get_render_context() = 0;

        //Global functionality
        virtual status set_title(const std::string& title) = 0;
        virtual status set_icon(const uint32_t width, const uint32_t height, void* rgba8_pixels) = 0;
        virtual status set_cursor_mode(const cursor_mode mode) = 0;
        virtual status set_vsync(const bool sync) = 0; //Enabled should pick the lowest latency non-tearing mode available.
        virtual status set_visible(const bool visible) = 0;
        virtual status set_floating(const bool floating) = 0;
        virtual status set_opacity(const float opacity) = 0; //Opacity of the window as a whole; not framebuffer.
        virtual status steal_focus() = 0;
        virtual status request_focus() = 0;

        //Windowed mode functionality
        virtual status set_size(const uint32_t width, const uint32_t height) = 0;
        virtual status set_position(const int32_t x, const int32_t y) = 0;
        virtual status set_decorated(const bool decorations) = 0;
        virtual status set_resizable(const bool resizable) = 0;

        virtual status set_resizing_limit(const uint32_t min_width, const uint32_t min_height, const uint32_t max_width, const uint32_t max_height) = 0;
        virtual status clear_resizing_limit() = 0;
        virtual status set_aspect_ratio_limit(const uint16_t width, const uint32_t height) = 0;
        virtual status clear_aspect_ratio_limit() = 0;

        //Mode switching
        virtual status to_exclusive_fullscreen(const uint32_t display_index, const fullscreen_window_config config) = 0;
        virtual status to_windowed_fullscreen(const uint32_t display_index) = 0; //Window is set to windowed mode and resized/positioned to cover the display.
        virtual status to_windowed(const uint32_t width, const uint32_t height, const int32_t x, const int32_t y) = 0;

        //Size switching
        virtual status maximise() = 0;
        virtual status minimise() = 0;
        virtual status restore() = 0;

        //Display information
        [[nodiscard]] virtual display_info get_primary_display() const = 0;
        [[nodiscard]] virtual std::vector<display_info> get_available_displays() const = 0;
        [[nodiscard]] virtual std::vector<fullscreen_window_config> get_supported_fullscreen_configs(const uint32_t display_index) const = 0;

        //Window information
        [[nodiscard]] virtual bool is_close_requested() const = 0;
        [[nodiscard]] virtual bool is_focused() const = 0;

        //Window callbacks
        virtual void set_close_requested_callback(std::function<void(window* window)> func) = 0;
        virtual void set_resized_callback(std::function<void(window* window, const uint32_t width, const uint32_t height)> func) = 0;
        virtual void set_repositioned_callback(std::function<void(window* window, const uint32_t x, const uint32_t y)> func) = 0;
        virtual void set_minimise_restore_callback(std::function<void(window* window, const bool minimized)> func) = 0;
        virtual void set_maximise_restore_callback(std::function<void(window* window, const bool maximised)> func) = 0;
        virtual void set_focus_callback(std::function<void(window* window, const bool focused)> func) = 0;
        virtual void set_redraw_callback(std::function<void(window* window)>) = 0;

    protected:
        virtual ~window() = 0;

        std::unique_ptr<render_context> context;
    };
}
