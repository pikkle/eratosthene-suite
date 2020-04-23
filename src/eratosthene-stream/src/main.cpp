#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstdint>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "main.h"
#include "models.h"
#include "utils.h"

const int WIDTH = 800;
const int HEIGHT = 600;
const float FPS = 60.f;

const std::vector<const char *> validation_layers = {
        "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char *> extensions = {
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
};

/* DEBUG DATA */
glm::mat4x4 view = glm::lookAt(
        glm::vec3(2.0f, 2.0f, 2.0f), // eye
        glm::vec3(0.0f, 0.0f, 0.0f), // center
        glm::vec3(0.0f, 0.0f, 1.0f) // up
);

const std::vector<Vertex> debug_vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},

        {{-0.6f, -0.6f, -0.6f}, {0.0f, 0.0f, 1.0f}},
        {{0.6f, 0.6f, 0.6f}, {0.0f, 1.0f, 0.0f}},

        {{0.7f, 0.7f, 0.7f}, {1.0f, 1.0f, 1.0f}},
};

const std::vector<uint16_t> debug_triangles = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
};

const std::vector<uint16_t> debug_lines = {
        8, 9,
};

const std::vector<uint16_t> debug_points = {
        10,
};
/* END OF DEBUG DATA */

int main() {
    init();
    try {
        while (true) {
//            draw_frame();
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void init() {
    create_instance();
#ifdef DEBUG
    setup_debugger();
#endif
    create_device();
//    create_command_pool();
//    create_pipeline();
//    create_attachment();
//    create_render_pass();
//    create_descriptor_layout();
//    create_graphics_pipeline();
//    create_transfer_pipeline();
//    create_depth_resources();
//    create_vertex_buffer();
//    create_index_buffer();
//    create_uniform_buffer();
//    create_descriptors();
//    create_command_buffers();
}

void create_instance() {
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };

#ifdef DEBUG
    TEST_RESULT(check_validation_layers_support(validation_layers),
                 "validation layers requested, but not available!");
#endif

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .pApplicationInfo = &appInfo,
#ifdef DEBUG
        .enabledLayerCount = static_cast<uint32_t>(validation_layers.size()),
        .ppEnabledLayerNames = validation_layers.data(),
#else
        .enabledLayerCount = 0,
#endif
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    TEST_VK_RESULT(vkCreateInstance(&createInfo, nullptr, &er_instance),
            "failed to create instance!");
}

void setup_debugger() {
    VkDebugReportCallbackCreateInfoEXT debugReportCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
        .flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,
        .pfnCallback = (PFN_vkDebugReportCallbackEXT) debug_callback,
    };
    auto vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
            vkGetInstanceProcAddr(er_instance, "vkCreateDebugReportCallbackEXT"));
    TEST_VK_RESULT(vkCreateDebugReportCallbackEXT(er_instance, &debugReportCreateInfo, nullptr, &er_debug_report),
            "error while creating debug reporter");
}

void create_device() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(er_instance, &deviceCount, nullptr);
    TEST_RESULT(deviceCount > 0, "failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(er_instance, &deviceCount, devices.data());

    for (auto& device : devices) {
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
        // @TODO @FUTURE: check extensions support (e.g. VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME)
        if (supportedFeatures.samplerAnisotropy) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            printf("GPU selected: %s\n", deviceProperties.deviceName);
            er_phys_device = device;
            break;
        }
    }
    TEST_RESULT(er_phys_device != VK_NULL_HANDLE, "failed to find a suitable GPU!");

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(er_phys_device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(er_phys_device, &queueFamilyCount, queueFamilyProperties.data());

    const float defaultQueuePriority(0.0f);
    VkDeviceQueueCreateInfo graphicsQueueInfo, transferQueueInfo;
    bool has_gq = false, has_tq = false;

    for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++) {
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && !has_gq) {
            er_graphics_queue_family_index = i;
            graphicsQueueInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = i,
                .queueCount = 1,
                .pQueuePriorities = &defaultQueuePriority,
            };
            has_gq = true;
        }
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT && !has_tq) {
            er_transfer_queue_family_index = i;
            transferQueueInfo = {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .queueFamilyIndex = i,
                    .queueCount = 1,
                    .pQueuePriorities = &defaultQueuePriority,
            };
            has_gq = true;
        }
    }
    std::vector<VkDeviceQueueCreateInfo> queuesCreateInfos = {graphicsQueueInfo, transferQueueInfo};
    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 2,
        .pQueueCreateInfos = queuesCreateInfos.data(),
    };

    TEST_VK_RESULT(vkCreateDevice(er_phys_device, &deviceCreateInfo, nullptr, &er_device),
            "failed to create logical device!");

    vkGetDeviceQueue(er_device, er_graphics_queue_family_index, 0, &er_graphics_queue);
    vkGetDeviceQueue(er_device, er_transfer_queue_family_index, 0, &er_transfer_queue);
}

void submit_work(VkCommandBuffer cmd, VkQueue queue) {
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
    };
    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = 0,
    };
    VkFence fence;
    TEST_VK_RESULT(vkCreateFence(er_device, &fenceInfo, nullptr, &fence), "error while creating fence");
    TEST_VK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence), "error while submitting to queue");
    TEST_VK_RESULT(vkWaitForFences(er_device, 1, &fence, VK_TRUE, UINT64_MAX), "error while waiting for queue submission fences");
    vkDestroyFence(er_device, fence, nullptr);
}
