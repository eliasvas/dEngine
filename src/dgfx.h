#ifndef __DGFX__
#define __DGFX__
#include "tools.h"
#include "dwin.h"
#include "volk/volk.h"

#define DG_PHYSICAL_DEVICE_MAX 10
#define MAX_FRAMES_IN_FLIGHT 2

typedef struct dgShader 
{
    VkShaderModule module;
    VkShaderStageFlagBits stage;

    //ShaderMetaInfo info;
    b32 uses_push_constants;
}dgShader;

typedef struct dgTexture{
    VkSampler sampler;
    VkImage image;
    VkImageLayout image_layout;
    VkDeviceMemory mem;
    VkImageView view;
    VkFormat format;
    u32 width, height;
    u32 mip_levels;
} dgTexture;

typedef struct dgSwapchain
{ 
	VkSwapchainKHR swapchain;
	VkImage *images;
	VkFormat image_format;
	VkExtent2D extent;
	VkImageView *image_views;
	VkFramebuffer *framebuffers;
	dgTexture depth_attachment;
	u32 image_count;
	
    //TODO(ilias): delete render pass, its the LAW!!!!!!!!!!
	VkRenderPass rp_begin;
}dgSwapchain;

typedef struct dgBuffer 
{
    VkDevice device;
    VkBuffer buffer;
    VkDeviceMemory mem;
    VkDescriptorBufferInfo desc;
    VkDeviceSize size;
    VkDeviceSize alignment;
    VkBufferUsageFlags usage_flags;
    VkMemoryPropertyFlags memory_property_flags;
    void* mapped;
    b32 active;
}dgBuffer;
typedef struct dgPipeline 
{
    dgShader vert_shader;
    dgShader frag_shader;
    //dgShader compute_shader;

    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;

    // do we really need the descriptor pool? (maybe have them in a cache??)
    VkDescriptorPool descriptor_pools[16]; //pools need to be reallocated with every swapchain recreation! @TODO
    VkDescriptorSet* descriptor_sets;
    dgBuffer* uniform_buffers;
}dgPipeline;

typedef struct dgDevice
{
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkSurfaceKHR surface;
    VkDebugUtilsMessengerEXT debug_msg;

    VkQueue graphics_queue;
    VkQueue present_queue;

    dgSwapchain swap;

    dgPipeline fullscreen_pipe;

    VkCommandPool command_pool;
    VkCommandBuffer* command_buffers;

    VkSemaphore *image_available_semaphores;
    VkSemaphore *render_finished_semaphores;
    VkFence *in_flight_fences;
    VkFence *images_in_flight;


    u32 image_index; //current image index to draw
    u32 current_frame;
}dgDevice;

b32 dgfx_init(void);

void dg_draw_frame(dgDevice *ddev);
#endif