#include "gl45_window.hpp"

#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>

#include "gl45_render_context.hpp"


namespace stardraw::gl45
{
    static bool has_loaded_glad = false;

    void check_load_glad()
    {
        if (!has_loaded_glad)
        {
            has_loaded_glad = (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0);
        }
    }

    gl45_window::gl45_window(const window_config& config)
    {
        ZoneScoped;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, config.debug_graphics_context);

        create_window(config);
        make_gl_context_active();
        check_load_glad();

        context.reset(new gl45_render_context(this));
    }

    status gl45_window::set_vsync(const bool sync)
    {
        ZoneScoped;
        status context_status = make_gl_context_active();
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
    status gl45_window::make_gl_context_active()
    {
        ZoneScoped;
        TracyGpuZone("[Stardraw] Make opengl context active")
        glfwMakeContextCurrent(handle);
        return status_from_last_glfw_error();
    }

    render_context* gl45_window::get_render_context()
    {
        return context.get();
    }

    gl45_window::~gl45_window()
    {
        glfwDestroyWindow(handle);
    }

    void gl45_window::on_framebuffer_resize(const uint32_t width, const uint32_t height)
    {
        make_gl_context_active();
        glViewport(0, 0, width, height);
    }
}
