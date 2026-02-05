//
// Created by Bowl on 03/02/2026.
//

#ifndef STARDRAW_DEVICE_H
#define STARDRAW_DEVICE_H
#pragma once

#include "simple_window.hpp"
#include <vulkan/vulkan.h>

#include <string>
#include <vector>
#include <stdexcept>

namespace stardraw::vk13 {

    struct swap_chain_support_details {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    struct queue_family_indices {
        uint32_t graphics_family = 0;
        uint32_t present_family = 0;
        bool graphics_family_has_value = false;
        bool present_family_has_value = false;
        [[nodiscard]] inline bool is_complete() const {return graphics_family_has_value && present_family_has_value;}
    };

    class Device {
    public:
#ifdef NDEBUG
        const bool enable_validation_layers = true;
#else
        const bool enable_validation_layers = true;
#endif

        Device(simple_window&);
        ~Device();

        Device(const Device &) = delete;
        Device &operator=(const Device &) = delete;
        Device (Device &&) = delete;
        Device &operator=(Device &&) = delete;

        [[nodiscard]] VkCommandPool getCommandPool() const {return command_pool;}
        [[nodiscard]] VkDevice get_device() const {return device;}
        [[nodiscard]] VkPhysicalDevice get_physical_device() {return physical_device;}
        [[nodiscard]] VkSurfaceKHR get_surface() const { return surface; }
        [[nodiscard]] VkQueue get_graphics_queue() const { return graphics_queue; }
        [[nodiscard]] VkQueue get_present_queue() const { return present_queue; }

        swap_chain_support_details get_swap_chain_support() {return query_swap_chain_support(physical_device);}
        uint32_t find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        queue_family_indices find_physical_queue_families() {return find_queue_families(physical_device);}
        VkFormat find_supported_format(
            const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

        void create_buffer(
            VkDeviceSize size,
            VkBufferUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkBuffer& buffer,
            VkDeviceMemory& buffer_memory);
        VkCommandBuffer begin_single_time_commands();
        void end_single_time_commands(VkCommandBuffer command_buffer);
        void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
        void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layter_count);

        void create_image_with_info(
            const VkImageCreateInfo& create_info,
            VkMemoryPropertyFlags properties,
            VkImage& image,
            VkDeviceMemory& image_memory);

        VkPhysicalDeviceProperties properties;


    private:
        void create_instance();
        void setup_debug_messenger();
        void create_surface();
        void pick_physical_device();
        void create_logical_device();
        void create_command_pool();

        bool is_device_suitable(VkPhysicalDevice device);
        std::vector<const char*> get_required_extensions();
        bool check_validation_layer_support();
        queue_family_indices find_queue_families(VkPhysicalDevice device);
        void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info);
        void has_glfw_required_instance_extensions();
        bool check_device_extension_support(VkPhysicalDevice device);
        swap_chain_support_details query_swap_chain_support(VkPhysicalDevice device);

        VkInstance instance;
        VkDebugUtilsMessengerEXT debug_messenger;
        VkPhysicalDevice physical_device = VK_NULL_HANDLE;
        simple_window window;
        VkCommandPool command_pool;

        VkDevice device;
        VkSurfaceKHR surface;
        VkQueue graphics_queue;
        VkQueue present_queue;


        const std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation" };
        const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    };
}

#endif //STARDRAW_DEVICE_H