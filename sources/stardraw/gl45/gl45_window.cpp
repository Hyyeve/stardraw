#include "gl45_window.hpp"

#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>

#include "gl45_render_context.hpp"


namespace stardraw::gl45
{
    static bool has_loaded_glad = false;

    bool check_loaded_glad()
    {
        if (!has_loaded_glad)
        {
            has_loaded_glad = (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0);
        }
        return has_loaded_glad;
    }

    status gl45_window::create_gl45_window(const window_config& config, window** out_window)
    {
        ZoneScoped;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, config.debug_graphics_context);

        gl45_window* window = new gl45_window();
        {
            status create_status = window->create_window(config);
            if (is_status_error(create_status))
            {
                delete window;
                return create_status;
            }
        }

        {
            status context_status = window->make_gl_context_active();
            if (is_status_error(context_status))
            {
                delete window;
                return context_status;
            }
        }

        const bool glad_loaded = check_loaded_glad();
        if (!glad_loaded)
        {
            delete window;
            return {status_type::BACKEND_ERROR, "Couldn't initialize GLAD" };
        }

        window->context.reset(new gl45_render_context(window));

        *out_window = window;
        return status_type::SUCCESS;
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
        const status context_status = make_gl_context_active();
        if (is_status_error(context_status)) return;
        glViewport(0, 0, width, height);
    }
}
