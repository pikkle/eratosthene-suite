#include <iostream>
#include <stdexcept>
#include <cstdint>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include "video-engine.h"


const char* SHADER_VERT_FILE = "shaders/shader.vert.spv";
const char* SHADER_FRAG_FILE = "shaders/shader.frag.spv";

const std::vector<const char *> validation_layers = {
        "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char *> extensions = {
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
};

/* ----------- Vulkan setup methods ------------ */

VkInstance VideoEngine::vk_instance = nullptr;
VkPhysicalDevice VideoEngine::vk_phys_device = nullptr;


VideoEngine::VideoEngine(std::shared_ptr<er_model_t> model, std::shared_ptr<er_view_t> view) : cl_model(model), cl_view(view) {
    er_imagedata_size = sizeof(uint8_t) * 4 * cl_width * cl_height;
    if (!VideoEngine::vk_instance && !vk_phys_device) {
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

    cl_displayed_state.running = true;

    std::thread cl_data_updater([this](){
        while (this->cl_displayed_state.running) {
            this->update_internal_data();
        }
    });
    cl_data_updater.detach();
}

VideoEngine::~VideoEngine() {
    // @TODO: free up all vulkan objects
    std::cerr << "Freeing up a engine instance..." << std::endl;
    cl_displayed_state.running = false;
    vkFreeCommandBuffers(vk_device, vk_graphics_command_pool, 1, &vk_draw_command_buffer);
    vkFreeCommandBuffers(vk_device, vk_transfer_command_pool, 1, &vk_copy_command_buffer);
    vkDestroyCommandPool(vk_device, vk_graphics_command_pool, nullptr);
    vkDestroyCommandPool(vk_device, vk_transfer_command_pool, nullptr);
    vkDestroyCommandPool(vk_device, vk_binding_command_pool, nullptr);
}

void VideoEngine::create_instance() {
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

    TEST_VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &vk_instance),
                   "failed to create instance!");
}

void VideoEngine::create_phys_device() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vk_instance, &deviceCount, nullptr);
    TEST_ASSERT(deviceCount > 0, "Failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vk_instance, &deviceCount, devices.data());

    for (auto& device : devices) {
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
        // @TODO @FUTURE: check extensions support (e.g. VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME)
        if (supportedFeatures.samplerAnisotropy && supportedFeatures.vertexPipelineStoresAndAtomics) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            std::cout << "GPU selected: " << deviceProperties.deviceName << " [" << deviceProperties.deviceID << "]" << std::endl;
            std::cout << "API Version: " << deviceProperties.apiVersion << std::endl;
            std::cout << "Driver Version: " << deviceProperties.driverVersion << std::endl;
//            std::cout << "Buffer max storage: " << deviceProperties.limits.maxStorageBufferRange << std::endl;

            vk_phys_device = device;
            break;
        }
    }
    TEST_ASSERT(vk_phys_device != VK_NULL_HANDLE, "Failed to find a suitable GPU!");
}

void VideoEngine::setup_debugger() {
    VkDebugReportCallbackCreateInfoEXT debugReportCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
        .flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,
        .pfnCallback = (PFN_vkDebugReportCallbackEXT) debug_callback,
    };
    auto vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
            vkGetInstanceProcAddr(vk_instance, "vkCreateDebugReportCallbackEXT"));
    TEST_VK_ASSERT(vkCreateDebugReportCallbackEXT(vk_instance, &debugReportCreateInfo, nullptr, &vk_debug_report),
                   "error while creating debug reporter");

}

void VideoEngine::create_device() {
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(vk_phys_device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(vk_phys_device, &queueFamilyCount, queueFamilyProperties.data());

    const float defaultQueuePriority(0.0f);
    VkDeviceQueueCreateInfo graphicsQueueInfo, transferQueueInfo;
    graphicsQueueInfo = transferQueueInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pQueuePriorities = &defaultQueuePriority,
    };
    bool has_gq = false, has_tq = false;

    for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++) {
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && !(has_gq)) {
            graphicsQueueInfo.queueFamilyIndex = vk_graphics_queue_family_index = i;
            graphicsQueueInfo.queueCount = 1,
            has_gq = true;
            continue;
        }
        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT && !(has_tq)) {
            transferQueueInfo.queueFamilyIndex = vk_transfer_queue_family_index = i;
            transferQueueInfo.queueCount = 2; // we need two queues of transfer, one for data binding to the gpu and one to retrieve rendered image data
            // @TODO @SUPPORT in case of support for only one transfer queue, we need to change the behaviour of the whole engine
            has_tq = true;
            continue;
        }
    }
    std::vector<VkDeviceQueueCreateInfo> queuesCreateInfos = {graphicsQueueInfo, transferQueueInfo};

    VkPhysicalDeviceFeatures features {
        .vertexPipelineStoresAndAtomics = true,
        .shaderFloat64 = true,
    };

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(queuesCreateInfos.size()),
        .pQueueCreateInfos = queuesCreateInfos.data(),
        .pEnabledFeatures = &features,
    };

    TEST_VK_ASSERT(vkCreateDevice(vk_phys_device, &deviceCreateInfo, nullptr, &vk_device),
                   "failed to create logical device!");

    vkGetDeviceQueue(vk_device, vk_graphics_queue_family_index, 0, &vk_graphics_queue);
    vkGetDeviceQueue(vk_device, vk_transfer_queue_family_index, 0, &vk_transfer_queue);
    vkGetDeviceQueue(vk_device, vk_transfer_queue_family_index, 1, &vk_binding_queue);
}

void VideoEngine::create_command_pool() {
    VkCommandPoolCreateInfo cmdPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = vk_graphics_queue_family_index,
    };
    TEST_VK_ASSERT(vkCreateCommandPool(vk_device, &cmdPoolInfo, nullptr, &vk_graphics_command_pool), "error while creating graphics command pool");
    cmdPoolInfo.queueFamilyIndex = vk_transfer_queue_family_index;
    TEST_VK_ASSERT(vkCreateCommandPool(vk_device, &cmdPoolInfo, nullptr, &vk_transfer_command_pool), "error while creating transfer command pool");
    TEST_VK_ASSERT(vkCreateCommandPool(vk_device, &cmdPoolInfo, nullptr, &vk_binding_command_pool), "error while creating binding command pool");
}

void VideoEngine::update_internal_data() {
    // @TODO @OPTIM refresh internal data whenever there is a change in the model
    mx_draw.lock();
    size_t count = dt_vertices.size();
    fill_data();
    if (count != dt_vertices.size()) {
        bind_data();
        create_pipeline();
        create_command_buffers();
    }
    mx_draw.unlock();
}

void VideoEngine::set_size(uint32_t width, uint32_t height) {
    mx_draw.lock();
    mx_bind.lock();
    mx_copy.lock();

    this->cl_width = width;
    this->cl_height = height;
    er_imagedata_size = sizeof(uint8_t) * 4 * cl_width * cl_height;

    recreate_chain();

    mx_draw.unlock();
    mx_bind.unlock();
    mx_copy.unlock();
}

void VideoEngine::fill_data() {
    // check that there is new data to be displayed (vertices count is an overall good estimate to know if vertices are already stored)
    size_t model_vert_count = 0;
    for (size_t i = 0; i < cl_model->md_size; i++) {
        auto cell = cl_model->md_cell + i;
        model_vert_count += er_cell_get_record(cell);
    }
    if (dt_vertices.size() == model_vert_count) return;

    dt_vertices.clear();
    dt_points.clear();
    dt_lines.clear();
    dt_triangles.clear();

    uint32_t point_id = 0;
    for (uint32_t cell_id = 0; cell_id < cl_model->md_size; ++cell_id) {
        auto cell = cl_model->md_cell + cell_id;
        auto base = le_array_get_byte(&cell->ce_data);

        // @TODO @OPTIM find a way to avoid copying vertices data and use directly models' stack mem to push onto gpu (need to memcpy with strided data)
        // iterate over data in the cell
        for (le_size_t i = 0; i < er_cell_get_record(cell); ++i) {
            // main pointer on the vertex, and points directly to the vertex coordinates
            auto *p_data = reinterpret_cast<double *>(base);

            // points to the color values
            le_byte_t *c_data = base + LE_ARRAY_DATA_POSE + LE_ARRAY_DATA_TYPE;

            // gather vertex data
            auto pos = glm::vec3(p_data[2], p_data[0], p_data[1]);
            auto color = glm::vec3(c_data[0], c_data[1], c_data[2]) / 255.f; // color representation from 8bit integer to [0,1] float

            // copy vertex to another data structure
            dt_vertices.push_back(Vertex{pos, color});
            dt_points.push_back(point_id++);
            // @TODO @FEATURE parse cell->ce_type[1] and [2] to construct lines and triangles instead of points

            base += LE_ARRAY_DATA;
        }
    }
    cl_displayed_state.vertices_count = dt_vertices.size();
}

void VideoEngine::bind_data() {
    BufferWrap stagingWrap;
    VkDeviceSize vertexBufferSize = dt_vertices.size() * sizeof(Vertex);
    VkDeviceSize triangleBufferSize = dt_triangles.size() * sizeof(uint32_t);
    VkDeviceSize lineBufferSize = dt_lines.size() * sizeof(uint32_t);
    VkDeviceSize pointBufferSize = dt_points.size() * sizeof(uint32_t);

    // Vertices
    if (!dt_vertices.empty()) {
        create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      &stagingWrap, vertexBufferSize, (void *) dt_vertices.data());
        delete_buffer(&vk_vertices_buffer);
        create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      &vk_vertices_buffer, vertexBufferSize);
        bind_memory(vertexBufferSize, stagingWrap, vk_vertices_buffer);
    }


    // Triangles
    if (!dt_triangles.empty()) {
        create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      &stagingWrap, triangleBufferSize, (void *) dt_triangles.data());
        delete_buffer(&vk_triangles_buffer);
        create_buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      &vk_triangles_buffer, triangleBufferSize);
        bind_memory(triangleBufferSize, stagingWrap, vk_triangles_buffer);
    }

    // Lines
    if (!dt_lines.empty()) {
        create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      &stagingWrap, lineBufferSize, (void *) dt_lines.data());
        delete_buffer(&vk_lines_buffer);
        create_buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      &vk_lines_buffer, lineBufferSize);
        bind_memory(lineBufferSize, stagingWrap, vk_lines_buffer);
    }

    // Points
    if (!dt_points.empty()) {
        create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      &stagingWrap, pointBufferSize, (void *) dt_points.data());
        delete_buffer(&vk_points_buffer);
        create_buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      &vk_points_buffer, pointBufferSize);
        bind_memory(pointBufferSize, stagingWrap, vk_points_buffer);
    }
}

void VideoEngine::create_attachments() {
    // Color attachment
    create_attachment(
            vk_color_attachment,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            vk_color_format,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_TILING_OPTIMAL
    );

    // Depth attachment
    vk_depth_format = find_supported_format(
            {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT,
             VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM},
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
    create_attachment(
            vk_depth_attachment,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            vk_depth_format,
            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            VK_IMAGE_TILING_OPTIMAL
    );

    // Copy image attachment
    create_attachment(
            vk_copy_attachment,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            vk_color_format,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VK_IMAGE_TILING_LINEAR,
            false
        );

}

void VideoEngine::create_render_pass() {
    std::array<VkAttachmentDescription, 2> attachmentDescriptions = {
        // Color attachment
        VkAttachmentDescription {
            .format = vk_color_format,
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
            .format = vk_depth_format,
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
        .attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size()),
        .pAttachments = attachmentDescriptions.data(),
        .subpassCount = 1,
        .pSubpasses = &subpassDescription,
        .dependencyCount = static_cast<uint32_t>(dependencies.size()),
        .pDependencies = dependencies.data(),
    };
    TEST_VK_ASSERT(vkCreateRenderPass(vk_device, &renderPassInfo, nullptr, &vk_render_pass), "error while creating render pass");
    VkImageView attachments[2] = {vk_color_attachment.view, vk_depth_attachment.view};

    VkFramebufferCreateInfo framebufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = vk_render_pass,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .width = cl_width,
        .height = cl_height,
        .layers = 1,
    };
    TEST_VK_ASSERT(vkCreateFramebuffer(vk_device, &framebufferCreateInfo, nullptr, &vk_framebuffer), "error while creating framebuffer");
}

void VideoEngine::create_pipeline() {
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {
            {
                    .binding = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .pImmutableSamplers = nullptr,
            },
    };
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
        .pBindings = layoutBindings.data(),
    };
    TEST_VK_ASSERT(vkCreateDescriptorSetLayout(vk_device, &layoutInfo, nullptr, &vk_descriptor_set_layout), "failed to create descriptor set layout!");

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &vk_descriptor_set_layout,
    };
    TEST_VK_ASSERT(vkCreatePipelineLayout(vk_device, &pipelineLayoutCreateInfo, nullptr, &vk_pipeline_layout), "error while creating pipeline layout");

    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    TEST_VK_ASSERT(vkCreatePipelineCache(vk_device, &pipelineCacheCreateInfo, nullptr, &vk_pipeline_cache), "error while creating pipeline cache");

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
        .cullMode = VK_CULL_MODE_NONE,
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
        .layout = vk_pipeline_layout,
        .renderPass = vk_render_pass,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    if (!dt_triangles.empty()) {
        inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        TEST_VK_ASSERT(vkCreateGraphicsPipelines(vk_device, vk_pipeline_cache, 1, &pipelineCreateInfo, nullptr,
                                                 &vk_pipeline_triangles), "error while creating triangles pipeline");
    }
    if (!dt_lines.empty()) {
        inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        TEST_VK_ASSERT(vkCreateGraphicsPipelines(vk_device, vk_pipeline_cache, 1, &pipelineCreateInfo, nullptr,
                                                 &vk_pipeline_lines), "error while creating lines pipeline");
    }
    if (!dt_points.empty()) {
        inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        TEST_VK_ASSERT(vkCreateGraphicsPipelines(vk_device, vk_pipeline_cache, 1, &pipelineCreateInfo, nullptr,
                                                 &vk_pipeline_points), "error while creating points pipeline");
    }

    for (auto stage : shaderStages) {
        vkDestroyShaderModule(vk_device, stage.module, nullptr);
    }
}

void VideoEngine::create_command_buffers() {
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vk_graphics_command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    TEST_VK_ASSERT(vkAllocateCommandBuffers(vk_device, &allocInfo, &vk_draw_command_buffer), "failed to allocate drawing command buffers!");
    allocInfo.commandPool = vk_transfer_command_pool;
    TEST_VK_ASSERT(vkAllocateCommandBuffers(vk_device, &allocInfo, &vk_copy_command_buffer), "failed to allocate transfer command buffers!");
    allocInfo.commandPool = vk_binding_command_pool;
    TEST_VK_ASSERT(vkAllocateCommandBuffers(vk_device, &allocInfo, &vk_binding_command_buffer), "failed to allocate binding command buffers!");

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    TEST_VK_ASSERT(vkBeginCommandBuffer(vk_draw_command_buffer, &beginInfo), "failed to begin recording command buffer!");

    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = { 0.f, 0.f, 0.f, 1.f };
    clearValues[1].depthStencil = { 1.0f, 0 };
    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = vk_render_pass,
        .framebuffer = vk_framebuffer,
        .renderArea = {
            .offset = {0, 0},
            .extent = {cl_width, cl_height},},
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data(),
    };

    VkBuffer vertexBuffers[] = {vk_vertices_buffer.buf};
    VkDeviceSize offsets[] = {0};
    vkCmdBeginRenderPass(vk_draw_command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {
        .width = (float) cl_width,
        .height = (float) cl_height,
        .minDepth = (float)0.0f,
        .maxDepth = (float)1.0f,
    };
    vkCmdSetViewport(vk_draw_command_buffer, 0, 1, &viewport);
    VkRect2D scissor = {.extent = {cl_width, cl_height},};

    if (!dt_vertices.empty()) {
        vkCmdSetScissor(vk_draw_command_buffer, 0, 1, &scissor);
        vkCmdBindDescriptorSets(vk_draw_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_layout, 0, 1, &vk_descriptor_set, 0, nullptr);
        vkCmdBindVertexBuffers(vk_draw_command_buffer, 0, 1, vertexBuffers, offsets);
    }
    if (!dt_triangles.empty()) {
        vkCmdBindPipeline(vk_draw_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_triangles);
        vkCmdBindIndexBuffer(vk_draw_command_buffer, vk_triangles_buffer.buf, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(vk_draw_command_buffer, dt_triangles.size(), 1, 0, 0, 0);
    }
    if (!dt_lines.empty()) {
        vkCmdBindPipeline(vk_draw_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_lines);
        vkCmdBindIndexBuffer(vk_draw_command_buffer, vk_lines_buffer.buf, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(vk_draw_command_buffer, dt_lines.size(), 1, 0, 0, 0);
    }
    if (!dt_points.empty()) {
        vkCmdBindPipeline(vk_draw_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline_points);
        vkCmdBindIndexBuffer(vk_draw_command_buffer, vk_points_buffer.buf, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(vk_draw_command_buffer, dt_points.size(), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(vk_draw_command_buffer);

    TEST_VK_ASSERT(vkEndCommandBuffer(vk_draw_command_buffer), "failed to record command buffer!");
}

void VideoEngine::create_descriptor_set() {
    std::vector<VkDescriptorPoolSize> poolSizes = {
            {
                    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
            },
    };
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };
    TEST_VK_ASSERT(vkCreateDescriptorPool(vk_device, &poolInfo, nullptr, &vk_descriptor_pool), "failed to create descriptor pool!");

    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = vk_descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &vk_descriptor_set_layout,
    };
    TEST_VK_ASSERT(vkAllocateDescriptorSets(vk_device, &allocInfo, &vk_descriptor_set), "failed to allocate descriptor sets!");

    VkDeviceSize uniformBufferSize = sizeof(MatrixBuffer);

    create_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &vk_uniform_buffer, uniformBufferSize);

    VkDescriptorBufferInfo uniformBufferInfo = {
            .buffer = vk_uniform_buffer.buf,
            .offset = 0,
            .range = uniformBufferSize,
    };

    std::vector<VkWriteDescriptorSet> descriptorWrites = {
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = vk_descriptor_set,
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &uniformBufferInfo,
            },
    };

    vkUpdateDescriptorSets(vk_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void VideoEngine::recreate_chain() {
    vkDeviceWaitIdle(vk_device);

    cleanup_chain();

    create_attachments();
    create_render_pass();
    create_pipeline();
    create_descriptor_set();
    create_command_buffers();
}

void VideoEngine::cleanup_chain() {
    vkDestroyImageView(vk_device, vk_depth_attachment.view, nullptr);
    vkDestroyImage(vk_device, vk_depth_attachment.img, nullptr);
    vkFreeMemory(vk_device, vk_depth_attachment.mem, nullptr);

    vkDestroyFramebuffer(vk_device, vk_framebuffer, nullptr);

    vkFreeCommandBuffers(vk_device, vk_binding_command_pool, 1, &vk_binding_command_buffer);
    vkFreeCommandBuffers(vk_device, vk_graphics_command_pool, 1, &vk_draw_command_buffer);
    vkFreeCommandBuffers(vk_device, vk_transfer_command_pool, 1, &vk_binding_command_buffer);

    vkDestroyPipeline(vk_device, vk_pipeline_points, nullptr);
    vkDestroyPipeline(vk_device, vk_pipeline_lines, nullptr);
    vkDestroyPipeline(vk_device, vk_pipeline_triangles, nullptr);
    vkDestroyPipelineLayout(vk_device, vk_pipeline_layout, nullptr);
    vkDestroyRenderPass(vk_device, vk_render_pass, nullptr);

    vkDestroyImageView(vk_device, vk_color_attachment.view, nullptr);

    vkDestroyBuffer(vk_device, vk_uniform_buffer.buf, nullptr);
    vkFreeMemory(vk_device, vk_uniform_buffer.mem, nullptr);

    vkDestroyDescriptorPool(vk_device, vk_descriptor_pool, nullptr);
}

/* -------- End of vulkan setup methods ------- */


/* --------- Vulkan rendering methods --------- */

void VideoEngine::update_uniform_buffers() {
    // the eye vector corresponds to the camera position in the world space, which needs to be converted into cartesian
    // coordinates to compute the view transformation matrix (conversion between WGS -> cartesian system)
    // https://stackoverflow.com/a/1185413/2981017
    double lat = cl_view->vw_lat * LE_D2R;
    double lon = cl_view->vw_lon * LE_D2R;
    double cos_lat = cos(lat);
    auto eye = glm::vec3(
            cos_lat * cos(lon), // x = R * cos(lat) * cos(lon)
            cos_lat * sin(lon), // y = R * cos(lat) * sin(lon)
            sin(lat)                 // z = R * sin(lat)
            ) * (float) cl_view->vw_alt;

    // @TODO @FEATURE take into account the view angle of the view object (instead of defining the center point, calculate a forward vector using the angles)
    // https://learnopengl.com/Getting-started/Camera
    // look at the center of earth
    auto center = glm::vec3(0, 0, 0);
    auto forward = glm::normalize(center-eye);
    forward = glm::rotate(glm::mat4(1), (float) cl_view->vw_azm, glm::vec3(0.0, 0.0, 1.0)) * glm::vec4(forward, 1.0);

    auto right = glm::cross(glm::vec3(0, 0, 1), forward);
    auto up = glm::cross(forward, right);

    float zNear = 0.01f;
    float zFar = cl_view->vw_alt;

    auto model = glm::mat4(1); // for now we don't need to modify the earth model (translation, rotation, scaling)
    auto view = glm::lookAt(eye, center, up);
    auto projection = glm::perspective(
            glm::radians(30.0f),
            cl_width / (float) cl_height,
            zNear,
            zFar
    );

    /* GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted, this is
    a compensation to render the image in the correct orientation */
    projection[0][0] *= -1;

    MatrixBuffer mvp_buffer = {model, view, projection};
    void *vp_data;
    TEST_VK_ASSERT(vkMapMemory(vk_device, vk_uniform_buffer.mem, 0, sizeof(mvp_buffer), 0, &vp_data), "error while mapping transformation buffer memory");
    memcpy(vp_data, &mvp_buffer, sizeof(mvp_buffer));
    vkUnmapMemory(vk_device, vk_uniform_buffer.mem);

}

void VideoEngine::draw_frame(char* imagedata, VkSubresourceLayout subresourceLayout) {
    mx_draw.lock_shared();
    if (!er_view_get_equal(&*this->cl_view, &cl_displayed_state.previous_view)) {
        cl_displayed_state.previous_view = *cl_view;
        update_uniform_buffers();
    }
    submit_work(vk_draw_command_buffer, vk_graphics_queue);
    mx_draw.unlock_shared();
    output_result(imagedata, subresourceLayout);
}

void VideoEngine::output_result(char* imagedata, VkSubresourceLayout subresourceLayout) {
    mx_copy.lock_shared();
    VkCommandBufferBeginInfo cmdBufInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
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
                    .width = cl_width,
                    .height = cl_height,
                    .depth = 1,
            }
    };
    VkImageMemoryBarrier imageMemoryBarrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = vk_copy_attachment.img,
            .subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };
    TEST_VK_ASSERT(vkResetCommandBuffer(vk_copy_command_buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT), "error while resetting command buffer for image copy");
    TEST_VK_ASSERT(vkBeginCommandBuffer(vk_copy_command_buffer, &cmdBufInfo), "error while beginning command buffer for image copy");
    vkCmdPipelineBarrier(vk_copy_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &imageMemoryBarrier);
    vkCmdCopyImage(vk_copy_command_buffer, vk_color_attachment.img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   vk_copy_attachment.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    vkCmdPipelineBarrier(vk_copy_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &imageMemoryBarrier);

    TEST_VK_ASSERT(vkEndCommandBuffer(vk_copy_command_buffer), "error while ending command buffers for image copy");
    submit_work(vk_copy_command_buffer, vk_transfer_queue);

    VkImageSubresource subResource{VK_IMAGE_ASPECT_COLOR_BIT};

    vkGetImageSubresourceLayout(vk_device, vk_copy_attachment.img, &subResource, &subresourceLayout);

    char *tmpdata;
    vkMapMemory(vk_device, vk_copy_attachment.mem, 0, er_imagedata_size, 0, (void**)&tmpdata);
    memcpy(imagedata, tmpdata, er_imagedata_size);
    vkUnmapMemory(vk_device, vk_copy_attachment.mem);
    mx_copy.unlock_shared();
}

/* ----- End of vulkan rendering methods ------ */


/* --------------- Helper methods --------------- */

inline void VideoEngine::submit_work(VkCommandBuffer cmd, VkQueue queue) {
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
    TEST_VK_ASSERT(vkCreateFence(vk_device, &fenceInfo, nullptr, &fence), "error while creating fence");
    TEST_VK_ASSERT(vkQueueSubmit(queue, 1, &submitInfo, fence), "error while submitting to queue");
    TEST_VK_ASSERT(vkWaitForFences(vk_device, 1, &fence, VK_TRUE, UINT64_MAX), "error while waiting for queue submission fences");
    vkDestroyFence(vk_device, fence, nullptr);
}

inline uint32_t VideoEngine::get_memtype_index(uint32_t typeBits, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(vk_phys_device, &deviceMemoryProperties);
    for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
        if ((typeBits & 1) == 1 && (deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
        typeBits >>= 1;
    }
    return 0;
}

inline void VideoEngine::delete_buffer(BufferWrap *wrap) {
    if (wrap->is_allocated) {
        vkDestroyBuffer(vk_device, wrap->buf, nullptr);
        vkFreeMemory(vk_device, wrap->mem, nullptr);
    }

    wrap->is_allocated = false;
}

inline void VideoEngine::create_buffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, BufferWrap *wrap, VkDeviceSize size, void *data) {
    VkBufferCreateInfo bufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = usageFlags,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    TEST_VK_ASSERT(vkCreateBuffer(vk_device, &bufferCreateInfo, nullptr, &wrap->buf), "error while creating buffer");

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(vk_device, wrap->buf, &memReqs);
    VkMemoryAllocateInfo memAlloc = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memReqs.size,
            .memoryTypeIndex = get_memtype_index(memReqs.memoryTypeBits, memoryPropertyFlags),
    };
    TEST_VK_ASSERT(vkAllocateMemory(vk_device, &memAlloc, nullptr, &wrap->mem), "error while allocating memory to buffer");

    if (data != nullptr) {
        void *mapped;
        TEST_VK_ASSERT(vkMapMemory(vk_device, wrap->mem, 0, size, 0, &mapped), "error while maping memory");
        memcpy(mapped, data, size);
        vkUnmapMemory(vk_device, wrap->mem);
    }

    TEST_VK_ASSERT(vkBindBufferMemory(vk_device, wrap->buf, wrap->mem, 0), "error while binding buffer memory");
    wrap->is_allocated = true;
}

inline void VideoEngine::bind_memory(VkDeviceSize dataSize, BufferWrap &stagingWrap, BufferWrap &destWrap) {
    mx_bind.lock();
    VkCommandBufferBeginInfo cmdBufInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    TEST_VK_ASSERT(vkResetCommandBuffer(vk_binding_command_buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT),
                   "error while resetting binding command buffer");
    TEST_VK_ASSERT(vkBeginCommandBuffer(vk_binding_command_buffer, &cmdBufInfo),
                   "error while starting binding command buffer");
    VkBufferCopy copyRegion = {
            .size = dataSize,
    };
    vkCmdCopyBuffer(vk_binding_command_buffer, stagingWrap.buf, destWrap.buf, 1, &copyRegion);
    TEST_VK_ASSERT(vkEndCommandBuffer(vk_binding_command_buffer),
                   "error while terminating command buffer");
    submit_work(vk_binding_command_buffer, vk_binding_queue);
    mx_bind.unlock();

    vkDestroyBuffer(vk_device, stagingWrap.buf, nullptr);
    vkFreeMemory(vk_device, stagingWrap.mem, nullptr);
}

inline VkFormat VideoEngine::find_supported_format(const std::vector<VkFormat> &candidates, VkFormatFeatureFlags features) {
    for (auto& format : candidates) {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(vk_phys_device, format, &formatProps);
        if (formatProps.optimalTilingFeatures & features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}

inline void VideoEngine::create_attachment(Attachment &att, VkImageUsageFlags imgUsage, VkFormat format, VkImageAspectFlags aspect, VkMemoryPropertyFlags memoryProperties, VkImageTiling tiling, bool createView) {
    VkImageCreateInfo imageInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = format,
            .extent = {
                    .width = cl_width,
                    .height = cl_height,
                    .depth = 1,},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = tiling,
            .usage = imgUsage,
    };
    VkMemoryRequirements memReqs;
    TEST_VK_ASSERT(vkCreateImage(vk_device, &imageInfo, nullptr, &att.img), "error while creating image");
    vkGetImageMemoryRequirements(vk_device, att.img, &memReqs);
    VkMemoryAllocateInfo memAlloc = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = memReqs.size,
            .memoryTypeIndex = get_memtype_index(memReqs.memoryTypeBits, memoryProperties),
    };
    TEST_VK_ASSERT(vkAllocateMemory(vk_device, &memAlloc, nullptr, &att.mem), "error while allocating attachment image memory");
    TEST_VK_ASSERT(vkBindImageMemory(vk_device, att.img, att.mem, 0), "error while binding attachment image to memory");

    if (createView) {
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
        TEST_VK_ASSERT(vkCreateImageView(vk_device, &viewInfo, nullptr, &att.view), "error while creating attachment view");
    }
}

inline VkShaderModule VideoEngine::create_shader_module(const std::vector<char> &code) {
    VkShaderModuleCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.size(),
            .pCode = reinterpret_cast<const uint32_t *>(code.data()),
    };
    VkShaderModule shaderModule;
    TEST_VK_ASSERT(vkCreateShaderModule(vk_device, &createInfo, nullptr, &shaderModule),
                   "failed to create shader module!");
    return shaderModule;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VideoEngine::debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
                                                           uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData) {
    fprintf(stderr, "[VALIDATION]: %s - %s\n", pLayerPrefix, pMessage);
    return VK_FALSE;
}

uint32_t VideoEngine::get_width() const {
    return cl_width;
}

uint32_t VideoEngine::get_height() const {
    return cl_height;
}

/* ----------- End of helper methods ----------- */
