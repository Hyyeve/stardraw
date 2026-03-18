#include "gl45_window.hpp"

#include <tracy/Tracy.hpp>
#include "starlib/general/string.hpp"

namespace starwin
{
    status gl45_window::create(const window_config& config, window*& out_window)
    {
        ZoneScoped;

        gl45_window* win = new gl45_window();

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, config.gl_debug_context);

        status create_status = win->initialize_window(config);
        if (create_status.is_error())
        {
            delete win;
            return create_status;
        }

        status context_status = win->gl_apply_context();
        if (context_status.is_error())
        {
            delete win;
            return context_status;
        }

        out_window = win;
        return status_type::SUCCESS;
    }

    status gl45_window::set_vsync(const bool sync)
    {
        ZoneScoped;
        status context_status = gl_apply_context();
        if (context_status.is_error()) return context_status;
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

    gl45_window::~gl45_window()
    {
        glfwDestroyWindow(handle);
    }

    status gl45_window::refresh()
    {
        ZoneScoped;
        glfwSwapBuffers(handle);
        return status_from_last_glfw_error();
    }
}
