//
// Created by Bowl on 03/02/2026.
//
//log of all exit conditions that are fatal:
// failed to create instance
// failed to enumerate physical devices
// failed to find a suitable physical device
// failed to create logical device
// failed to create command pool
// missing required GLFW extensions
// failed to find suitable memory types
// failed to create vertex buffer
// failed to allocate vertex buffer memory
// failed to create image
// failed to allocate image memory
// failed to bind image memory

//log of all exit conditions that are fatal on debug:
// failed to create debug messenger
// no validation layers

#include "Device.hpp"

#include <cstring>
#include <iostream>
#include <queue>
#include <set>
#include <unordered_set>
#include <stardraw/api/commands.hpp>

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    void* pUserData) {
    std::cerr << "Validiation layer: " << p_callback_data->pMessage << std::endl;
    return VK_FALSE;
}

VkResult create_debug_utils_messenger_EXT(
VkInstance instance,
const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
const VkAllocationCallbacks* p_allocator,
VkDebugUtilsMessengerEXT* debug_messenger) {
    auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance,
        "vkCreateDebugUtilsMessengerEXT");

    if (fn != nullptr) {
        return fn(instance, p_create_info, p_allocator, debug_messenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroy_debug_utils_messenger_EXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debug_messenger,
    const VkAllocationCallbacks* p_allocator) {
    auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance,
        "vkDestroyDebugUtilsMessengerEXT");
    if (fn != nullptr) {
        fn(instance, debug_messenger, p_allocator);
    }
}

namespace stardraw::vk13 {

    Device::Device(simple_window& _window) : window(_window) {
        create_instance();
        setup_debug_messenger();
        create_surface();
        pick_physical_device();
        create_logical_device();
        create_command_pool();
    }

    Device::~Device() {
        vkDestroyCommandPool(device, command_pool, nullptr);
        vkDestroyDevice(device, nullptr);

        if (enable_validation_layers) {
            destroy_debug_utils_messenger_EXT(instance, debug_messenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);
    }

    void Device::create_instance() {
        if (enable_validation_layers && !check_validation_layer_support()) {
            throw std::runtime_error("Validation layers requested, but not available");
        }

        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = "Stardraw";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "Hyengine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        auto ext = get_required_extensions();
        create_info.enabledExtensionCount = static_cast<uint32_t>(ext.size());
        create_info.ppEnabledExtensionNames = ext.data();

        VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};
        if (enable_validation_layers) {
            create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
            create_info.ppEnabledLayerNames = validation_layers.data();

            populate_debug_messenger_create_info(debug_create_info);
            create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
        }
        else {
            create_info.enabledLayerCount = 0;
            create_info.pNext = nullptr;
        }

        if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
        //has_glfw_required_instance_extentions();
    }

    void Device::pick_physical_device() {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
        if (device_count == 0) {
            throw std::runtime_error("failed to enumerate physical devices!");
        }

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

        for (const auto& d : devices) {
            if (is_device_suitable(d)) {
                physical_device = d;
                break;
            }
        }

        if (physical_device == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable physical device!");
        }

        vkGetPhysicalDeviceProperties(physical_device, &properties);
    }

    void Device::create_logical_device() {
        queue_family_indices indicies = find_queue_families(physical_device);

        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        std::set<uint32_t> unique_queue_families = {indicies.graphics_family, indicies.present_family};
        float queue_priority = 1.0f;
        for (uint32_t q : unique_queue_families) {
            VkDeviceQueueCreateInfo queue_create_info = {};
            queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = q;
            queue_create_info.queueCount = 1;
            queue_create_info.pQueuePriorities = &queue_priority;
            queue_create_infos.push_back(queue_create_info);
        }

        VkPhysicalDeviceFeatures features = {};
        features.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.pEnabledFeatures = &features;
        create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
        create_info.ppEnabledExtensionNames = device_extensions.data();

        if (enable_validation_layers) {
            create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
            create_info.ppEnabledLayerNames = validation_layers.data();
        }
        else {
            create_info.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

#ifndef NDEBUG
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(physical_device, &props);
        std::cout << "Physical limits of the push constant: " << props.limits.maxPushConstantsSize << "\n";
#endif
        vkGetDeviceQueue(device, indicies.graphics_family, 0, &graphics_queue);
        vkGetDeviceQueue(device, indicies.present_family, 0, &present_queue);
    }

    void Device::create_command_pool() {
        queue_family_indices indicies = find_physical_queue_families();

        VkCommandPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex = indicies.graphics_family;
        pool_info.flags =
            VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void Device::create_surface() {
        window.create_window_surface(instance, &surface);
    }

    bool Device::is_device_suitable(VkPhysicalDevice device) {
        queue_family_indices indicies = find_queue_families(device);
        bool ext_support = check_device_extension_support(device);

        bool adequate_swap_chain = false;
        if (ext_support) {
            swap_chain_support_details details = query_swap_chain_support(device);
            adequate_swap_chain = !details.formats.empty() && !details.present_modes.empty();
        }
        VkPhysicalDeviceFeatures supported_features = {};
        vkGetPhysicalDeviceFeatures(device, &supported_features);

        return indicies.is_complete() && ext_support && adequate_swap_chain &&
            supported_features.samplerAnisotropy;
    }

    void Device::populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT &info) {
        info = {};
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        info.pfnUserCallback = vulkan_debug_callback;
        info.pUserData = nullptr;  // Optional
    }

    void Device::setup_debug_messenger() {
        if (!enable_validation_layers) return;
        VkDebugUtilsMessengerCreateInfoEXT info;
        populate_debug_messenger_create_info(info);
        if (create_debug_utils_messenger_EXT(instance, &info, nullptr, &debug_messenger) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create debug messenger");
        }
    }

    bool Device::check_validation_layer_support() {
        uint32_t count;
        vkEnumerateInstanceLayerProperties(&count, nullptr);
        std::vector<VkLayerProperties> layers(count);
        vkEnumerateInstanceLayerProperties(&count, layers.data());
        for (const char* name : validation_layers) {
            bool found = false;
            for (const auto& layer : layers) {
                if (strcmp(name, layer.layerName) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }

        return true;
    }

    std::vector<const char *> Device::get_required_extensions() {
        uint32_t ext_count = 0;
        const char** extentions;
        extentions = glfwGetRequiredInstanceExtensions(&ext_count);
        std::vector<const char*> ext(extentions, extentions + ext_count);

        if (enable_validation_layers) {
            ext.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        return ext;
    }

    void Device::has_glfw_required_instance_extensions() {
        uint32_t ext_count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr);
        std::vector<VkExtensionProperties> extentions(ext_count);
        vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, extentions.data());
        std::cout << "available extensions: \n";
        std::unordered_set<std::string> available_extentions;
        for (const auto e : extentions) {
            std::cout << "\t" << e.extensionName << std::endl;
            available_extentions.insert(e.extensionName);
        }

        std::cout << "required extensions: \n";
        auto required_extensions = get_required_extensions();
        for (const auto& r : required_extensions) {
            std::cout << "\t" << r << std::endl;
            if (available_extentions.find(r) != available_extentions.end()) {
                throw std::runtime_error("Missing reuqired GLFW extension");
            }
        }
    }

    bool Device::check_device_extension_support(VkPhysicalDevice device) {
        uint32_t count;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(count);
        vkEnumerateDeviceExtensionProperties(
            device,
            nullptr,
            &count,
            available_extensions.data());

        std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

        for (const auto& extension : available_extensions) {
            required_extensions.erase(extension.extensionName);
        }

        return required_extensions.empty();
    }

    queue_family_indices Device::find_queue_families(VkPhysicalDevice device) {
        queue_family_indices indices;
        uint32_t count;

        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, queue_families.data());
        int i = 0;
        for (const auto& family : queue_families) {
            if (family.queueFlags > 0 && family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphics_family = i;
                indices.graphics_family_has_value = true;
            }
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);
            if (family.queueCount > 0 && present_support) {
                indices.present_family = i;
                indices.present_family_has_value = true;
            }

            if (indices.is_complete()) break;

            i++;
        }
        return indices;
    }

    swap_chain_support_details Device::query_swap_chain_support(VkPhysicalDevice device) {
        swap_chain_support_details details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);

        if (count != 0) {
            details.formats.resize(count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, details.formats.data());
        }

        count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);
        if (count != 0) {
            details.present_modes.resize(count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, details.present_modes.data());
        }

        return details;
    }

    VkFormat Device::find_supported_format(
        const std::vector<VkFormat> &candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features) {
        for (VkFormat fmt : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physical_device, fmt, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return fmt;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return fmt;
            }
        }
    }

    uint32_t Device::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties mem_properties;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);
        for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
            if ((type_filter & (1 << i)) &&
                (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory types");
    }

    void Device::create_buffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

VkCommandBuffer Device::begin_single_time_commands() {
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = command_pool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &beginInfo);
    return command_buffer;
}

void Device::end_single_time_commands(VkCommandBuffer command_buffer)
{
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &command_buffer;

    vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

void Device::copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = begin_single_time_commands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;  // Optional
    copyRegion.dstOffset = 0;  // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    end_single_time_commands(commandBuffer);
}

void Device::copy_buffer_to_image(
    VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount)
{
    VkCommandBuffer commandBuffer = begin_single_time_commands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = layerCount;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);
    end_single_time_commands(commandBuffer);
}

void Device::create_image_with_info(
    const VkImageCreateInfo& imageInfo,
    VkMemoryPropertyFlags properties,
    VkImage& image,
    VkDeviceMemory& image_memory)
{
    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &image_memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    if (vkBindImageMemory(device, image, image_memory, 0) != VK_SUCCESS) {
        throw std::runtime_error("failed to bind image memory!");
    }
}
}

