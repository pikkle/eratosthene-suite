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

/* --------------- Debug data --------------- */

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
/* ------------- End of debug data ------------- */

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
    create_command_pool();
    bind_data();
    create_attachments();
    create_render_pass();
//    create_pipeline();
//    create_descriptor_layout();
//    create_graphics_pipeline();
//    create_transfer_pipeline();
//    create_depth_resources();
//    create_uniform_buffer();
//    create_descriptors();
//    create_command_buffers();
}

/* --------------- Helper methods --------------- */

inline void submit_work(VkCommandBuffer cmd, VkQueue queue) {
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
    TEST_VK_ASSERT(vkCreateFence(er_device, &fenceInfo, nullptr, &fence), "error while creating fence");
    TEST_VK_ASSERT(vkQueueSubmit(queue, 1, &submitInfo, fence), "error while submitting to queue");
    TEST_VK_ASSERT(vkWaitForFences(er_device, 1, &fence, VK_TRUE, UINT64_MAX), "error while waiting for queue submission fences");
    vkDestroyFence(er_device, fence, nullptr);
}

inline uint32_t get_memtype_index(uint32_t typeBits, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(er_phys_device, &deviceMemoryProperties);
    for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
        if ((typeBits & 1) == 1 && (deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
        typeBits >>= 1;
    }
    return 0;
}

inline void create_buffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, BufferWrap *wrap, VkDeviceSize size, void *data = nullptr) {
    VkBufferCreateInfo bufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usageFlags,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    TEST_VK_ASSERT(vkCreateBuffer(er_device, &bufferCreateInfo, nullptr, &wrap->buf), "error while creating buffer");

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(er_device, wrap->buf, &memReqs);
    VkMemoryAllocateInfo memAlloc = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memReqs.size,
            .memoryTypeIndex = get_memtype_index(memReqs.memoryTypeBits, memoryPropertyFlags),
    };
    TEST_VK_ASSERT(vkAllocateMemory(er_device, &memAlloc, nullptr, &wrap->mem), "error while allocating memory to buffer");

    if (data != nullptr) {
        void *mapped;
        TEST_VK_ASSERT(vkMapMemory(er_device, wrap->mem, 0, size, 0, &mapped), "error while maping memory");
        memcpy(mapped, data, size);
        vkUnmapMemory(er_device, wrap->mem);
    }

    TEST_VK_ASSERT(vkBindBufferMemory(er_device, wrap->buf, wrap->mem, 0), "error while binding buffer memory");
}

inline void bind_memory(VkDeviceSize dataSize, BufferWrap &stagingWrap, BufferWrap &destWrap) {
    VkCommandBufferAllocateInfo cmdBufAllocateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = er_transfer_command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
    };
    VkCommandBuffer copyCmd;
    TEST_VK_ASSERT(vkAllocateCommandBuffers(er_device, &cmdBufAllocateInfo, &copyCmd), "error while allocating command buffers");
    VkCommandBufferBeginInfo cmdBufInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    TEST_VK_ASSERT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo), "error while starting command buffer");
    VkBufferCopy copyRegion = {
            .size = dataSize,
    };
    vkCmdCopyBuffer(copyCmd, stagingWrap.buf, destWrap.buf, 1, &copyRegion);
    TEST_VK_ASSERT(vkEndCommandBuffer(copyCmd), "error while terminating command buffer");
    submit_work(copyCmd, er_transfer_queue);

    vkDestroyBuffer(er_device, stagingWrap.buf, nullptr);
    vkFreeMemory(er_device, stagingWrap.mem, nullptr);
}

inline VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkFormatFeatureFlags features) {
    for (auto& format : candidates) {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(er_phys_device, format, &formatProps);
        if (formatProps.optimalTilingFeatures & features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}

inline void create_attachment(Attachment &att, VkImageUsageFlags imgUsage, VkFormat format, VkImageAspectFlags aspect) {
    VkImageCreateInfo imageInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = format,
            .extent = {
                    .width = WIDTH,
                    .height = HEIGHT,
                    .depth = 1,},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = imgUsage,
    };
    VkMemoryRequirements memReqs;
    TEST_VK_ASSERT(vkCreateImage(er_device, &imageInfo, nullptr, &att.img), "error while creating image");
    vkGetImageMemoryRequirements(er_device, att.img, &memReqs);
    VkMemoryAllocateInfo memAlloc = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memReqs.size,
            .memoryTypeIndex = get_memtype_index(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };
    TEST_VK_ASSERT(vkAllocateMemory(er_device, &memAlloc, nullptr, &att.mem), "error while allocating attachment image memory");
    TEST_VK_ASSERT(vkBindImageMemory(er_device, att.img, att.mem, 0), "error while binding attachment image to memory");

    VkImageViewCreateInfo viewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = att.img,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = format,
            .subresourceRange = {
                    .aspectMask = aspect,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,},
    };
    TEST_VK_ASSERT(vkCreateImageView(er_device, &viewInfo, nullptr, &att.view), "error while creating attachment view");
}

/* ----------- End of helper methods ----------- */

/* ----------- Vulkan setup methods ------------ */

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
    TEST_ASSERT(check_validation_layers_support(validation_layers),
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

    TEST_VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &er_instance),
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
    TEST_VK_ASSERT(vkCreateDebugReportCallbackEXT(er_instance, &debugReportCreateInfo, nullptr, &er_debug_report),
                   "error while creating debug reporter");
}

void create_device() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(er_instance, &deviceCount, nullptr);
    TEST_ASSERT(deviceCount > 0, "failed to find GPUs with Vulkan support!");

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
    TEST_ASSERT(er_phys_device != VK_NULL_HANDLE, "failed to find a suitable GPU!");

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
            continue;
        }
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT && !has_tq) {
            er_transfer_queue_family_index = i;
            transferQueueInfo = {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .queueFamilyIndex = i,
                    .queueCount = 1,
                    .pQueuePriorities = &defaultQueuePriority,
            };
            has_tq = true;
            continue;
        }
    }
    std::vector<VkDeviceQueueCreateInfo> queuesCreateInfos = {graphicsQueueInfo, transferQueueInfo};
    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 2,
        .pQueueCreateInfos = queuesCreateInfos.data(),
    };

    TEST_VK_ASSERT(vkCreateDevice(er_phys_device, &deviceCreateInfo, nullptr, &er_device),
                   "failed to create logical device!");

    vkGetDeviceQueue(er_device, er_graphics_queue_family_index, 0, &er_graphics_queue);
    vkGetDeviceQueue(er_device, er_transfer_queue_family_index, 0, &er_transfer_queue);
}

void create_command_pool() {
    VkCommandPoolCreateInfo cmdPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = er_graphics_queue_family_index,
    };
    TEST_VK_ASSERT(vkCreateCommandPool(er_device, &cmdPoolInfo, nullptr, &er_graphics_command_pool), "error while creating graphics command pool");
    cmdPoolInfo.queueFamilyIndex = er_transfer_queue_family_index;
    TEST_VK_ASSERT(vkCreateCommandPool(er_device, &cmdPoolInfo, nullptr, &er_transfer_command_pool), "error while creating graphics command pool");
}

void bind_data() {
    BufferWrap stagingWrap;
    VkDeviceSize vertexBufferSize = debug_vertices.size() * sizeof(Vertex);
    VkDeviceSize triangleBufferSize = debug_triangles.size() * sizeof(uint16_t);
    VkDeviceSize lineBufferSize = debug_lines.size() * sizeof(uint16_t);
    VkDeviceSize pointBufferSize = debug_points.size() * sizeof(uint16_t);

    // Vertices
    create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &stagingWrap, vertexBufferSize, (void *) debug_vertices.data());
    create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  &er_vertices_buffer, vertexBufferSize);
    bind_memory(vertexBufferSize, stagingWrap, er_vertices_buffer);


    // Triangles
    create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &stagingWrap, triangleBufferSize, (void *) debug_vertices.data());
    create_buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  &er_triangles_buffer, triangleBufferSize);
    bind_memory(triangleBufferSize, stagingWrap, er_triangles_buffer);

    // Lines
    create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &stagingWrap, lineBufferSize, (void *) debug_lines.data());
    create_buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  &er_lines_buffer, lineBufferSize);
    bind_memory(lineBufferSize, stagingWrap, er_lines_buffer);

    // Points
    create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &stagingWrap, pointBufferSize, (void *) debug_points.data());
    create_buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  &er_points_buffer, pointBufferSize);
    bind_memory(pointBufferSize, stagingWrap, er_points_buffer);
}

void create_attachments() {
    // Color attachment
    create_attachment(
            er_color_attachment,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            er_color_format,
            VK_IMAGE_ASPECT_COLOR_BIT
    );

    // Depth attachment
    er_depth_format = findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT,
             VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM},
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
    create_attachment(
            er_depth_attachment,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            er_depth_format,
            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
    );

}

void create_render_pass() {
    std::array<VkAttachmentDescription, 2> attchmentDescriptions = {
        // Color attachment
        VkAttachmentDescription {
            .format = er_color_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        },
        // Depth attachment
        VkAttachmentDescription {
            .format = er_depth_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
    };
    VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpassDescription = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorReference,
        .pDepthStencilAttachment = &depthReference,
    };

    std::array<VkSubpassDependency, 2> dependencies = {
            VkSubpassDependency {
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                .srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
            },
            VkSubpassDependency {
                .srcSubpass = 0,
                .dstSubpass = VK_SUBPASS_EXTERNAL,
                .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
                .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
            },
    };
    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size()),
        .pAttachments = attchmentDescriptions.data(),
        .subpassCount = 1,
        .pSubpasses = &subpassDescription,
        .dependencyCount = static_cast<uint32_t>(dependencies.size()),
        .pDependencies = dependencies.data(),
    };
    TEST_VK_ASSERT(vkCreateRenderPass(er_device, &renderPassInfo, nullptr, &er_render_pass), "error while creating render pass");
    VkImageView attachments[2] = {er_color_attachment.view, er_depth_attachment.view};

    VkFramebufferCreateInfo framebufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = er_render_pass,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .width = WIDTH,
        .height = HEIGHT,
        .layers = 1,
    };
    TEST_VK_ASSERT(vkCreateFramebuffer(er_device, &framebufferCreateInfo, nullptr, &er_framebuffer), "error while creating framebuffer");
}

/* -------- End of vulkan setup methods ------- */