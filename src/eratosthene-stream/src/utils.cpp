#include "utils.h"

#include <cstring>

VkResult create_debug(VkInstance &instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroy_debug(VkInstance &instance, VkDebugUtilsMessengerEXT &debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

bool check_validation_layers_support(const std::vector<const char *> &layers) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layer : layers) {
        bool layerFound = false;
        for (const auto &layerProperties : availableLayers) {
            if (strcmp(layer, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) return false;
    }
    return true;
}

std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

void encode_image(const char* data, size_t size, unsigned char *output) {
    uint32_t bmp_header_size = sizeof(char) * 14;
    uint32_t dib_header_size = sizeof(char) * 108;
    uint32_t total_size = (bmp_header_size + dib_header_size + size);
    output = (unsigned char*) malloc(sizeof(char) * total_size);
    output[0] = 0x42; output[1] = 0x4D;
    memcpy(&total_size, &output[2], 4);

}