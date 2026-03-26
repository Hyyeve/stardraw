#include "starwin/api/input.hpp"

#include <ranges>
#include <tracy/Tracy.hpp>

#include "starlib/general/string.hpp"

namespace starwin
{
    using namespace starlib_stdint;
    void virtual_keyboard_input::type_character(const u32 codepoint)
    {
        current_text += starlib::stringify_utf32(codepoint);
    }

    void virtual_keyboard_input::press_key(const u32 id)
    {
        key_map[id] = 1;
        last_pressed_id = id;
    }

    void virtual_keyboard_input::release_key(const u32 id)
    {
        key_map[id] = -1;
        last_released_id = id;
    }

    bool keyboard_input::is_pressed(const u32 id)
    {
        return key_map[id] > 0;
    }

    bool keyboard_input::is_released(const u32 id)
    {
        return key_map[id] < 0;
    }

    bool keyboard_input::was_pressed_this_frame(const u32 id)
    {
        return get_frame_count(id) == 1;
    }

    bool keyboard_input::was_released_this_frame(const u32 id)
    {
        return get_frame_count(id) == -1;
    }

    i32 keyboard_input::get_frame_count(const u32 id)
    {
        return key_map[id];
    }

    u32 keyboard_input::get_pressed_frames(const u32 id)
    {
        return std::max(0, get_frame_count(id));
    }

    u32 keyboard_input::get_released_frames(const u32 id)
    {
        return std::max(0, -get_frame_count(id));
    }

    std::string keyboard_input::get_typed_text() const
    {
        return current_text;
    }

    i32 keyboard_input::get_last_pressed_this_frame() const
    {
        return last_pressed_id;
    }

    i32 keyboard_input::get_last_released_this_frame() const
    {
        return last_released_id;
    }

    void virtual_keyboard_input::advance_frame()
    {
        ZoneScoped;
        for (i32& value : key_map | std::views::values)
        {
            if (value > 0) value++;
            else if (value < 0) value--;
        }

        current_text.clear();
        last_released_id = -1;
        last_pressed_id = -1;
    }
}
