#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "models.h"

#ifndef ERATOSTHENE_STREAM_HARDWARE_CPP_H
#define ERATOSTHENE_STREAM_HARDWARE_CPP_H

const std::vector<const char *> deviceExtensions = {
        // VK_KHR_SWAPCHAIN_EXTENSION_NAME
//        VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME
};

void pickPhysicalDevice(VkInstance& instance, VkPhysicalDevice& physicalDevice, VkSurfaceKHR &surface);
bool checkDeviceExtensionSupport(VkPhysicalDevice& device, std::vector<const char*> deviceExtensions);
bool isDeviceSuitable(VkPhysicalDevice &device, VkSurfaceKHR &surface);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice& device, VkSurfaceKHR& surface);
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice& device, VkSurfaceKHR& surface);
VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
VkExtent2D chooseSwapExtent(GLFWwindow *window, const VkSurfaceCapabilitiesKHR& capabilities);

#endif //ERATOSTHENE_STREAM_HARDWARE_CPP_H
