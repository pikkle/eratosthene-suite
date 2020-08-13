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
const size_t VideoEngine::er_imagedata_size = sizeof(uint8_t) * 4 * WIDTH * HEIGHT;


VideoEngine::VideoEngine(std::shared_ptr<er_model_t> model, std::shared_ptr<er_view_t> view)
        : cl_model(model), cl_view(view) {
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
}

VideoEngine::~VideoEngine() {
    // @TODO: free up all vulkan objects
    std::cerr << "Freeing up a engine instance..." << std::endl;
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
    TEST_ASSERT(deviceCount > 0, "failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vk_instance, &deviceCount, devices.data());

    for (auto& device : devices) {
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
        // @TODO @FUTURE: check extensions support (e.g. VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME)
        if (supportedFeatures.samplerAnisotropy && supportedFeatures.vertexPipelineStoresAndAtomics) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            printf("GPU selected: %s\n", deviceProperties.deviceName);
            vk_phys_device = device;
            break;
        }
    }
    TEST_ASSERT(vk_phys_device != VK_NULL_HANDLE, "failed to find a suitable GPU!");
}
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    std::cerr << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

void VideoEngine::setup_debugger() {
//    VkDebugReportCallbackCreateInfoEXT debugReportCreateInfo = {
//        .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
//        .flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,
//        .pfnCallback = (PFN_vkDebugReportCallbackEXT) debug_callback,
//    };
//    auto vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
//            vkGetInstanceProcAddr(vk_instance, "vkCreateDebugReportCallbackEXT"));
//    TEST_VK_ASSERT(vkCreateDebugReportCallbackEXT(vk_instance, &debugReportCreateInfo, nullptr, &vk_debug_report),
//                   "error while creating debug reporter");
    if (validation_layers.empty()) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debugCallback,
    };

    if (create_debug(vk_instance, &createInfo, nullptr, &vk_debug_messenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
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
            has_tq = true;
            continue;
        }
    }
    std::vector<VkDeviceQueueCreateInfo> queuesCreateInfos = {graphicsQueueInfo, transferQueueInfo};

    VkPhysicalDeviceFeatures features {
        .vertexPipelineStoresAndAtomics = true,
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
    fill_data();
    bind_data();
    create_render_pass();
    create_pipeline();
    create_descriptor_set();
    create_command_buffers();
}

void VideoEngine::fill_data() {
    // Temporarily reclean all the time data to be display
    // @TODO @OPTIM only remove data that is not anymore shown and only add fresh data
    // @TODO unmap memory that is removed on GPU !
    dt_vertices.clear();
    dt_points.clear();
    dt_lines.clear();
    dt_triangles.clear();

    /* angle variable */
    float er_lon = er_view_get_lon(&*cl_view);
    float er_lat = er_view_get_lat(&*cl_view);

    /* trigonometric variable */
    le_real_t er_cosl = cos(-er_lon * LE_D2R);
    le_real_t er_sinl = sin(-er_lon * LE_D2R);
    le_real_t er_cosa = cos(+er_lat * LE_D2R);
    le_real_t er_sina = sin(+er_lat * LE_D2R);

    dt_vertices.push_back(Vertex{
        .pos = {0, 10, 0},
        .color = {0, 1, 0},
        .cell_id = 0,
    });
    uint32_t point_id = 0;
    dt_points.push_back(point_id++);

    for (uint32_t cell_id = 0; cell_id < cl_model->md_size; ++cell_id) {
        auto cell = cl_model->md_cell + cell_id;
        auto base = le_array_get_byte(&cell->ce_data);

        // transformation matrix computation (compute only once)
        if (dt_transformations.size() < cl_model->md_size) {
            le_real_t er_trans[3] = {
                    cell->ce_edge[0] * er_cosl + cell->ce_edge[2] * er_sinl,
                    cell->ce_edge[0] * er_sinl - cell->ce_edge[2] * er_cosl,
            };
            /* compute cell translation */
            er_trans[2] = cell->ce_edge[1] * er_sina - er_trans[1] * er_cosa;
            er_trans[1] = cell->ce_edge[1] * er_cosa + er_trans[1] * er_sina;

            /* cell translation */
            auto transformation = glm::translate(
                    glm::mat4(1),
                    glm::vec3(er_trans[0], er_trans[1], er_trans[2])
            );

            /* cell rotation - planimetric rotation */
            transformation = glm::rotate(transformation, er_lat, glm::vec3{1.f, 0.f, 0.f});
            transformation = glm::rotate(transformation, -er_lon, glm::vec3{0.f, 1.f, 0.f});
            dt_transformations.push_back(transformation);

//            std::cout << "Transformation [" << cell_id << "] :" << glm::to_string(transformation) << std::endl;

        }

        /* iterate over data in the cell */
        for (le_size_t i = 0; i < le_array_get_size(&cell->ce_data) / LE_ARRAY_DATA; ++i) {
//            std::cout << "Cell[" << cell_id << "] size = " << le_array_get_size(&cell->ce_data) / LE_ARRAY_DATA << std::endl;
            // main pointer on the vertex, and points directly to the vertex coordinates
            le_real_t *p_data = reinterpret_cast<le_real_t *>(base);

            // points to the color values
            le_byte_t *c_data = base + LE_ARRAY_DATA_POSE + LE_ARRAY_DATA_TYPE;

            // gather vertex data
            dt_vertices.push_back(Vertex{
                    .pos =     {p_data[0], p_data[1], p_data[2]},
                    .color =   {(float) c_data[0] / 255.f, (float) c_data[1] / 255.f, (float) c_data[2] / 255.f},
                    .cell_id = cell_id,
            });
            dt_points.push_back(point_id++);

            glm::vec4 v = glm::vec4(p_data[0], p_data[1], p_data[2], 1.0);
//            std::cout << "v" << point_id << "[" << cell_id << "] : " << glm::to_string(v) << std::endl;
            auto trp = dt_transformations[0] * v;
//            std::cout << "v" << point_id << "[" << cell_id << "] : " << glm::to_string(trp) << std::endl;
            // @TODO: parse cell->ce_type[1] and [2] to construct lines and triangles instead of points

            base += LE_ARRAY_DATA;
        }
    }
}

void VideoEngine::bind_data() {
    BufferWrap stagingWrap;
    VkDeviceSize vertexBufferSize = dt_vertices.size() * sizeof(Vertex);
    VkDeviceSize triangleBufferSize = dt_triangles.size() * sizeof(uint32_t);
    VkDeviceSize lineBufferSize = dt_lines.size() * sizeof(uint32_t);
    VkDeviceSize pointBufferSize = dt_points.size() * sizeof(uint32_t);

    // Vertices
    if (!dt_vertices.empty()) {
//        std::cerr << "Loaded " << dt_vertices.size() << " vertices in gpu memory" << std::endl;
        create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      &stagingWrap, vertexBufferSize, (void *) dt_vertices.data());
        create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      &vk_vertices_buffer, vertexBufferSize);
        bind_memory(vertexBufferSize, stagingWrap, vk_vertices_buffer);
    }


    // Triangles
    if (!dt_triangles.empty()) {
        std::cerr << "Loaded " << dt_triangles.size() << " triangle indices in gpu memory" << std::endl;
        create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      &stagingWrap, triangleBufferSize, (void *) dt_triangles.data());
        create_buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      &vk_triangles_buffer, triangleBufferSize);
        bind_memory(triangleBufferSize, stagingWrap, vk_triangles_buffer);
    }

    // Lines
    if (!dt_lines.empty()) {
        std::cerr << "Loaded " << dt_lines.size() << " line indices in gpu memory" << std::endl;
        create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      &stagingWrap, lineBufferSize, (void *) dt_lines.data());
        create_buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                      &vk_lines_buffer, lineBufferSize);
        bind_memory(lineBufferSize, stagingWrap, vk_lines_buffer);
    }

    // Points
    if (!dt_points.empty()) {
//        std::cerr << "Loaded " << dt_points.size() << " point indices in gpu memory " << std::endl;
        create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      &stagingWrap, pointBufferSize, (void *) dt_points.data());
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
    std::array<VkAttachmentDescription, 2> attchmentDescriptions = {
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
        .attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size()),
        .pAttachments = attchmentDescriptions.data(),
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
        .width = WIDTH,
        .height = HEIGHT,
        .layers = 1,
    };
    TEST_VK_ASSERT(vkCreateFramebuffer(vk_device, &framebufferCreateInfo, nullptr, &vk_framebuffer), "error while creating framebuffer");
}

void VideoEngine::create_pipeline() {
    VkDescriptorSetLayoutBinding uboLayoutBinding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr,
    };
    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &uboLayoutBinding,
    };
    TEST_VK_ASSERT(vkCreateDescriptorSetLayout(vk_device, &layoutInfo, nullptr, &vk_descriptor_set_layout), "failed to create descriptor set layout!");

    VkPushConstantRange pushConstantRange = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(glm::mat4),
    };
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &vk_descriptor_set_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange,
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
    clearValues[0].color = { 1.0f, 1.0f, 1.0f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };
    VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = vk_render_pass,
        .framebuffer = vk_framebuffer,
        .renderArea = {
            .offset = {0, 0},
            .extent = {WIDTH, HEIGHT},},
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data(),
    };

    VkBuffer vertexBuffers[] = {vk_vertices_buffer.buf};
    VkDeviceSize offsets[] = {0};
    vkCmdBeginRenderPass(vk_draw_command_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {
        .width = (float) WIDTH,
        .height = (float) HEIGHT,
        .minDepth = (float)0.0f,
        .maxDepth = (float)1.0f,
    };
    vkCmdSetViewport(vk_draw_command_buffer, 0, 1, &viewport);
    VkRect2D scissor = {.extent = {WIDTH, HEIGHT},};

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
    VkDescriptorPoolSize poolSize = {
        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
    };
    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize,
    };
    TEST_VK_ASSERT(vkCreateDescriptorPool(vk_device, &poolInfo, nullptr, &vk_descriptor_pool), "failed to create descriptor pool!");

    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = vk_descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &vk_descriptor_set_layout,
    };
    TEST_VK_ASSERT(vkAllocateDescriptorSets(vk_device, &allocInfo, &vk_descriptor_set), "failed to allocate descriptor sets!");

    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    create_buffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &vk_uniform_buffer, bufferSize);

    VkDescriptorBufferInfo bufferInfo = {
        .buffer = vk_uniform_buffer.buf,
        .offset = 0,
        .range = sizeof(UniformBufferObject),
    };

    VkWriteDescriptorSet descriptorWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = vk_descriptor_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &bufferInfo,
    };

    vkUpdateDescriptorSets(vk_device, 1, &descriptorWrite, 0, nullptr);
}

/* -------- End of vulkan setup methods ------- */


/* --------- Vulkan rendering methods --------- */

void VideoEngine::update_uniform_buffers() {
    auto eye = glm::vec3(1.f, 1.f, 1.f);
    auto center = glm::vec3(0.0f, 0.0f, 0.f);

    auto altitude = er_view_get_alt(&*cl_view);
    auto gamma = er_view_get_gam(&*cl_view);
//    auto scale = er_geodesy_scale(altitude);
    auto scale = 0.0001f;
//    std::cout << "scale: " << scale << std::endl;

    float zNear = er_geodesy_near(altitude, scale);
    float zFar = er_geodesy_far(altitude, gamma, scale);

//    std::cout << "planes: " << zNear << ", " << zFar << std::endl;

//    auto view = glm::scale(
//            glm::lookAt(
//                    eye, // eye
//                    center, // center
//                    glm::vec3(0.0f, 0.0f, 1.0f) // up
//            ),
//            glm::vec3(0.001f)
//    );
    auto view = glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, scale));
    auto projection = glm::perspective(
            glm::radians(45.0f),
            (float) WIDTH / (float) HEIGHT,
            1.f,
            255.f
    );
    projection[1][1] *= -1;

if (! dt_vertices.empty()) {
//    std::cout << glm::to_string(view) << std::endl;
//    std::cout << glm::to_string(projection) << std::endl;
//    std::cout << glm::to_string(dt_transformations[0]) << std::endl;
//    glm::vec4 v = glm::vec4(dt_vertices[0].pos, 1.0);
//    std::cout << glm::to_string(v) << std::endl;
//    std::cout << glm::to_string(dt_transformations[0] * v) << std::endl;
//    std::cout << glm::to_string(view * projection * dt_transformations[0]) << std::endl;
//    std::cout << glm::to_string(projection * dt_transformations[0] * v) << std::endl;
//    std::cout << glm::to_string(view * projection * dt_transformations[0] * v) << std::endl;

}
    UniformBufferObject ubo = {
            .view = view,
            .proj = projection,
            .modelCount = static_cast<uint32_t>(dt_transformations.size()),
            .model = dt_transformations.data(),
    };

    void *data;
    vkMapMemory(vk_device, vk_uniform_buffer.mem, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(vk_device, vk_uniform_buffer.mem);
}

void VideoEngine::draw_frame(char* imagedata, VkSubresourceLayout subresourceLayout) {
    mx_draw.lock();
    update_uniform_buffers();
    submit_work(vk_draw_command_buffer, vk_graphics_queue);
    mx_draw.unlock();
    vkDeviceWaitIdle(vk_device);
    output_result(imagedata, subresourceLayout);
}

void VideoEngine::output_result(char* imagedata, VkSubresourceLayout subresourceLayout) {
    mx_copy.lock();
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
                    .width = WIDTH,
                    .height = HEIGHT,
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
    vkMapMemory(vk_device, vk_copy_attachment.mem, 0, VK_WHOLE_SIZE, 0, (void**)&tmpdata);
    memcpy(imagedata, tmpdata, er_imagedata_size);
    vkUnmapMemory(vk_device, vk_copy_attachment.mem);
    mx_copy.unlock();
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

    vkDestroyBuffer(vk_device, stagingWrap.buf, nullptr);
    vkFreeMemory(vk_device, stagingWrap.mem, nullptr);
    mx_bind.unlock();
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
                    .width = WIDTH,
                    .height = HEIGHT,
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

/* ----------- End of helper methods ----------- */
