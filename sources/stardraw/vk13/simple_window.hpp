//
// Created by Bowl on 04/02/2026.
//

#ifndef STARDRAW_SIMPLE_WINDOW_H
#define STARDRAW_SIMPLE_WINDOW_H
#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
#include <stdexcept>

namespace stardraw::vk13 {
    class simple_window {
    public:
        simple_window(int, int, std::string);
        ~simple_window();



        bool shouldClose() {return glfwWindowShouldClose(window);}
        VkExtent2D get_extent() {
            return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
        }

        void create_window_surface(VkInstance, VkSurfaceKHR*);
        bool was_window_resized() {return frame_buffer_resized;}
        void reset_resize_flag() {frame_buffer_resized = false;}

        GLFWwindow* get_window() {return window;}

    private:
        void init();
        static void framebuffer_resize_callback(GLFWwindow*, int, int);

        int width, height;
        bool frame_buffer_resized = false;

        std::string name;
        GLFWwindow* window;
    };
} // stardraw

#endif //STARDRAW_SIMPLE_WINDOW_H