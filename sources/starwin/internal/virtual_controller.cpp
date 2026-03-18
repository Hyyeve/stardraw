#include <ranges>
#include "starwin/api/input.hpp"

#include "tracy/Tracy.hpp"

namespace starwin
{
    void virtual_controller_input::press_button(const i32 button)
    {
        button_map[button] = 1;
    }

    void virtual_controller_input::release_button(const i32 button)
    {
        button_map[button] = -1;
    }

    void virtual_controller_input::set_axis(const i32 axis, const f32 value)
    {
        axis_map[axis] = value;
    }

    void virtual_controller_input::press_direction(const i32 pad, const dpad direction)
    {
        pad_map[pad][direction] = 1;
    }

    void virtual_controller_input::release_direction(const i32 pad, const dpad direction)
    {
        pad_map[pad][direction] = -1;
    }

    bool controller_input::is_pressed(const i32 button)
    {
        return button_map[button] > 0;
    }

    bool controller_input::is_released(const i32 button)
    {
        return button_map[button] < 0;
    }

    bool controller_input::was_pressed_this_frame(const i32 button)
    {
        return get_frame_count(button) == 1;
    }

    bool controller_input::was_released_this_frame(const i32 button)
    {
        return get_frame_count(button) == -1;
    }

    bool controller_input::is_direction_pressed(const i32 pad, const dpad direction)
    {
        return pad_map[pad][direction] > 0;
    }

    bool controller_input::is_direction_pressed_this_frame(const i32 pad, const dpad direction)
    {
        return pad_map[pad][direction] == 1;
    }

    f32 controller_input::axis_value(const i32 axis)
    {
        return axis_map[axis];
    }

    bool controller_input::is_axis_pressed(const i32 axis, const f32 threshold)
    {
        return axis_map[axis] > threshold;
    }

    bool controller_input::is_axis_released(const i32 axis, const f32 threshold)
    {
        return axis_map[axis] < threshold;
    }

    i32 controller_input::get_frame_count(const i32 button)
    {
        return button_map[button];
    }

    i32 controller_input::get_pressed_frames(const i32 button)
    {
        return std::max(0, get_frame_count(button));
    }

    i32 controller_input::get_released_frames(const i32 button)
    {
        return std::max(0, -get_frame_count(button));
    }

    i32 controller_input::get_direction_frame_count(const i32 pad, const dpad direction)
    {
        return pad_map[pad][direction];
    }

    i32 controller_input::get_direction_pressed_frames(const i32 pad, const dpad direction)
    {
        return std::max(get_direction_frame_count(pad, direction), 0);
    }

    i32 controller_input::get_direction_released_frames(const i32 pad, const dpad direction)
    {
        return std::max(0, -get_direction_frame_count(pad, direction));
    }

    void virtual_controller_input::advance_frame()
    {
        ZoneScoped;
        for (i32& value : button_map | std::views::values)
        {
            if (value > 0) value++;
            else if (value < 0) value--;
        }

        for (std::map<dpad, int>& direction_map : pad_map | std::views::values)
        {
            for (i32& value : direction_map | std::views::values)
            {
                if (value > 0) value++;
                else if (value < 0) value--;
            }
        }
    }
}
