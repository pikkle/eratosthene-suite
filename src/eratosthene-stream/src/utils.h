#ifndef ERATOSTHENE_STREAM_UTILS_H
#define ERATOSTHENE_STREAM_UTILS_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <stdexcept>
#include <fstream>
#include <iostream>

typedef uint32_t uint32;

#define DEBUG (!NDEBUG)

#define TEST_ASSERT(test_cmd, error_msg) if (!(test_cmd)) throw std::runtime_error(error_msg)
/*! test a vulkan command to return the code VK_SUCCESS, otherwise crashes with a given message */
#define TEST_VK_ASSERT(test_cmd, error_msg) TEST_ASSERT(test_cmd == VK_SUCCESS, error_msg)

VkResult create_debug(VkInstance &instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void destroy_debug(VkInstance &instance, VkDebugUtilsMessengerEXT &debugMessenger, const VkAllocationCallbacks* pAllocator);
bool check_validation_layers_support(const std::vector<const char *> &layers);
std::vector<char> readFile(const std::string& filename);

#endif //ERATOSTHENE_STREAM_UTILS_H
