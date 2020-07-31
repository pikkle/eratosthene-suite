#ifndef ERATOSTHENE_STREAM_VIDEO_ENGINE_H
#define ERATOSTHENE_STREAM_VIDEO_ENGINE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <eratosthene-client-model.h>
#include <eratosthene-client-view.h>
#include <memory>

#include "models.h"
#include "utils.h"

using namespace StreamModels;
using namespace StreamUtils;

typedef std::vector<Vertex> Vertices;
typedef std::vector<uint32_t> Indices;
typedef std::vector<glm::mat4> Transformations;

// @TODO: change size depending on the client's resolution (with some max resolution TBD)
const int WIDTH = 1600;
const int HEIGHT = 1200;
const float FPS = 60.f; // @TODO: adapt the frames rendered based on target FPS

class VideoEngine {
public:
    VideoEngine(std::shared_ptr<er_model_t> model, std::shared_ptr<er_view_t> ptr);
    ~VideoEngine();
    void draw_frame(char *imagedata, VkSubresourceLayout subresourceLayout);

    static const size_t er_imagedata_size;

    void update_internal_data();

private:
    /* Shared vulkan objects among all engines running */
    static VkInstance vk_instance;
    static VkPhysicalDevice vk_phys_device;
    constexpr static const VkSurfaceKHR vk_surface = VK_NULL_HANDLE; // @FUTURE obtain a headless surface for swapchain rendering

    static void create_instance();
    static void create_phys_device();

    std::shared_ptr<er_model_t> cl_model;
    std::shared_ptr<er_view_t> cl_view;

    Transformations dt_transformations = {};
    Vertices dt_vertices = {};
    Indices dt_triangles = {};
    Indices dt_lines = {};
    Indices dt_points = {};

    VkDevice vk_device;
    VkDebugReportCallbackEXT vk_debug_report;
    uint32 vk_graphics_queue_family_index;
    uint32 vk_transfer_queue_family_index;
    VkQueue vk_graphics_queue;
    VkQueue vk_transfer_queue;
    VkCommandPool vk_graphics_command_pool;
    VkCommandPool vk_transfer_command_pool;
    VkFormat vk_color_format = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormat vk_depth_format;
    Attachment vk_color_attachment;
    Attachment vk_depth_attachment;
    VkRenderPass vk_render_pass;
    VkFramebuffer vk_framebuffer;
    VkDescriptorSetLayout vk_descriptor_set_layout;
    VkDescriptorPool vk_descriptor_pool;
    VkDescriptorSet vk_descriptor_set;
    VkPipeline vk_pipeline_triangles;
    VkPipeline vk_pipeline_lines;
    VkPipeline vk_pipeline_points;
    VkPipelineLayout vk_pipeline_layout;
    VkPipelineCache vk_pipeline_cache;
    VkCommandBuffer vk_command_buffer;
    BufferWrap vk_vertices_buffer;
    BufferWrap vk_triangles_buffer;
    BufferWrap vk_lines_buffer;
    BufferWrap vk_points_buffer;
    BufferWrap vk_uniform_buffer;

    void setup_debugger();
    void create_device();
    void create_command_pool();

    void fill_data();
    void bind_data();
    void update_uniform_buffers();

    void create_pipeline();
    void create_descriptor_set();
    void create_attachments();
    void create_render_pass();
    void create_command_buffers();

    void output_result(char *imagedata, VkSubresourceLayout subresourceLayout);

    /* Helper methods */
    VkShaderModule create_shader_module(const std::vector<char> &code);
    void create_attachment(Attachment &att, VkImageUsageFlags imgUsage, VkFormat format, VkImageAspectFlags aspect);
    VkFormat find_supported_format(const std::vector<VkFormat> &candidates, VkFormatFeatureFlags features);
    void bind_memory(VkDeviceSize dataSize, BufferWrap &stagingWrap, BufferWrap &destWrap);
    void create_buffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, BufferWrap *wrap, VkDeviceSize size, void *data = nullptr);
    uint32_t get_memtype_index(uint32_t typeBits, VkMemoryPropertyFlags properties);
    void submit_work(VkCommandBuffer cmd, VkQueue queue);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
                                                         uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData);
    /* End of Helper methods */
};

#endif