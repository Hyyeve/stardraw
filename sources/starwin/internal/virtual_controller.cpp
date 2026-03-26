#include <ranges>
#include "starwin/api/input.hpp"

#include "tracy/Tracy.hpp"

namespace starwin
{
    using namespace starlib_stdint;
    bool controller_input::is_pressed(const u32 id)
    {
        return button_map[id] > 0;
    }

    bool controller_input::is_released(const u32 id)
    {
        return button_map[id] < 0;
    }

    bool controller_input::was_pressed_this_frame(const u32 id)
    {
        return get_frame_count(id) == 1;
    }

    bool controller_input::was_released_this_frame(const u32 id)
    {
        return get_frame_count(id) == -1;
    }

    bool controller_input::is_dpad_pressed(const u32 id, const u32 pad_index)
    {
        return pad_map[pad_index][id] > 0;
    }

    bool controller_input::is_dpad_released(const u32 id, const u32 pad_index)
    {
        return pad_map[pad_index][id] < 0;
    }

    bool controller_input::was_dpad_pressed_this_frame(const u32 id, const u32 pad_index)
    {
        return pad_map[pad_index][id] == 1;
    }

    bool controller_input::was_dpad_released_this_frame(const u32 id, const u32 pad_index)
    {
        return pad_map[pad_index][id] == -1;
    }

    f32 controller_input::axis_value(const u32 id)
    {
        return axis_map[id];
    }

    bool controller_input::is_axis_pressed(const u32 id) {
        return axis_pressed_map[id] > 0;
    }

    bool controller_input::is_axis_released(const u32 id)
    {
        return axis_pressed_map[id] < 0;
    }

    bool controller_input::was_axis_pressed_this_frame(const u32 id)
    {
        return axis_pressed_map[id] == 1;
    }

    bool controller_input::was_axis_released_this_frame(const u32 id)
    {
        return axis_pressed_map[id] == -1;
    }

    i32 controller_input::get_frame_count(const u32 id)
    {
        return button_map[id];
    }

    u32 controller_input::get_pressed_frames(const u32 id)
    {
        return std::max(0, get_frame_count(id));
    }

    u32 controller_input::get_released_frames(const u32 id)
    {
        return std::max(0, -get_frame_count(id));
    }

    i32 controller_input::get_axis_frame_count(const u32 id)
    {
        return axis_pressed_map[id];
    }

    u32 controller_input::get_axis_pressed_frames(const u32 id)
    {
        return std::max(0, get_axis_frame_count(id));
    }

    u32 controller_input::get_axis_released_frames(const u32 id)
    {
        return std::max(0, -get_axis_frame_count(id));
    }

    i32 controller_input::get_dpad_frame_count(const u32 pad, const u32 id)
    {
        return pad_map[pad][id];
    }

    u32 controller_input::get_dpad_pressed_frames(const u32 pad, const u32 id)
    {
        return std::max(get_dpad_frame_count(pad, id), 0);
    }

    u32 controller_input::get_dpad_released_frames(const u32 pad, const u32 id)
    {
        return std::max(0, -get_dpad_frame_count(pad, id));
    }

    i32 controller_input::get_last_pressed_this_frame() const
    {
        return last_pressed_button;
    }

    i32 controller_input::get_last_released_this_frame() const
    {
        return last_released_button;
    }

    std::tuple<i32, u32> controller_input::get_last_pressed_dpad_this_frame() const
    {
        return last_pressed_dpad;
    }

    std::tuple<i32, u32> controller_input::get_last_released_dpad_this_frame() const
    {
        return last_released_dpad;
    }

    i32 controller_input::get_last_pressed_axis_this_frame() const
    {
        return last_pressed_axis;
    }

    i32 controller_input::get_last_released_axis_this_frame() const
    {
        return last_released_axis;
    }

    void controller_input::set_axis_press_threshold(const f32 threshold)
    {
        axis_pressed_threshold = threshold;
    }

    void virtual_controller_input::press_button(const u32 id)
    {
        button_map[id] = 1;
        last_pressed_button = id;
    }

    void virtual_controller_input::release_button(const u32 id)
    {
        button_map[id] = -1;
        last_released_button = id;
    }

    void virtual_controller_input::set_axis(const u32 id, const f32 value)
    {
        axis_map[id] = value;
        if (value > axis_pressed_threshold && !is_axis_pressed(id))
        {
            axis_pressed_map[id] = 1;
            last_pressed_axis = id;
        }
        else if (value < axis_pressed_threshold && is_axis_pressed(id))
        {
            axis_pressed_map[id] = -1;
            last_released_axis = id;
        }
    }

    void virtual_controller_input::press_dpad(const u32 id, const u32 pad_index)
    {
        pad_map[pad_index][id] = 1;
        last_pressed_dpad = {id, pad_index};
    }

    void virtual_controller_input::release_dpad(const u32 id, const u32 pad_index)
    {
        pad_map[pad_index][id] = -1;
        last_released_dpad = {id, pad_index};
    }

    void virtual_controller_input::advance_frame()
    {
        ZoneScoped;
        for (i32& value : button_map | std::views::values)
        {
            if (value > 0) value++;
            else if (value < 0) value--;
        }

        for (i32& value : axis_pressed_map | std::views::values)
        {
            if (value > 0) value++;
            else if (value < 0) value--;
        }

        for (std::map<u32, int>& direction_map : pad_map | std::views::values)
        {
            for (i32& value : direction_map | std::views::values)
            {
                if (value > 0) value++;
                else if (value < 0) value--;
            }
        }

        last_pressed_axis = -1;
        last_released_axis = -1;
        last_pressed_button = -1;
        last_released_button = -1;
        last_pressed_dpad = {-1, 0};
        last_released_dpad = {-1, 0};
    }
}
