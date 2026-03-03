#pragma once

#include <string_view>

#include "commands.hpp"
#include "descriptors.hpp"
#include "memory_transfer.hpp"

namespace stardraw
{
    class render_context;
    class async_upload_handle;

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

        //Create a memory transfer handle for uploading or downloading data
        //Memory transfer handles are single-use and threadsafe.
        [[nodiscard]] virtual status prepare_memory_transfer(const memory_transfer_info& info, memory_transfer_handle** out_handle) = 0;

        //Flush a memory transfer, completing or cancelling it. Any memory writes by the transfer are gaurenteed to be visible after flushing.
        //The handle will be deleted by this call.
        [[nodiscard]] virtual status flush_memory_transfer(memory_transfer_handle* handle) = 0;

        //Creates and processes a memory transfer immediately. Blocks until the transfer is completed or an error is generated.
        [[nodiscard]] inline status transfer_memory_immediate(const memory_transfer_info& info, void* data)
        {
            memory_transfer_handle* transfer_handle;
            status prepare_status = prepare_memory_transfer(info, &transfer_handle);
            if (is_status_error(prepare_status)) return prepare_status;
            transfer_handle->transfer(data);
            return flush_memory_transfer(transfer_handle);
        }

        [[nodiscard]] virtual signal_status check_signal(const std::string_view& name) = 0;
        [[nodiscard]] virtual signal_status wait_signal(const std::string_view& name, const uint64_t timeout_nanos) = 0;
    };
}
