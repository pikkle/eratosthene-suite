#include <iostream>
#include <stdexcept>
#include <cstdint>
#include <vector>
#include <chrono>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>

#include "engine.h"


const char* SHADER_VERT_FILE = "shaders/shader.vert.spv";
const char* SHADER_FRAG_FILE = "shaders/shader.frag.spv";

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


/* ----------- Vulkan setup methods ------------ */

VkInstance Er_vk_engine::er_instance = nullptr;
VkPhysicalDevice Er_vk_engine::er_phys_device = nullptr;
const size_t Er_vk_engine::er_imagedata_size = sizeof(uint8_t) * 4 * WIDTH * HEIGHT;


Er_vk_engine::Er_vk_engine() {
    if (!Er_vk_engine::er_instance && !er_phys_device) {
        create_instance();
        create_phys_device();
    }
#ifdef DEBUG
    setup_debugger();
#endif
    create_device();
    create_command_pool();
    bind_data();
    create_attachments();
    create_render_pass();
    create_pipeline();
    create_descriptor_set();
    create_command_buffers();
}

Er_vk_engine::~Er_vk_engine() {
    // TODO: free up all vulkan objects
}


void Er_vk_engine::create_instance() {
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Eratosthene-stream",
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

void Er_vk_engine::create_phys_device() {
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
}

void Er_vk_engine::setup_debugger() {
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

void Er_vk_engine::create_device() {
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

void Er_vk_engine::create_command_pool() {
    VkCommandPoolCreateInfo cmdPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = er_graphics_queue_family_index,
    };
    TEST_VK_ASSERT(vkCreateCommandPool(er_device, &cmdPoolInfo, nullptr, &er_graphics_command_pool), "error while creating graphics command pool");
    cmdPoolInfo.queueFamilyIndex = er_transfer_queue_family_index;
    TEST_VK_ASSERT(vkCreateCommandPool(er_device, &cmdPoolInfo, nullptr, &er_transfer_command_pool), "error while creating graphics command pool");
}

void Er_vk_engine::bind_data() {
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
                  &stagingWrap, triangleBufferSize, (void *) debug_triangles.data());
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

void Er_vk_engine::create_attachments() {
    // Color attachment
    create_attachment(
            er_color_attachment,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            er_color_format,
            VK_IMAGE_ASPECT_COLOR_BIT
    );

    // Depth attachment
    er_depth_format = find_supported_format(
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

void Er_vk_engine::create_render_pass() {
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

void Er_vk_engine::create_pipeline() {
    VkDescriptorSetLayoutBinding uboLayoutBinding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr,
    };
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &uboLayoutBinding,
    };
    TEST_VK_ASSERT(vkCreateDescriptorSetLayout(er_device, &layoutInfo, nullptr, &er_descriptor_set_layout), "failed to create descriptor set layout!");

    VkPushConstantRange pushConstantRange = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(glm::mat4),
    };
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &er_descriptor_set_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange,
    };
    TEST_VK_ASSERT(vkCreatePipelineLayout(er_device, &pipelineLayoutCreateInfo, nullptr, &er_pipeline_layout), "error while creating pipeline layout");

    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    TEST_VK_ASSERT(vkCreatePipelineCache(er_device, &pipelineCacheCreateInfo, nullptr, &er_pipeline_cache), "error while creating pipeline cache");

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .flags = 0,
        .primitiveRestartEnable = VK_FALSE,
    };
    VkPipelineRasterizationStateCreateInfo rasterizationState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_FRONT_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0f,
    };
    VkPipelineColorBlendAttachmentState blendAttachmentState = {
        .blendEnable = VK_FALSE,
        .colorWriteMask = 0xf,
    };
    VkPipelineColorBlendStateCreateInfo colorBlendState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &blendAttachmentState,
    };
    VkPipelineDepthStencilStateCreateInfo depthStencilState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .back = {
                .compareOp = VK_COMPARE_OP_ALWAYS, },
    };
    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .flags = 0,
        .viewportCount = 1,
        .scissorCount = 1,
    };
    VkPipelineMultisampleStateCreateInfo multisampleState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .flags = 0,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size()),
        .pDynamicStates = dynamicStateEnables.data(),
    };

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
        VkPipelineShaderStageCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = create_shader_module(readFile(SHADER_VERT_FILE)),
            .pName = "main",
        },
        VkPipelineShaderStageCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = create_shader_module(readFile(SHADER_FRAG_FILE)),
            .pName = "main",
        }
    };

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescription = Vertex::getAttributeDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size()),
        .pVertexAttributeDescriptions = attributeDescription.data(),
    };

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .flags = 0,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState = &multisampleState,
        .pDepthStencilState = &depthStencilState,
        .pColorBlendState = &colorBlendState,
        .pDynamicState = &dynamicState,
        .layout = er_pipeline_layout,
        .renderPass = er_render_pass,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    TEST_VK_ASSERT(vkCreateGraphicsPipelines(er_device, er_pipeline_cache, 1, &pipelineCreateInfo, nullptr, &er_pipeline_triangles), "error while creating triangles pipeline");

    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    TEST_VK_ASSERT(vkCreateGraphicsPipelines(er_device, er_pipeline_cache, 1, &pipelineCreateInfo, nullptr, &er_pipeline_lines), "error while creating lines pipeline");

    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    TEST_VK_ASSERT(vkCreateGraphicsPipelines(er_device, er_pipeline_cache, 1, &pipelineCreateInfo, nullptr, &er_pipeline_points), "error while creating points pipeline");

    for (auto stage : shaderStages) {
        vkDestroyShaderModule(er_device, stage.module, nullptr);
    }
}

void Er_vk_engine::create_command_buffers() {
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = er_graphics_command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    TEST_VK_ASSERT(vkAllocateCommandBuffers(er_device, &allocInfo, &er_command_buffer), "failed to allocate command buffers!");

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    TEST_VK_ASSERT(vkBeginCommandBuffer(er_command_buffer, &beginInfo), "failed to begin recording command buffer!");

    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };
    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = er_render_pass,
        .framebuffer = er_framebuffer,
        .renderArea = {
            .offset = {0, 0},
            .extent = {WIDTH, HEIGHT},},
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data(),
    };

    VkBuffer vertexBuffers[] = {er_vertices_buffer.buf};
    VkDeviceSize offsets[] = {0};
    vkCmdBeginRenderPass(er_command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {
        .width = (float) WIDTH,
        .height = (float) HEIGHT,
        .minDepth = (float)0.0f,
        .maxDepth = (float)1.0f,
    };
    vkCmdSetViewport(er_command_buffer, 0, 1, &viewport);
    VkRect2D scissor = {.extent = {WIDTH, HEIGHT},};

    vkCmdSetScissor(er_command_buffer, 0, 1, &scissor);
    vkCmdBindDescriptorSets(er_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, er_pipeline_layout, 0, 1, &er_descriptor_set, 0, nullptr);
    vkCmdBindVertexBuffers(er_command_buffer, 0, 1, vertexBuffers, offsets);

    vkCmdBindPipeline(er_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, er_pipeline_triangles);
    vkCmdBindIndexBuffer(er_command_buffer, er_triangles_buffer.buf, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(er_command_buffer, debug_triangles.size(), 1, 0, 0, 0);

    vkCmdBindPipeline(er_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, er_pipeline_lines);
    vkCmdBindIndexBuffer(er_command_buffer, er_lines_buffer.buf, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(er_command_buffer, debug_lines.size(), 1, 0, 0, 0);

    vkCmdBindPipeline(er_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, er_pipeline_points);
    vkCmdBindIndexBuffer(er_command_buffer, er_points_buffer.buf, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(er_command_buffer, debug_points.size(), 1, 0, 0, 0);

    vkCmdEndRenderPass(er_command_buffer);

    TEST_VK_ASSERT(vkEndCommandBuffer(er_command_buffer), "failed to record command buffer!");
}

void Er_vk_engine::create_descriptor_set() {
    VkDescriptorPoolSize poolSize = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
    };
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize,
    };
    TEST_VK_ASSERT(vkCreateDescriptorPool(er_device, &poolInfo, nullptr, &er_descriptor_pool), "failed to create descriptor pool!");

    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = er_descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &er_descriptor_set_layout,
    };
    TEST_VK_ASSERT(vkAllocateDescriptorSets(er_device, &allocInfo, &er_descriptor_set), "failed to allocate descriptor sets!");

    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    create_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            &er_uniform_buffer, bufferSize);

    VkDescriptorBufferInfo bufferInfo = {
        .buffer = er_uniform_buffer.buf,
        .offset = 0,
        .range = sizeof(UniformBufferObject),
    };

    VkWriteDescriptorSet descriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = er_descriptor_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &bufferInfo,
    };

    vkUpdateDescriptorSets(er_device, 1, &descriptorWrite, 0, nullptr);
}

/* -------- End of vulkan setup methods ------- */


/* --------- Vulkan rendering methods --------- */

void Er_vk_engine::update_uniform_buffers() {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo = {
        .model = glm::rotate(glm::mat4(1.0f), (time+1.f) * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        .view = view,
        .proj = glm::perspective(glm::radians(45.0f), WIDTH / (float) HEIGHT, 0.1f, 256.0f),
    };
    ubo.proj[1][1] *= -1;

    void *data;
    vkMapMemory(er_device, er_uniform_buffer.mem, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(er_device, er_uniform_buffer.mem);
}

void Er_vk_engine::draw_frame(char* imagedata, VkSubresourceLayout subresourceLayout) {
    update_uniform_buffers();
    submit_work(er_command_buffer, er_graphics_queue);
    vkDeviceWaitIdle(er_device);
    output_result(imagedata, subresourceLayout);
}

void Er_vk_engine::output_result(char* imagedata, VkSubresourceLayout subresourceLayout) {
    VkImage copyImage;
    VkMemoryRequirements memRequirements;
    VkDeviceMemory dstImageMemory;
    VkCommandBuffer copyCmd;

    VkImageCreateInfo imageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = er_color_format,
        .extent = {
                .width = WIDTH,
                .height = HEIGHT,
                .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_LINEAR,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    TEST_VK_ASSERT(vkCreateImage(er_device, &imageCreateInfo, nullptr, &copyImage), "error while creating copy image");

    vkGetImageMemoryRequirements(er_device, copyImage, &memRequirements);
    VkMemoryAllocateInfo memAllocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = get_memtype_index(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    };

    TEST_VK_ASSERT(vkAllocateMemory(er_device, &memAllocInfo, nullptr, &dstImageMemory), "error while allocating memory for copy image");
    TEST_VK_ASSERT(vkBindImageMemory(er_device, copyImage, dstImageMemory, 0), "error while binding memory to copy image");

    VkCommandBufferAllocateInfo cmdBufAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = er_transfer_command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    TEST_VK_ASSERT(vkAllocateCommandBuffers(er_device, &cmdBufAllocateInfo, &copyCmd), "error while allocating command buffer for image copy");
    VkCommandBufferBeginInfo cmdBufInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    TEST_VK_ASSERT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo), "error while beginning command buffer for image copy");

    VkImageMemoryBarrier imageMemoryBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = copyImage,
        .subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };
    vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &imageMemoryBarrier);

    VkImageCopy imageCopyRegion = {
        .srcSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1,
        },
        .dstSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1,
        },
        .extent = {
            .width = WIDTH,
            .height = HEIGHT,
            .depth = 1,
        }
    };

    vkCmdCopyImage(copyCmd, er_color_attachment.img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   copyImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);

    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &imageMemoryBarrier);

    TEST_VK_ASSERT(vkEndCommandBuffer(copyCmd), "error while ending command buffers for image copy");
    submit_work(copyCmd, er_transfer_queue);

    VkImageSubresource subResource{VK_IMAGE_ASPECT_COLOR_BIT};

    vkGetImageSubresourceLayout(er_device, copyImage, &subResource, &subresourceLayout);

    char *tmpdata;
    vkMapMemory(er_device, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&tmpdata);
    memcpy(imagedata, tmpdata, er_imagedata_size);

    vkUnmapMemory(er_device, dstImageMemory);
    vkFreeMemory(er_device, dstImageMemory, nullptr);
    vkDestroyImage(er_device, copyImage, nullptr);

}

/* ----- End of vulkan rendering methods ------ */


/* --------------- Helper methods --------------- */

inline void Er_vk_engine::submit_work(VkCommandBuffer cmd, VkQueue queue) {
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

inline uint32_t Er_vk_engine::get_memtype_index(uint32_t typeBits, VkMemoryPropertyFlags properties) {
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

inline void Er_vk_engine::create_buffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, BufferWrap *wrap, VkDeviceSize size, void *data) {
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

inline void Er_vk_engine::bind_memory(VkDeviceSize dataSize, BufferWrap &stagingWrap, BufferWrap &destWrap) {
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

inline VkFormat Er_vk_engine::find_supported_format(const std::vector<VkFormat> &candidates, VkFormatFeatureFlags features) {
    for (auto& format : candidates) {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(er_phys_device, format, &formatProps);
        if (formatProps.optimalTilingFeatures & features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}

inline void Er_vk_engine::create_attachment(Attachment &att, VkImageUsageFlags imgUsage, VkFormat format, VkImageAspectFlags aspect) {
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

inline VkShaderModule Er_vk_engine::create_shader_module(const std::vector<char> &code) {
    VkShaderModuleCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.size(),
            .pCode = reinterpret_cast<const uint32_t *>(code.data()),
    };
    VkShaderModule shaderModule;
    TEST_VK_ASSERT(vkCreateShaderModule(er_device, &createInfo, nullptr, &shaderModule),
                   "failed to create shader module!");
    return shaderModule;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Er_vk_engine::debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
                                                     uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData) {
    fprintf(stderr, "[VALIDATION]: %s - %s\n", pLayerPrefix, pMessage);
    return VK_FALSE;
}

/* ----------- End of helper methods ----------- */
