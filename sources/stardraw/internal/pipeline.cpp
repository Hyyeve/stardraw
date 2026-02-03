#include "../api/pipeline.hpp"

namespace stardraw
{
    pipeline_ptr pipeline::create()
    {
        return pipeline_ptr(new pipeline());
    }

    pipeline::~pipeline()
    {
        delete backend;
    }

    status pipeline::create_backend(const graphics_api api)
    {
        if (backend != nullptr)
        {
            return { status_type::ALREADY_INITIALIZED, "Pipeline already has backend set" };
        }

        backend = api_impl::create(api);

        if (backend == nullptr)
        {
            return { status_type::UNSUPPORTED, "Failed to create pipeline backend" };
        }

        return { status_type::SUCCESS };
    }

    status pipeline::execute_command_buffer(const std::string_view& name) const
    {
        if (backend == nullptr) return { status_type::NOT_INITIALIZED, "No pipeline backend" };
        return backend->execute_command_buffer(name);
    }

    status pipeline::execute_temp_command_buffer(command_list_ptr cmd_list) const
    {
        if (backend == nullptr) return { status_type::NOT_INITIALIZED, "No pipeline backend" };
        if (cmd_list == nullptr) return { status_type::UNEXPECTED_NULL, "Null command list"};
        return backend->execute_temp_command_buffer(std::move(cmd_list));
    }

    status pipeline::create_command_buffer(const std::string_view& name, command_list_ptr cmd_list) const
    {
        if (backend == nullptr) return { status_type::NOT_INITIALIZED, "No pipeline backend" };
        if (cmd_list == nullptr) return { status_type::UNEXPECTED_NULL, "Null command list"};
        return backend->create_command_buffer(name, std::move(cmd_list));
    }

    status pipeline::create_objects(descriptor_list_ptr descriptors) const
    {
        if (backend == nullptr) return { status_type::NOT_INITIALIZED, "No pipeline backend" };
        if (descriptors == nullptr) return { status_type::UNEXPECTED_NULL, "Null descriptor list"};
        return backend->create_objects(std::move(descriptors));
    }

    status pipeline::delete_command_buffer(const std::string_view& name) const
    {
        if (backend == nullptr) return { status_type::NOT_INITIALIZED, "No pipeline backend" };
        return backend->delete_command_buffer(name);
    }

    status pipeline::delete_object(const std::string_view& name) const
    {
        if (backend == nullptr) return { status_type::NOT_INITIALIZED, "No pipeline backend" };
        return backend->delete_object(name);
    }

    signal_status pipeline::check_signal(const std::string_view& name) const
    {
        if (backend == nullptr) return signal_status::UNKNOWN_SIGNAL;
        return backend->check_signal(name);
    }

    signal_status pipeline::wait_signal(const std::string_view& name, const uint64_t timeout_nanos) const
    {
        if (backend == nullptr) return signal_status::UNKNOWN_SIGNAL;
        return backend->wait_signal(name, timeout_nanos);
    }
}
