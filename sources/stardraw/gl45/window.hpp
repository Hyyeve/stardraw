#pragma once
// ReSharper disable once CppUnusedIncludeDirective
#include "stardraw/gl45/gl_headers.hpp"
#include "stardraw/internal/glfw_window.hpp"

namespace stardraw::gl45
{
    class window final : public glfw_window
    {
    public:
        static status create_gl45_window(const window_config& config, stardraw::window** out_window);

        [[nodiscard]] status set_vsync(const bool sync) override;
        [[nodiscard]] status make_gl_context_active();
        stardraw::render_context* get_render_context() override;

        ~window() override;

    private:
        void on_framebuffer_resize(const uint32_t width, const uint32_t height) override;

    public:
        void TEMP_UPDATE_WINDOW() override
        {
            glfwSwapBuffers(handle);
            glfwPollEvents();
        }
    };

}