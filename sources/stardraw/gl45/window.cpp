#include "window.hpp"

#include <memory>
#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>

#include "render_context.hpp"
#include "starlib/general/string.hpp"


namespace stardraw::gl45
{
    static bool has_loaded_glad = false;

    bool check_loaded_glad()
    {
        if (!has_loaded_glad)
        {
            has_loaded_glad = gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));
        }
        return has_loaded_glad;
    }

    status window::create_gl45_window(const window_config& config, stardraw::window** out_window)
    {
        ZoneScoped;

        window* win = new window();

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, config.enable_backend_validation);

        status create_status = win->initialize_window(config);
        if (create_status.is_error())
        {
            delete win;
            return create_status;
        }

        status context_status = win->make_gl_context_active();
        if (context_status.is_error())
        {
            delete win;
            return context_status;
        }

        const bool glad_loaded = check_loaded_glad();
        if (!glad_loaded)
        {
            delete win;
            return {status_type::BACKEND_ERROR, "Couldn't initialize GLAD"};
        }

        TracyGpuContext; //init tracy context

        win->context = std::make_unique<render_context>(win, config.enable_stardraw_validation, config.enable_backend_validation);
        win->validation_message_callback = config.validation_message_callback;
        if (config.enable_backend_validation)
            glDebugMessageCallback(window::on_gl_error_static, win);

        *out_window = win;
        return status_type::SUCCESS;
    }

    status window::set_vsync(const bool sync)
    {
        ZoneScoped;
        status context_status = make_gl_context_active();
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

    // ReSharper disable once CppMemberFunctionMayBeConst
    status window::make_gl_context_active()
    {
        ZoneScoped;
        glfwMakeContextCurrent(handle);
        return status_from_last_glfw_error();
    }

    stardraw::render_context* window::get_render_context()
    {
        return context.get();
    }

    window::~window()
    {
        glfwDestroyWindow(handle);
    }

    void window::on_framebuffer_resize(const u32 width, const u32 height)
    {
        const status context_status = make_gl_context_active();
        if (context_status.is_error()) return;
        glViewport(0, 0, width, height);
    }

    void window::on_gl_error_static(const GLenum source, const GLenum type, GLuint, const GLenum severity, GLsizei, const GLchar* message, const void* user_ptr)
    {
        const window* relevant_window = static_cast<const window*>(user_ptr);
        relevant_window->on_gl_error(source, type, severity, message);
    }

    void window::on_gl_error(GLenum source, GLenum type, GLenum severity, const GLchar* message) const
    {
        std::string source_str;
        std::string type_str;
        std::string serverity_str;

        switch (source)
        {
            case GL_DEBUG_SOURCE_API: source_str = "API";
                break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM: source_str = "Window System";
                break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER: source_str = "Shader Compiler";
                break;
            case GL_DEBUG_SOURCE_THIRD_PARTY: source_str = "Third Party";
                break;
            case GL_DEBUG_SOURCE_APPLICATION: source_str = "Application";
                break;
            case GL_DEBUG_SOURCE_OTHER: source_str = "Other";
                break;
            default: source_str = "Unknown";
                break;
        }
        switch (type)
        {
            case GL_DEBUG_TYPE_ERROR: type_str = "Error";
                break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_str = "Deprecated Behavior";
                break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: type_str = "Undefined Behavior";
                break;
            case GL_DEBUG_TYPE_PORTABILITY: type_str = "Portability";
                break;
            case GL_DEBUG_TYPE_PERFORMANCE: type_str = "Performance";
                break;
            case GL_DEBUG_TYPE_MARKER: type_str = "Marker";
                break;
            case GL_DEBUG_TYPE_PUSH_GROUP: type_str = "Push Group";
                break;
            case GL_DEBUG_TYPE_POP_GROUP: type_str = "Pop Group";
                break;
            case GL_DEBUG_TYPE_OTHER: type_str = "Other";
                break;
            default: type_str = "Unknown";
                break;
        }

        switch (severity)
        {
            case GL_DEBUG_SEVERITY_HIGH: serverity_str = "Important";
                break;
            case GL_DEBUG_SEVERITY_MEDIUM: serverity_str = "Warn";
                break;
            case GL_DEBUG_SEVERITY_LOW: serverity_str = "Info";
                break;
            default: serverity_str = "Debug";
                break;
        }

        const std::string result = starlib::stringify("GL / ", source_str, " / ", type_str, " / ", serverity_str, " : ", message);
        if (validation_message_callback != nullptr) validation_message_callback(result);
    }
}
