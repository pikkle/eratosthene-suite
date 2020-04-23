#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "models.h"
#include "utils.h"

VkInstance er_instance;
VkSurfaceKHR er_surface = VK_NULL_HANDLE;
VkPhysicalDevice er_phys_device;
VkDevice er_device;
uint32 er_graphics_queue_family_index;
uint32 er_transfer_queue_family_index;
//VkDebugUtilsMessengerEXT er_debugger;
VkDebugReportCallbackEXT er_debug_report{};
VkQueue er_graphics_queue;
VkQueue er_transfer_queue;
Attachment er_color_attachment;
Attachment er_depth_attachment;
VkBuffer er_uni_buffer;

void init();
void create_instance();
void setup_debugger();
void create_device();
void create_pipeline();
void create_attachment();
void create_render_pass();
void create_descriptor_layout();
void create_graphics_pipeline();
void create_transfer_pipeline();
void create_command_pool();
void create_depth_resources();
void create_vertex_buffer();
void create_index_buffer();
void create_uniform_buffer();
void create_descriptors();
void create_command_buffers();

void submit_work(VkCommandBuffer cmd, VkQueue queue);
void draw_frame();

const char* output_result();
void store_img_to_disk(const char* imagedata);
void broadcast_frame();

