#include "render_context.hpp"

#include <format>

#include "api_conversion.hpp"
#include "object_states/framebuffer_state.hpp"
#include "object_states/texture_sampler_state.hpp"
#include "stardraw/internal/internal.hpp"
#include "starlib/utility/string.hpp"
#include "tracy/TracyOpenGL.hpp"

namespace stardraw::gl45
{
    static bool has_loaded_glad = false;

    bool check_loaded_glad(const gl_loader_func func)
    {
        ZoneScoped;
        if (!has_loaded_glad)
        {
            if (func != nullptr) has_loaded_glad = gladLoadGLLoader(func);
            else has_loaded_glad = gladLoadGL();
        }
        return has_loaded_glad;
    }

    render_context::render_context(const render_context_config& config, status& out_status) : backend_validation_enabled(config.enable_backend_validation)
    {
        ZoneScoped;
        out_status = status_type::SUCCESS;

        if (!check_loaded_glad(config.gl_loader))
        {
            out_status = {status_type::BACKEND_ERROR, "Failed to initialize GL loader (GLAD) - try verifying the loader function you provided?"};
        }

        validation_message_callback = config.validation_message_callback;
        if (config.enable_backend_validation)
        {
            glDebugMessageCallback(render_context::on_gl_error_static, this);
        }

        TracyGpuContext;
    }

    [[nodiscard]] status render_context::execute_command_buffer(const std::string_view& name)
    {
        ZoneScoped;
        //Opengl doesn't have any persistant command buffers, so we just execute it like a temporary one without consuming it.
        if (!command_lists.contains(std::string(name))) return status_type::UNKNOWN;
        const command_list& refren = command_lists[std::string(name)];

        for (const starlib::polymorphic<command>& cmd : refren)
        {
            const status result = execute_command(cmd.ptr());
            if (result.is_error()) return result;
        }

        return status_from_last_gl_error();
    }

    [[nodiscard]] status render_context::execute_command_buffer(const command_list&& commands)
    {
        ZoneScoped;
        for (const starlib::polymorphic<command>& cmd : commands)
        {
            const status result = execute_command(cmd.ptr());
            if (result.is_error()) return result;
        }

        return status_from_last_gl_error();
    }

    [[nodiscard]] status render_context::create_command_buffer(const std::string_view& name, const command_list&& commands)
    {
        ZoneScoped;
        if (command_lists.contains(std::string(name))) return {status_type::DUPLICATE, std::format("A command buffer named '{0}' already exists", name)};
        command_lists[std::string(name)] = commands;
        return status_type::SUCCESS;
    }

    [[nodiscard]] status render_context::delete_command_buffer(const std::string_view& name)
    {
        ZoneScoped;
        if (!command_lists.contains(std::string(name))) return status_type::NOTHING_TO_DO;
        command_lists.erase(std::string(name));
        return status_type::SUCCESS;
    }

    [[nodiscard]] status render_context::create_objects(const descriptor_list&& descriptors)
    {
        ZoneScoped;
        for (const starlib::polymorphic<descriptor>& descriptor : descriptors)
        {
            const status create_status = create_object(descriptor.ptr());
            if (create_status.is_error()) return create_status;
        }

        return status_from_last_gl_error();
    }

    [[nodiscard]] status render_context::delete_object(const descriptor_type type, const std::string_view& name)
    {
        ZoneScoped;
        const object_identifier identifier = object_identifier(name);
        if (!objects.contains(type)) return status_type::NOTHING_TO_DO;
        if (!objects[type].contains(identifier.hash)) return status_type::NOTHING_TO_DO;

        delete objects[type][identifier.hash];
        objects[type].erase(identifier.hash);

        return status_from_last_gl_error();
    }

    [[nodiscard]] signal_status render_context::check_signal(const std::string_view& name)
    {
        ZoneScoped;
        return wait_signal(name, 0);
    }

    [[nodiscard]] signal_status render_context::wait_signal(const std::string_view& name, const u64 timeout)
    {
        ZoneScoped;
        if (!signals.contains(std::string(name)))
        {
            return signal_status::UNKNOWN_SIGNAL;
        }

        const signal_state& state = signals[std::string(name)];
        const GLenum status = glClientWaitSync(state.sync_point, 0, timeout);
        switch (status)
        {
            case GL_ALREADY_SIGNALED: return signal_status::SIGNALLED;
            case GL_TIMEOUT_EXPIRED: return signal_status::TIMED_OUT;
            case GL_CONDITION_SATISFIED: return signal_status::SIGNALLED;
            default: return signal_status::NOT_SIGNALLED;
        }
    }


    [[nodiscard]] status render_context::execute_command(const command* cmd)
    {
        ZoneScoped;
        if (cmd == nullptr)
        {
            return {status_type::UNEXPECTED, "Null command"};
        }

        const command_type type = cmd->type();
        switch (type)
        {
            case command_type::DRAW: return execute_draw(dynamic_cast<const draw*>(cmd));
            case command_type::DRAW_INDEXED: return execute_draw_indexed(dynamic_cast<const draw_indexed*>(cmd));
            case command_type::DRAW_INDIRECT: return execute_draw_indirect(dynamic_cast<const draw_indirect*>(cmd));
            case command_type::DRAW_INDEXED_INDIRECT: return execute_draw_indexed_indirect(dynamic_cast<const draw_indexed_indirect*>(cmd));

            case command_type::CONFIG_DRAW: return execute_config_draw(dynamic_cast<const configure_draw*>(cmd));
            case command_type::CONFIG_BLENDING: return execute_config_blending(dynamic_cast<const configure_blending*>(cmd));
            case command_type::CONFIG_STENCIL: return execute_config_stencil(dynamic_cast<const configure_stencil*>(cmd));
            case command_type::CONFIG_SCISSOR: return execute_config_scissor(dynamic_cast<const configure_scissor_test*>(cmd));
            case command_type::CONFIG_FACE_CULL: return execute_config_face_cull(dynamic_cast<const configure_face_cull*>(cmd));
            case command_type::CONFIG_DEPTH_TEST: return execute_config_depth_test(dynamic_cast<const configure_depth_test*>(cmd));
            case command_type::CONFIG_DEPTH_RANGE: return execute_config_depth_range(dynamic_cast<const configure_depth_range*>(cmd));

            case command_type::BUFFER_COPY: return execute_buffer_copy(dynamic_cast<const buffer_copy*>(cmd));
            case command_type::TEXTURE_COPY: return execute_texture_copy(dynamic_cast<const texture_copy*>(cmd));

            case command_type::CLEAR_WINDOW: return execute_clear_window(dynamic_cast<const clear_window*>(cmd));
            case command_type::CLEAR_FRAMEBUFFER: return execute_clear_framebuffer(dynamic_cast<const clear_framebuffer*>(cmd));
            case command_type::CLEAR_TEXTURE: return execute_clear_texture(dynamic_cast<const clear_texture*>(cmd));
            case command_type::CONFIG_SHADER: return execute_shader_parameters_upload(dynamic_cast<const configure_shader*>(cmd));
            case command_type::SIGNAL: return execute_signal(dynamic_cast<const signal*>(cmd));
            case command_type::PRESENT: return execute_present(dynamic_cast<const present*>(cmd));
            case command_type::COMPUTE_DISPATCH: return execute_compute_dispatch(dynamic_cast<const dispatch_compute*>(cmd));
            case command_type::COMPUTE_DISPATCH_INDIRECT: return execute_compute_dispatch_indirect(dynamic_cast<const dispatch_compute_indirect*>(cmd));
            case command_type::CONFIG_VIEWPORTS: return execute_config_viewports(dynamic_cast<const comnfigure_viewports*>(cmd));
            case command_type::FRAMEBUFFER_COPY: return execute_framebuffer_copy(dynamic_cast<const framebuffer_copy*>(cmd));
            case command_type::AQUIRE: return execute_aquire(dynamic_cast<const aquire*>(cmd));
        }

        return {status_type::UNSUPPORTED, "Unsupported command"};
    }

    [[nodiscard]] status render_context::create_object(const descriptor* descriptor)
    {
        ZoneScoped;
        const descriptor_type type = descriptor->type();
        switch (type)
        {
            case descriptor_type::BUFFER: return create_buffer_state(dynamic_cast<const buffer*>(descriptor));
            case descriptor_type::SHADER: return create_shader_state(dynamic_cast<const shader*>(descriptor));
            case descriptor_type::VERTEX_CONFIGURATION: return create_vertex_specification_state(dynamic_cast<const vertex_configuration*>(descriptor));
            case descriptor_type::DRAW_CONFIGURATION: return create_draw_specification_state(dynamic_cast<const draw_configuration*>(descriptor));
            case descriptor_type::TEXTURE: return create_texture_state(dynamic_cast<const texture*>(descriptor));
            case descriptor_type::SAMPLER: return create_texture_sampler_state(dynamic_cast<const sampler*>(descriptor));
            case descriptor_type::FRAMEBUFFER: return create_framebuffer_state(dynamic_cast<const framebuffer*>(descriptor));
            case descriptor_type::TRANSFER_BUFFER: return create_transfer_buffer_state(dynamic_cast<const transfer_buffer*>(descriptor));
        }
        return status_type::UNIMPLEMENTED;
    }

    status render_context::status_from_last_gl_error() const
    {
        ZoneScoped;
        if (!backend_validation_enabled)
        {
            return status_type::SUCCESS;
        }

        GLenum latest_status = glGetError();
        if (latest_status == GL_NO_ERROR) return status_type::SUCCESS;

        std::vector<GLenum> errors;
        while (latest_status != GL_NO_ERROR)
        {
            errors.push_back(latest_status);
            latest_status = glGetError();
        }

        std::string error_string = "???";
        switch (errors[0])
        {
            case GL_INVALID_ENUM: error_string = "Invalid enum (probably stardraw bug)";
                break;
            case GL_INVALID_OPERATION: error_string = "Invalid operation (probably stardraw bug)";
                break;
            case GL_INVALID_VALUE: error_string = "Invalid value (probably stardraw bug)";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error_string = "Invalid framebuffer operation (probably stardraw bug)";
                break;
            case GL_OUT_OF_MEMORY: error_string = "Out of memory";
                break;
            case GL_STACK_OVERFLOW: error_string = "Stack overflow (this is definitely a stardraw bug!)";
                break;
            case GL_STACK_UNDERFLOW: error_string = "Stack underflow (this is definitely a stardraw bug!)";
                break;
            default: error_string = "Unknown error";
                break;
        }
        return {status_type::BACKEND_ERROR, std::format("Operations generated {0} GL errors. First error triggered: {1}", errors.size(), error_string)};
    }

    status render_context::find_buffer_state(const object_identifier& identifier, buffer_state** out_state) {
        *out_state = find_object_state<buffer_state, descriptor_type::BUFFER>(identifier);
        if (*out_state == nullptr) return { status_type::UNKNOWN, std::format("No buffer with name '{0}' in context", identifier.name) };
        if (!(*out_state)->is_valid()) return{ status_type::INVALID, std::format("Buffer '{0}' is in an invalid state", identifier.name) };
        return status_type::SUCCESS;
    }

    status render_context::find_shader_state(const object_identifier& identifier, shader_state** out_state) {
        *out_state = find_object_state<shader_state, descriptor_type::SHADER>(identifier);
        if (*out_state == nullptr) return { status_type::UNKNOWN, std::format("No shader with name '{0}' in context", identifier.name) };
        if (!(*out_state)->is_valid()) return{ status_type::INVALID, std::format("Shader '{0}' is in an invalid state", identifier.name) };
        return status_type::SUCCESS;
    }

    status render_context::find_root_texture_state(const object_identifier& identifier, texture_state** out_state)
    {
        *out_state = find_object_state<texture_state, descriptor_type::TEXTURE>(identifier);
        if (*out_state == nullptr) return { status_type::UNKNOWN, std::format("No texture with name '{0}' in context", identifier.name) };
        if (!(*out_state)->is_valid()) return{ status_type::INVALID, std::format("Texture '{0}' is in an invalid state", identifier.name) };
        if ((*out_state)->root_data_store_texture != identifier)
        {
            texture_state* root_state;
            const status root_status = find_root_texture_state((*out_state)->root_data_store_texture, &root_state);
            if (root_status.is_error()) return root_status;
            *out_state = root_state;
        }

        return status_type::SUCCESS;
    }

    status render_context::find_texture_state(const object_identifier& identifier, texture_state** out_state) {
        *out_state = find_object_state<texture_state, descriptor_type::TEXTURE>(identifier);
        if (*out_state == nullptr) return { status_type::UNKNOWN, std::format("No texture with name '{0}' in context", identifier.name) };
        if (!(*out_state)->is_valid()) return{ status_type::INVALID, std::format("Texture '{0}' is in an invalid state", identifier.name) };
        if ((*out_state)->root_data_store_texture != identifier)
        {
            texture_state* root_state;
            const status root_status = find_texture_state((*out_state)->root_data_store_texture, &root_state);
            if (root_status.is_error()) return root_status;
        }

        return status_type::SUCCESS;
    }

    status render_context::find_texture_sampler_state(const object_identifier& identifier, texture_sampler_state** out_state)
    {
        *out_state = find_object_state<texture_sampler_state, descriptor_type::SAMPLER>(identifier);
        if (*out_state == nullptr) return {status_type::UNKNOWN, std::format("No sampler state with name '{0}' exists in context", identifier.name)};
        if (!(*out_state)->is_valid()) return {status_type::INVALID, std::format("Sampler state object '{0}' is in an invalid state", identifier.name)};
        return status_type::SUCCESS;
    }

    status render_context::find_framebuffer_state(const object_identifier& identifier, framebuffer_state** out_state)
    {
        *out_state = find_object_state<framebuffer_state, descriptor_type::FRAMEBUFFER>(identifier);
        if (*out_state == nullptr) return {status_type::UNKNOWN, std::format("No framebuffer with name '{0}' exists in context", identifier.name)};
        if (!(*out_state)->is_valid()) return {status_type::INVALID, std::format("Framebuffer object '{0}' is in an invalid state", identifier.name)};
        return status_type::SUCCESS;
    }

    status render_context::find_vertex_specification_state(const object_identifier& identifier, vertex_specification_state** out_state) {
        *out_state = find_object_state<vertex_specification_state, descriptor_type::VERTEX_CONFIGURATION>(identifier);
        if (*out_state == nullptr) return {status_type::UNKNOWN, std::format("No vertex specification with name '{0}' exists in context", identifier.name)};
        if (!(*out_state)->is_valid()) return {status_type::INVALID, std::format("Vertex specification object '{0}' is in an invalid state", identifier.name)};
        return status_type::SUCCESS;
    }

    status render_context::find_draw_specification_state(const object_identifier& identifier, draw_specification_state** out_state) {
        *out_state = find_object_state<draw_specification_state, descriptor_type::DRAW_CONFIGURATION>(identifier);
        if (*out_state == nullptr) return {status_type::UNKNOWN, std::format("No draw specification with name '{0}' exists in context", identifier.name)};
        return status_type::SUCCESS;
    }

    status render_context::find_transfer_buffer_state(const object_identifier& identifier, transfer_buffer_state** out_state)
    {
        *out_state = find_object_state<transfer_buffer_state, descriptor_type::TRANSFER_BUFFER>(identifier);
        if (*out_state == nullptr) return {status_type::UNKNOWN, std::format("No transfer buffer with name '{0}' exists in context", identifier.name)};
        return status_type::SUCCESS;
    }

    void render_context::on_gl_error_static(const GLenum source, const GLenum type, GLuint, const GLenum severity, GLsizei, const GLchar* message, const void* user_ptr)
    {
        ZoneScoped;
        const render_context* relevant_ctx = static_cast<const render_context*>(user_ptr);
        relevant_ctx->on_gl_error(source, type, severity, message);
    }

    void render_context::on_gl_error(GLenum source, GLenum type, GLenum severity, const GLchar* message) const
    {
        ZoneScoped;
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
