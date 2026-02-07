#pragma once
#include "stardraw/internal/glfw_window.hpp"

namespace stardraw
{
    class gl45_window final : public glfw_window
    {
    public:
        explicit gl45_window(const window_config& config);
        status set_vsync(const bool sync) override;
        status apply_context();

        ~gl45_window() override;

    protected:
        void on_framebuffer_resize(const uint32_t width, const uint32_t height) override;
    };
}