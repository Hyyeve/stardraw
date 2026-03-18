#include "starwin/api/input.hpp"

#include <ranges>
#include <tracy/Tracy.hpp>

#include "starlib/general/string.hpp"

namespace starwin
{
    void virtual_keyboard_input::type_character(const u32 codepoint)
    {
        current_text += starlib::stringify_utf32(codepoint);
    }

    void virtual_keyboard_input::press_key(const i32 key)
    {
        key_map[key] = 1;
    }

    void virtual_keyboard_input::release_key(const i32 key)
    {
        key_map[key] = -1;
    }

    bool keyboard_input::is_pressed(const i32 key)
    {
        return key_map[key] > 0;
    }

    bool keyboard_input::is_released(const i32 key)
    {
        return key_map[key] < 0;
    }

    bool keyboard_input::was_pressed_this_frame(const i32 key)
    {
        return get_frame_count(key) == 1;
    }

    bool keyboard_input::was_released_this_frame(const i32 key)
    {
        return get_frame_count(key) == -1;
    }

    i32 keyboard_input::get_frame_count(const i32 key)
    {
        return key_map[key];
    }

    i32 keyboard_input::get_pressed_frames(const i32 key)
    {
        return std::max(0, get_frame_count(key));
    }

    i32 keyboard_input::get_released_frames(const i32 key)
    {
        return std::max(0, -get_frame_count(key));
    }

    std::string keyboard_input::get_typed_text() const
    {
        return current_text;
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
    }
}
