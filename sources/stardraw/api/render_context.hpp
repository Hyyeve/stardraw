#pragma once

#include <string_view>

#include "commands.hpp"
#include "descriptors.hpp"

namespace stardraw
{
    class render_context;

    class render_context
    {
    public:
        virtual ~render_context() = default;

        [[nodiscard]] virtual status execute_command_buffer(const std::string_view& name) = 0;
        [[nodiscard]] virtual status execute_temp_command_buffer(const command_list&& cmd_list) = 0;
        [[nodiscard]] virtual status create_command_buffer(const std::string_view& name, const command_list&& cmd_list) = 0;
        [[nodiscard]] virtual status delete_command_buffer(const std::string_view& name) = 0;
        [[nodiscard]] virtual status create_objects(const descriptor_list&& descriptors) = 0;
        [[nodiscard]] virtual status delete_object(const std::string_view& name) = 0;

        [[nodiscard]] virtual signal_status check_signal(const std::string_view& name) = 0;
        [[nodiscard]] virtual signal_status wait_signal(const std::string_view& name, const uint64_t timeout_nanos) = 0;

        [[nodiscard]] virtual status cache_shader(const std::string_view& name, void** out_cache_data, uint64_t& out_cache_size) = 0;
    };
}
