#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "models.h"
#include "utils.h"

VkInstance er_instance;
VkSurfaceKHR er_surface = VK_NULL_HANDLE;
VkPhysicalDevice er_phys_device;
VkDevice er_device;
VkDebugReportCallbackEXT er_debug_report{};
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
VkBuffer er_uni_buffer;
BufferWrap er_vertices_buffer;
BufferWrap er_triangles_buffer;
BufferWrap er_lines_buffer;
BufferWrap er_points_buffer;

void init();
void create_instance();
void setup_debugger();
void create_device();
void create_command_pool();
void bind_data();
void create_descriptor_layout();
void create_pipeline();
void create_attachments();
void create_render_pass();
void create_graphics_pipeline();
void create_transfer_pipeline();
void create_depth_resources();
void create_uniform_buffer();
void create_descriptors();
void create_command_buffers();
void draw_frame();

const char* output_result();
void store_img_to_disk(const char* imagedata);
void broadcast_frame();

