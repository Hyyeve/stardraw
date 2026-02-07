#include "../api/render_context.hpp"

namespace stardraw
{
    render_context_handle render_context::create()
    {
        return render_context_handle(new render_context());
    }

    render_context::~render_context()
    {
        delete backend;
    }

    status render_context::create_backend(const graphics_api api)
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

    status render_context::execute_command_buffer(const std::string_view& name) const
    {
        if (backend == nullptr) return { status_type::NOT_INITIALIZED, "No pipeline backend" };
        return backend->execute_command_buffer(name);
    }

    status render_context::execute_temp_command_buffer(command_list_handle cmd_list) const
    {
        if (backend == nullptr) return { status_type::NOT_INITIALIZED, "No pipeline backend" };
        if (cmd_list == nullptr) return { status_type::UNEXPECTED_NULL, "Null command list"};
        return backend->execute_temp_command_buffer(std::move(cmd_list));
    }

    status render_context::create_command_buffer(const std::string_view& name, command_list_handle cmd_list) const
    {
        if (backend == nullptr) return { status_type::NOT_INITIALIZED, "No pipeline backend" };
        if (cmd_list == nullptr) return { status_type::UNEXPECTED_NULL, "Null command list"};
        return backend->create_command_buffer(name, std::move(cmd_list));
    }

    status render_context::delete_command_buffer(const std::string_view& name) const
    {
        if (backend == nullptr) return { status_type::NOT_INITIALIZED, "No pipeline backend" };
        return backend->delete_command_buffer(name);
    }

    status render_context::create_objects(descriptor_list_handle descriptors) const
    {
        if (backend == nullptr) return { status_type::NOT_INITIALIZED, "No pipeline backend" };
        if (descriptors == nullptr) return { status_type::UNEXPECTED_NULL, "Null descriptor list"};
        return backend->create_objects(std::move(descriptors));
    }

    status render_context::delete_object(const std::string_view& name) const
    {
        if (backend == nullptr) return { status_type::NOT_INITIALIZED, "No pipeline backend" };
        return backend->delete_object(name);
    }

    signal_status render_context::check_signal(const std::string_view& name) const
    {
        if (backend == nullptr) return signal_status::UNKNOWN_SIGNAL;
        return backend->check_signal(name);
    }

    signal_status render_context::wait_signal(const std::string_view& name, const uint64_t timeout_nanos) const
    {
        if (backend == nullptr) return signal_status::UNKNOWN_SIGNAL;
        return backend->wait_signal(name, timeout_nanos);
    }
}
