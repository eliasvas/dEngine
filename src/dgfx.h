#ifndef __DGFX__
#define __DGFX__
#include "tools.h"
#include "dwin.h"
#include "volk/volk.h"
#include "spirv_reflect/spirv_reflect.h"

#define DG_PHYSICAL_DEVICE_MAX 10
#define DG_QUEUE_FAMILY_MAX 32
#define DG_VERTEX_INPUT_ATTRIB_MAX 4
#define DG_MAX_COLOR_ATTACHMENTS 4
#define MAX_FRAMES_IN_FLIGHT 2
#define DG_DEPTH_SIZE 2048 
#define DG_MAX_DESCRIPTOR_POOLS 32 

typedef struct dgShader 
{
    VkShaderModule module;
    VkShaderStageFlagBits stage;

    SpvReflectShaderModule info;

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

typedef struct dgDescriptorAllocator
{
    
    VkDevice device;
    VkDescriptorPool current_pool;

    VkDescriptorPool used_pools[DG_MAX_DESCRIPTOR_POOLS];
    u32 used_pool_count;
    VkDescriptorPool free_pools[DG_MAX_DESCRIPTOR_POOLS];
    u32 free_pool_count;


    VkDescriptorType *desc_types;
    float pool_sizes[4];
    H32_static desc_type_hash; //gives arrayto pool sizes

}dgDescriptorAllocator;


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
    dgPipeline base_pipe;

    VkCommandPool command_pool;
    VkCommandBuffer* command_buffers;

    VkSemaphore *image_available_semaphores;
    VkSemaphore *render_finished_semaphores;
    VkFence *in_flight_fences;
    VkFence *images_in_flight;

    dgDescriptorAllocator desc_alloc;

    u32 image_index; //current image index to draw
    u32 current_frame;
}dgDevice;

typedef struct dgVertex
{
    vec3 pos;
    vec3 normal;
    vec2 tex_coord;
}dgVertex;

typedef struct dgRT
{
    dgTexture color_attachments[DG_MAX_COLOR_ATTACHMENTS];
    dgTexture depth_attachment;
    u32 color_attachment_count;
    b32 depth_active;
}dgRT;


static void dg_rt_init(dgDevice *ddev, dgRT* rt, u32 color_count, b32 depth);

b32 dgfx_init(void);

void dg_draw_frame(dgDevice *ddev);
void dg_frame_begin(dgDevice *ddev);
void dg_frame_end(dgDevice *ddev);
#endif