#pragma once

#include <string_view>

#include "commands.hpp"
#include "common.hpp"
#include "descriptors.hpp"
#include "memory_transfer.hpp"
#include "starlib/types/graphics.hpp"
#include "starlib/types/status.hpp"

namespace stardraw
{
    ///Render context configuration information
    struct render_context_config
    {
        ///The graphics API this render context will use.
        ///NOTE: Not all defined graphics APIs are available. Currently implmented backends: GL45
        starlib::graphics_api api;

        ///Toggle whether the backend graphics API validation features should be used.
        ///If enabled, stardraw will be able to detect and report additional data from the backend api.
        ///You probably want to enable this during development.
        ///Note: You may need to set a debug context hint on your window for this to work.
        bool enable_backend_validation = false;

        ///Callback that will be passed additional validation messages that are not returned via statuses.
        std::function<void(const std::string message)> validation_message_callback;

        ///--- OPENGL ---

        ///Custom gl loader function (such as GLFWGetProcAddress, SDL_GL_GetProcAddress, etc)
        ///OpenGL support will work without a loader, but it's more efficient to use one if you have it!
        starlib::gl_loader_func gl_loader;
    };

    ///Main render context interface that manages graphics state and objects.
    ///This is your main interface for performing rendering operations.
    ///It is undefined to create multiple render contexts that rely on the same backend graphics context / window, and may result in unexpected behaviour.
    class render_context
    {
    public:
        ///Create a render context with some configuration.
        static starlib::status create(const render_context_config& info, render_context*& out_ptr);
        virtual ~render_context() = default;

        ///Create a named command buffer to later execute.
        [[nodiscard]] virtual starlib::status create_command_buffer(const std::string_view& name, const command_list&& cmd_buffer) = 0;

        ///Delete a named command buffer.
        [[nodiscard]] virtual starlib::status delete_command_buffer(const std::string_view& name) = 0;

        ///Consume a list of descriptors and set up internal graphics state for them.
        ///Descriptors are consumed in order and may reference objects defined earlier in the list or in any earlier calls.
        ///Descriptor names are globally exclusive irregardless of descriptor type.
        [[nodiscard]] virtual starlib::status create_objects(const descriptor_list&& descriptors) = 0;

        ///Delete a named object.
        ///NOTE: Deleting an object may cause other objects to become invalid (for instance, deleting a texture that has a texture view into it).
        ///Stardraw tries to detect theses cases and may return an error status either from the delete call or a later call,
        ///but error checking deleted objects is harder and less robust than checking object creation,
        ///so unexpected behaviour may occur if you delete an object that is still being referenced by any other objects.
        [[nodiscard]] virtual starlib::status delete_object(descriptor_type type, const std::string_view& name) = 0;

        ///Execute a previously defined command buffer. You should set up and reuse command buffers for commands that do not require dynamic data.
        [[nodiscard]] virtual starlib::status execute_command_buffer(const std::string_view& name) = 0;

        ///Consume and execute a given command buffer immediately. You should only use this for commands that require dynamic data.
        [[nodiscard]] virtual starlib::status execute_command_buffer(const command_list&& cmd_buffer) = 0;

        ///Check the status of a named signal
        [[nodiscard]] virtual signal_status check_signal(const std::string_view& name) = 0;

        ///Wait for a named signal to be flagged
        [[nodiscard]] virtual signal_status wait_signal(const std::string_view& name, const starlib_stdint::u64 timeout_nanos) = 0;

        //Create a memory transfer handle for uploading or downloading data to/from a buffer.
        //Memory transfer handles are single-use and threadsafe.
        [[nodiscard]] virtual starlib::status prepare_buffer_memory_transfer(const buffer_memory_transfer_info& info, memory_transfer_handle*& out_handle) = 0;

        //Flush a memory transfer to/from a buffer, completing or cancelling it. Any memory writes by the transfer are gaurenteed to be visible after flushing.
        //The handle will be deleted by this call.
        [[nodiscard]] virtual starlib::status flush_buffer_memory_transfer(memory_transfer_handle* handle) = 0;

        //Creates and processes a memory transfer immediately. Blocks until the transfer is completed or an error is generated.
        [[nodiscard]] inline starlib::status transfer_buffer_memory_immediate(const buffer_memory_transfer_info& info, void* data)
        {
            memory_transfer_handle* transfer_handle;
            starlib::status prepare_status = prepare_buffer_memory_transfer(info, transfer_handle);
            if (prepare_status.is_error()) return prepare_status;
            transfer_handle->transfer(data);
            return flush_buffer_memory_transfer(transfer_handle);
        }

        //Create a memory transfer handle for uploading or downloading data to/from a texture
        //Memory transfer handles are single-use and threadsafe.
        [[nodiscard]] virtual starlib::status prepare_texture_memory_transfer(const texture_memory_transfer_info& info, memory_transfer_handle*& out_handle) = 0;

        //Flush a memory transfer to/from a buffer, completing or cancelling it. Any memory writes by the transfer are gaurenteed to be visible after flushing.
        //The handle will be deleted by this call.
        [[nodiscard]] virtual starlib::status flush_texture_memory_transfer(memory_transfer_handle* handle) = 0;

        //Creates and processes a memory transfer immediately. Blocks until the transfer is completed or an error is generated.
        [[nodiscard]] inline starlib::status transfer_texture_memory_immediate(const texture_memory_transfer_info& info, void* data)
        {
            memory_transfer_handle* transfer_handle;
            starlib::status prepare_status = prepare_texture_memory_transfer(info, transfer_handle);
            if (prepare_status.is_error()) return prepare_status;
            transfer_handle->transfer(data);
            return flush_texture_memory_transfer(transfer_handle);
        }
    };
}
