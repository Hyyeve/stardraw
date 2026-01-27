#pragma once

#include <string_view>

#include "api_impl.hpp"
#include "commands.hpp"
#include "descriptors.hpp"

namespace stardraw
{
    class pipeline;
    typedef std::unique_ptr<pipeline> pipeline_ptr;

    class pipeline
    {
    public:
        static pipeline_ptr create();
        ~pipeline();

        pipeline(pipeline& other) = delete;                      //COPY CONSTRUCTOR
        pipeline(pipeline&& other) = delete;                     //MOVE CONSTRUCTOR
        pipeline& operator=(pipeline& other) = delete;           //COPY ASSIGNMENT
        pipeline& operator=(pipeline&& other) noexcept = delete; //MOVE ASSIGNMENT

        status create_backend(graphics_api api);

        [[nodiscard]] status execute_command_buffer(const std::string_view& name) const;
        [[nodiscard]] status execute_temp_command_buffer(command_list_ptr cmd_list) const;
        [[nodiscard]] status create_command_buffer(const std::string_view& name, command_list_ptr cmd_list) const;
        [[nodiscard]] status delete_command_buffer(const std::string_view& name) const;
        [[nodiscard]] status create_objects(const descriptor_list_ptr descriptors) const;
        [[nodiscard]] status delete_object(const std::string_view& name) const;

        [[nodiscard]] signal_status check_signal(const std::string_view& name) const;
        [[nodiscard]] signal_status wait_signal(const std::string_view& name, const uint64_t timeout_nanos) const;

        template <typename... command_types>
        [[nodiscard]] status execute_commands(const command_types&... commands)
        {
            command_list_builder builder;
            (builder.add(std::forward<const command_types&...>(commands)), ...);
            command_list_ptr list = builder.finish();
            return execute_temp_command_buffer(std::move(list));
        }

        template <typename... command_types>
        [[nodiscard]] status emplace_command_buffer(const std::string_view& name, const command_types&... commands)
        {
            command_list_builder builder;
            (builder.add(std::forward<const command_types&>(commands)), ...);
            command_list_ptr list = builder.finish();
            return create_command_buffer(name, std::move(list));
        }

        template <typename... descriptor_types>
        [[nodiscard]] status emplace_objects(const descriptor_types&... descriptors)
        {
            descriptor_list_builder builder;
            (builder.add(std::forward<const descriptor_types&>(descriptors)), ...);
            descriptor_list_ptr list = builder.finish();
            return create_objects(std::move(list));
        }

    private:
        pipeline() = default;
        api_impl* backend = nullptr;
    };
}
