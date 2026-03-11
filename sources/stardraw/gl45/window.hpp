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
        void on_framebuffer_resize(const u32 width, const u32 height) override;
        static void on_gl_error_static(GLenum source, GLenum type, GLuint /*id*/, GLenum severity, GLsizei /*length*/, const GLchar* message, const void* user_ptr);
        void on_gl_error(GLenum source, GLenum type, GLenum severity, const GLchar* message) const;

        std::function<void(const std::string message)> validation_message_callback;

    public:
        void TEMP_UPDATE_WINDOW() override
        {
            glfwSwapBuffers(handle);
            glfwPollEvents();
        }
    };

}