//
// Created by Bowl on 04/02/2026.
//

#include "simple_window.hpp"

namespace stardraw::vk13 {

    simple_window::simple_window(int w, int h, std::string _name) : width{ w }, height{ h }, name{ _name } {
        init();
    }

    simple_window::~simple_window()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void simple_window::init()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
    }

    void simple_window::create_window_surface(VkInstance instance, VkSurfaceKHR* surface)
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface");
        }
    }

    void simple_window::framebuffer_resize_callback(GLFWwindow* window, int w, int h)
    {
        auto _window = reinterpret_cast<simple_window*>(glfwGetWindowUserPointer(window));
        _window->frame_buffer_resized = true;
        _window->width = w;
        _window->height = h;
    }
} // stardraw