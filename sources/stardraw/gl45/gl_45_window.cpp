#include "gl_45_window.hpp"

#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>


namespace stardraw
{
    gl45_window::gl45_window(const window_config& config)
    {
        ZoneScoped;
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, config.debug_graphics_context);
        create_window(config);
    }

    status gl45_window::set_vsync(const bool sync)
    {
        ZoneScoped;
        status context_status = apply_context();
        if (is_status_error(context_status)) return context_status;
        if (!sync)
        {
            if (glfwExtensionSupported("GLX_EXT_swap_control_tear")) glfwSwapInterval(-1);
            else glfwSwapInterval(0);
        }
        else
        {
            glfwSwapInterval(1);
        }

        return status_from_last_glfw_error();
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    status gl45_window::apply_context()
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Make opengl context active")
        glfwMakeContextCurrent(handle);
        return status_from_last_glfw_error();
    }

    gl45_window::~gl45_window()
    {
        glfwDestroyWindow(handle);
    }

    void gl45_window::on_framebuffer_resize(const uint32_t width, const uint32_t height)
    {
        apply_context();
        glViewport(0, 0, width, height);
    }
}
