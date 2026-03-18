#pragma once
#include <map>
#include <string>

#include "starlib/math/glm.hpp"
#include "starlib/types/starlib_stdint.hpp"

namespace starwin
{
    using namespace starlib_stdint;

    class keyboard_input
    {
    public:
        [[nodiscard]] bool is_pressed(i32 key);
        [[nodiscard]] bool is_released(i32 key);
        [[nodiscard]] bool was_pressed_this_frame(i32 key);
        [[nodiscard]] bool was_released_this_frame(i32 key);

        [[nodiscard]] i32 get_frame_count(i32 key);
        [[nodiscard]] i32 get_pressed_frames(i32 key);
        [[nodiscard]] i32 get_released_frames(i32 key);

        [[nodiscard]] std::string get_typed_text() const;

    protected:
        std::map<i32, i32> key_map = std::map<i32, i32>();
        std::string current_text;
    };

    class virtual_keyboard_input : public keyboard_input
    {
    public:
        void type_character(u32 codepoint);
        void press_key(i32 key);
        void release_key(i32 key);

        void advance_frame();
    };

    class mouse_input
    {
    public:
        [[nodiscard]] bool is_clicked(i32 button);
        [[nodiscard]] bool is_released(i32 button);
        [[nodiscard]] bool was_clicked_this_frame(i32 button);
        [[nodiscard]] bool was_released_this_frame(i32 button);

        [[nodiscard]] i32 get_frame_count(i32 button);
        [[nodiscard]] i32 get_pressed_frames(i32 button);
        [[nodiscard]] i32 get_released_frames(i32 button);

        [[nodiscard]] glm::dvec2 get_position_absolute() const;
        [[nodiscard]] glm::dvec2 get_position_delta() const;
        [[nodiscard]] glm::dvec2 get_scroll_absolute() const;
        [[nodiscard]] glm::dvec2 get_scroll_delta() const;

    protected:
        std::map<i32, i32> button_map = std::map<i32, i32>();
        glm::dvec2 mouse_position = glm::dvec2();
        glm::dvec2 mouse_delta = glm::dvec2();
        glm::dvec2 absolute_scroll = glm::dvec2();
        glm::dvec2 delta_scroll = glm::dvec2();
    };

    class virtual_mouse_input : public mouse_input
    {
    public:
        void move_by(glm::dvec2 delta);
        void move_to(glm::dvec2 position);
        void scroll_by(glm::dvec2 delta);
        void scroll_to(glm::dvec2 scroll);
        void click(i32 button);
        void release(i32 button);

        void advance_frame();
    };

    class controller_input
    {
    public:
        enum class dpad
        {
            LEFT, RIGHT, UP, DOWN
        };

        [[nodiscard]] bool is_pressed(i32 button);
        [[nodiscard]] bool is_released(i32 button);
        [[nodiscard]] bool was_pressed_this_frame(i32 button);
        [[nodiscard]] bool was_released_this_frame(i32 button);

        [[nodiscard]] bool is_direction_pressed(i32 pad, dpad direction);
        [[nodiscard]] bool is_direction_pressed_this_frame(i32 pad, dpad direction);

        [[nodiscard]] f32 axis_value(i32 axis);
        [[nodiscard]] bool is_axis_pressed(i32 axis, f32 threshold);
        [[nodiscard]] bool is_axis_released(i32 axis, f32 threshold);

        [[nodiscard]] i32 get_frame_count(i32 button);
        [[nodiscard]] i32 get_pressed_frames(i32 button);
        [[nodiscard]] i32 get_released_frames(i32 button);

        [[nodiscard]] i32 get_direction_frame_count(i32 pad, dpad direction);
        [[nodiscard]] i32 get_direction_pressed_frames(i32 pad, dpad direction);
        [[nodiscard]] i32 get_direction_released_frames(i32 pad, dpad direction);

    protected:
        std::map<i32, i32> button_map = std::map<i32, i32>();
        std::map<i32, f32> axis_map = std::map<i32, f32>();
        std::map<i32, std::map<dpad, i32>> pad_map = std::map<i32, std::map<dpad, i32>>();
    };

    class virtual_controller_input : public controller_input
    {
    public:
        void press_button(i32 button);
        void release_button(i32 button);
        void set_axis(i32 axis, f32 value);

        void press_direction(i32 pad, dpad direction);
        void release_direction(i32 pad, dpad direction);

        void advance_frame();
    };
}
