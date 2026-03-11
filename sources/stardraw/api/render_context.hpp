#pragma once

#include <string_view>

#include "commands.hpp"
#include "common.hpp"
#include "descriptors.hpp"
#include "memory_transfer.hpp"

namespace stardraw
{
    using namespace starlib_stdint;

    enum class signal_status : u8
    {
        SIGNALLED, NOT_SIGNALLED, TIMED_OUT, UNKNOWN_SIGNAL, CONTEXT_ERROR
    };

    class render_context
    {
    public:
        virtual ~render_context() = default;

        [[nodiscard]] virtual status execute_command_buffer(const std::string_view& name) = 0;
        [[nodiscard]] virtual status execute_temp_command_buffer(const command_list&& cmd_list) = 0;
        [[nodiscard]] virtual status create_command_buffer(const std::string_view& name, const command_list&& cmd_list) = 0;
        [[nodiscard]] virtual status delete_command_buffer(const std::string_view& name) = 0;
        [[nodiscard]] virtual status create_objects(const descriptor_list&& descriptors) = 0;
        [[nodiscard]] virtual status delete_object(descriptor_type type, const std::string_view& name) = 0;

        [[nodiscard]] virtual signal_status check_signal(const std::string_view& name) = 0;
        [[nodiscard]] virtual signal_status wait_signal(const std::string_view& name, const u64 timeout_nanos) = 0;

        //Create a memory transfer handle for uploading or downloading data to/from a buffer.
        //Memory transfer handles are single-use and threadsafe.
        [[nodiscard]] virtual status prepare_buffer_memory_transfer(const buffer_memory_transfer_info& info, memory_transfer_handle** out_handle) = 0;

        //Flush a memory transfer to/from a buffer, completing or cancelling it. Any memory writes by the transfer are gaurenteed to be visible after flushing.
        //The handle will be deleted by this call.
        [[nodiscard]] virtual status flush_buffer_memory_transfer(memory_transfer_handle* handle) = 0;

        //Creates and processes a memory transfer immediately. Blocks until the transfer is completed or an error is generated.
        [[nodiscard]] inline status transfer_buffer_memory_immediate(const buffer_memory_transfer_info& info, void* data)
        {
            memory_transfer_handle* transfer_handle;
            status prepare_status = prepare_buffer_memory_transfer(info, &transfer_handle);
            if (prepare_status.is_error()) return prepare_status;
            transfer_handle->transfer(data);
            return flush_buffer_memory_transfer(transfer_handle);
        }

        //Create a memory transfer handle for uploading or downloading data to/from a texture
        //Memory transfer handles are single-use and threadsafe.
        [[nodiscard]] virtual status prepare_texture_memory_transfer(const texture_memory_transfer_info& info, memory_transfer_handle** out_handle) = 0;

        //Flush a memory transfer to/from a buffer, completing or cancelling it. Any memory writes by the transfer are gaurenteed to be visible after flushing.
        //The handle will be deleted by this call.
        [[nodiscard]] virtual status flush_texture_memory_transfer(memory_transfer_handle* handle) = 0;

        //Creates and processes a memory transfer immediately. Blocks until the transfer is completed or an error is generated.
        [[nodiscard]] inline status transfer_texture_memory_immediate(const texture_memory_transfer_info& info, void* data)
        {
            memory_transfer_handle* transfer_handle;
            status prepare_status = prepare_texture_memory_transfer(info, &transfer_handle);
            if (prepare_status.is_error()) return prepare_status;
            transfer_handle->transfer(data);
            return flush_texture_memory_transfer(transfer_handle);
        }
    };
}
