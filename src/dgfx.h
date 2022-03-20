#ifndef __DGFX__
#define __DGFX__
#include "tools.h"
#include "dwin.h"
#include "volk/volk.h"

#define DG_PHYSICAL_DEVICE_MAX 10

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

    u32 image_index; //current image index to draw
}dgDevice;

b32 dgfx_init(void);

#endif