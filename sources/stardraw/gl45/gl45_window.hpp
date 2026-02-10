#pragma once
// ReSharper disable once CppUnusedIncludeDirective
#include "stardraw/gl45/gl_headers.hpp"
#include "stardraw/internal/glfw_window.hpp"

namespace stardraw::gl45
{
    class gl45_window final : public glfw_window
    {
    public:
        explicit gl45_window(const window_config& config);
        status set_vsync(const bool sync) override;
        status apply_context();
        render_context* get_render_context() override;

        ~gl45_window() override;

    private:
        void on_framebuffer_resize(const uint32_t width, const uint32_t height) override;
    };
}