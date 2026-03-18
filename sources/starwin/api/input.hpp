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
        [[nodiscard]] bool is_pressed(u32 id);
        [[nodiscard]] bool is_released(u32 id);
        [[nodiscard]] bool was_pressed_this_frame(u32 id);
        [[nodiscard]] bool was_released_this_frame(u32 id);

        ///Positive values indicate that the button is being held down, negative values indicate that it is released
        ///A zero value indicates an unknown state.
        [[nodiscard]] i32 get_frame_count(u32 id);
        [[nodiscard]] u32 get_pressed_frames(u32 id);
        [[nodiscard]] u32 get_released_frames(u32 id);

        [[nodiscard]] std::string get_typed_text() const;

        ///Negative values indicate no button was clicked/released this frame.
        [[nodiscard]] i32 get_last_pressed_this_frame() const;

        ///Negative values indicate no button was clicked/released this frame.
        [[nodiscard]] i32 get_last_released_this_frame() const;

    protected:
        std::map<i32, i32> key_map = std::map<i32, i32>();
        std::string current_text;
        i32 last_pressed_id = -1;
        i32 last_released_id = -1;
    };

    class virtual_keyboard_input : public keyboard_input
    {
    public:
        void type_character(u32 codepoint);
        void press_key(u32 id);
        void release_key(u32 id);

        void advance_frame();
    };

    class mouse_input
    {
    public:
        [[nodiscard]] bool is_clicked(u32 id);
        [[nodiscard]] bool is_released(u32 id);
        [[nodiscard]] bool was_clicked_this_frame(u32 id);
        [[nodiscard]] bool was_released_this_frame(u32 id);

        ///Positive values indicate that the button is being held down, negative values indicate that it is released
        ///A zero value indicates an unknown state.
        [[nodiscard]] i32 get_frame_count(u32 id);
        [[nodiscard]] u32 get_pressed_frames(u32 id);
        [[nodiscard]] u32 get_released_frames(u32 id);

        [[nodiscard]] glm::dvec2 get_position_absolute() const;
        [[nodiscard]] glm::dvec2 get_position_delta() const;
        [[nodiscard]] glm::dvec2 get_scroll_absolute() const;
        [[nodiscard]] glm::dvec2 get_scroll_delta() const;

        ///Negative values indicate no button was clicked/released this frame.
        [[nodiscard]] i32 get_last_clicked_this_frame() const;

        ///Negative values indicate no button was clicked/released this frame.
        [[nodiscard]] i32 get_last_released_this_frame() const;

    protected:
        std::map<i32, i32> button_map = std::map<i32, i32>();
        glm::dvec2 mouse_position = glm::dvec2();
        glm::dvec2 mouse_delta = glm::dvec2();
        glm::dvec2 absolute_scroll = glm::dvec2();
        glm::dvec2 delta_scroll = glm::dvec2();
        i32 last_clicked_id = -1;
        i32 last_released_id = -1;
    };

    class virtual_mouse_input : public mouse_input
    {
    public:
        void move_by(glm::dvec2 delta);
        void move_to(glm::dvec2 position);
        void scroll_by(glm::dvec2 delta);
        void scroll_to(glm::dvec2 scroll);
        void click(u32 id);
        void release(u32 id);

        void advance_frame();
    };

    class controller_input
    {
    public:
        [[nodiscard]] bool is_pressed(u32 id);
        [[nodiscard]] bool is_released(u32 id);
        [[nodiscard]] bool was_pressed_this_frame(u32 id);
        [[nodiscard]] bool was_released_this_frame(u32 id);

        [[nodiscard]] bool is_dpad_pressed(u32 id, u32 pad_index = 0);
        [[nodiscard]] bool is_dpad_released(u32 id, u32 pad_index = 0);
        [[nodiscard]] bool was_dpad_pressed_this_frame(u32 id, u32 pad_index = 0);
        [[nodiscard]] bool was_dpad_released_this_frame(u32 id, u32 pad_index = 0);

        [[nodiscard]] f32 axis_value(u32 id);
        [[nodiscard]] bool is_axis_pressed(u32 id);
        [[nodiscard]] bool is_axis_released(u32 id);
        [[nodiscard]] bool was_axis_pressed_this_frame(u32 id);
        [[nodiscard]] bool was_axis_released_this_frame(u32 id);

        [[nodiscard]] i32 get_frame_count(u32 id);
        [[nodiscard]] u32 get_pressed_frames(u32 id);
        [[nodiscard]] u32 get_released_frames(u32 id);

        [[nodiscard]] i32 get_axis_frame_count(u32 id);
        [[nodiscard]] u32 get_axis_pressed_frames(u32 id);
        [[nodiscard]] u32 get_axis_released_frames(u32 id);

        [[nodiscard]] i32 get_dpad_frame_count(u32 pad, u32 id);
        [[nodiscard]] u32 get_dpad_pressed_frames(u32 pad, u32 id);
        [[nodiscard]] u32 get_dpad_released_frames(u32 pad, u32 id);

        ///Negative values indicate no button was clicked/released this frame.
        [[nodiscard]] i32 get_last_pressed_this_frame() const;
        ///Negative values indicate no button was clicked/released this frame.
        [[nodiscard]] i32 get_last_released_this_frame() const;
        ///Negative values indicate no button was clicked/released this frame.
        ///The tuple contains both the dpad button ID and the dpad index.
        [[nodiscard]] std::tuple<i32, u32> get_last_pressed_dpad_this_frame() const;
        ///Negative values indicate no button was clicked/released this frame.
        ///The tuple contains both the dpad button ID and the dpad index.
        [[nodiscard]] std::tuple<i32, u32> get_last_released_dpad_this_frame() const;
        ///Negative values indicate no button was clicked/released this frame.
        [[nodiscard]] i32 get_last_pressed_axis_this_frame() const;
        ///Negative values indicate no button was clicked/released this frame.
        [[nodiscard]] i32 get_last_released_axis_this_frame() const;

        void set_axis_press_threshold(f32 threshold);

    protected:
        std::map<i32, i32> button_map = std::map<i32, i32>();
        std::map<i32, f32> axis_map = std::map<i32, f32>();
        std::map<i32, i32> axis_pressed_map = std::map<i32, i32>();
        f32 axis_pressed_threshold = 0.1f;
        std::map<i32, std::map<u32, i32>> pad_map = std::map<i32, std::map<u32, i32>>();

        i32 last_pressed_button = -1;
        i32 last_released_button = -1;
        i32 last_pressed_axis = -1;
        i32 last_released_axis = -1;
        std::tuple<i32, u32> last_pressed_dpad = {-1, 0};
        std::tuple<i32, u32> last_released_dpad = {-1, 0};
    };

    class virtual_controller_input : public controller_input
    {
    public:
        void press_button(u32 id);
        void release_button(u32 id);

        void press_dpad(u32 id, u32 pad_index = 0);
        void release_dpad(u32 id, u32 pad_index = 0);

        void set_axis(u32 id, f32 value);

        void advance_frame();
    };
}
