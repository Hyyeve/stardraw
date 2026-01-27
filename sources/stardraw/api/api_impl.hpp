#pragma once
#include <string_view>

#include "commands.hpp"
#include "descriptors.hpp"

namespace stardraw
{
    class api_impl
    {
    public:
        static api_impl* create(const graphics_api api);
        virtual ~api_impl() = default;
        [[nodiscard]] virtual status execute_command(const command* cmd) = 0;
        [[nodiscard]] virtual status execute_command_buffer(const std::string_view& name) = 0;
        [[nodiscard]] virtual status execute_temp_command_buffer(command_list_ptr commands) = 0;
        [[nodiscard]] virtual status create_command_buffer(const std::string_view& name, command_list_ptr commands) = 0;
        [[nodiscard]] virtual status delete_command_buffer(const std::string_view& name) = 0;
        [[nodiscard]] virtual status create_objects(descriptor_list_ptr descriptors) = 0;
        [[nodiscard]] virtual status delete_object(const std::string_view& name) = 0;
        [[nodiscard]] virtual signal_status check_signal(const std::string_view& name) = 0;
        [[nodiscard]] virtual signal_status wait_signal(const std::string_view& name, uint64_t timeout) = 0;
    };
}
