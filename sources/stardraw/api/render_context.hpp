#pragma once

#include <string_view>

#include "api_impl.hpp"
#include "commands.hpp"
#include "descriptors.hpp"

namespace stardraw
{
    class render_context;
    typedef std::unique_ptr<render_context> render_context_handle;

    class render_context
    {
    public:
        static render_context_handle create();
        ~render_context();

        render_context(render_context& other) = delete;                      //COPY CONSTRUCTOR
        render_context(render_context&& other) = delete;                     //MOVE CONSTRUCTOR
        render_context& operator=(render_context& other) = delete;           //COPY ASSIGNMENT
        render_context& operator=(render_context&& other) noexcept = delete; //MOVE ASSIGNMENT

        status create_backend(graphics_api api);

        [[nodiscard]] status execute_command_buffer(const std::string_view& name) const;
        [[nodiscard]] status execute_temp_command_buffer(command_list_handle cmd_list) const;
        [[nodiscard]] status create_command_buffer(const std::string_view& name, command_list_handle cmd_list) const;
        [[nodiscard]] status delete_command_buffer(const std::string_view& name) const;
        [[nodiscard]] status create_objects(const descriptor_list_handle descriptors) const;
        [[nodiscard]] status delete_object(const std::string_view& name) const;

        [[nodiscard]] signal_status check_signal(const std::string_view& name) const;
        [[nodiscard]] signal_status wait_signal(const std::string_view& name, const uint64_t timeout_nanos) const;

        template <typename... command_types>
        [[nodiscard]] status execute_commands(const command_types&... commands)
        {
            command_list_builder builder;
            (builder.add(std::forward<const command_types&...>(commands)), ...);
            command_list_handle list = builder.finish();
            return execute_temp_command_buffer(std::move(list));
        }

        template <typename... command_types>
        [[nodiscard]] status emplace_command_buffer(const std::string_view& name, const command_types&... commands)
        {
            command_list_builder builder;
            (builder.add(std::forward<const command_types&>(commands)), ...);
            command_list_handle list = builder.finish();
            return create_command_buffer(name, std::move(list));
        }

        template <typename... descriptor_types>
        [[nodiscard]] status emplace_objects(const descriptor_types&... descriptors)
        {
            descriptor_list_builder builder;
            (builder.add(std::forward<const descriptor_types&>(descriptors)), ...);
            descriptor_list_handle list = builder.finish();
            return create_objects(std::move(list));
        }

    private:
        render_context() = default;
        api_impl* backend = nullptr;
    };
}
