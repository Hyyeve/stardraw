#include "starwin/api/window.hpp"

#include "gl45_window.hpp"
#include "starlib/types/status.hpp"
#include "tracy/Tracy.hpp"

namespace starwin
{
    using namespace starlib_stdint;

    status window::create(const window_config& config, window*& out_window)
    {
        switch (config.api)
        {
            case graphics_api::GL45: return gl45_window::create(config, out_window);
            default: return {status_type::UNSUPPORTED, "Provided graphics api is not supported."};
        }
    }

    window_input* window::get_input()
    {
        return input_devices;
    }

    virtual_keyboard_input* window::get_keyboard_hardware() const
    {
        return &input_devices->keyboard_hardware;
    }

    virtual_mouse_input* window::get_mouse_hardware() const
    {
        return &input_devices->mouse_hardware;
    }

    u32 window::connect_controller(const i32 id) const
    {
        return input_devices->connect_controller(id);
    }

    u32 window::disconnect_controller(const i32 id) const
    {
        return input_devices->disconnect_controller(id);
    }

    void window::poll_controllers() const
    {
        input_devices->poll_controllers();
    }

    void window::input_advance_frame() const
    {
        input_devices->keyboard_hardware.advance_frame();
        input_devices->mouse_hardware.advance_frame();
        for (virtual_controller_input& controller_input : input_devices->controller_hardware)
        {
            controller_input.advance_frame();
        }
    }
}
