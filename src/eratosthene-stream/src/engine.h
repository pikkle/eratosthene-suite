#ifndef ERATOSTHENE_STREAM_ENGINE_H
#define ERATOSTHENE_STREAM_ENGINE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <seasocks/Server.h>
#include <seasocks/WebSocket.h>

#include "models.h"
#include "utils.h"

const int WIDTH = 800;
const int HEIGHT = 600;
const float FPS = 60.f;

class Er_vk_engine {
public:
    Er_vk_engine();
    ~Er_vk_engine();
    void draw_frame(float angle, char *imagedata, VkSubresourceLayout subresourceLayout);

    static const size_t er_imagedata_size;

private:
    /* Shared vulkan objects among all engines running */
    static VkInstance er_instance;
    static VkPhysicalDevice er_phys_device;
    constexpr static const VkSurfaceKHR er_surface = VK_NULL_HANDLE; // @FUTURE obtain a headless surface for swapchain rendering

    static void create_instance();
    static void create_phys_device();

    VkDevice er_device;
    VkDebugReportCallbackEXT er_debug_report;
    uint32 er_graphics_queue_family_index;
    uint32 er_transfer_queue_family_index;
    VkQueue er_graphics_queue;
    VkQueue er_transfer_queue;
    VkCommandPool er_graphics_command_pool;
    VkCommandPool er_transfer_command_pool;
    VkFormat er_color_format = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormat er_depth_format;
    Attachment er_color_attachment;
    Attachment er_depth_attachment;
    VkRenderPass er_render_pass;
    VkFramebuffer er_framebuffer;
    VkDescriptorSetLayout er_descriptor_set_layout;
    VkDescriptorPool er_descriptor_pool;
    VkDescriptorSet er_descriptor_set;
    VkPipeline er_pipeline_triangles;
    VkPipeline er_pipeline_lines;
    VkPipeline er_pipeline_points;
    VkPipelineLayout er_pipeline_layout;
    VkPipelineCache er_pipeline_cache;
    VkCommandBuffer er_command_buffer;
    BufferWrap er_vertices_buffer;
    BufferWrap er_triangles_buffer;
    BufferWrap er_lines_buffer;
    BufferWrap er_points_buffer;
    BufferWrap er_uniform_buffer;

    void setup_debugger();
    void create_device();
    void create_command_pool();
    void bind_data();
    void create_pipeline();
    void create_descriptor_set();
    void create_attachments();
    void create_render_pass();
    void create_command_buffers();
    void update_uniform_buffers(float angle);
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