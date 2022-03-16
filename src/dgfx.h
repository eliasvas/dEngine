#ifndef __DGFX__
#define __DGFX__
#include "tools.h"
#include "dwin.h"
#include "volk/volk.h"

#define DG_PHYSICAL_DEVICE_MAX 10

typedef struct dgDevice
{
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkSurfaceKHR surface;
    VkDebugUtilsMessengerEXT debug_msg;

    VkQueue graphics_queue;
    VkQueue present_queue;

    u32 image_index; //current image index to draw
}dgDevice;

b32 dgfx_init(void);

#endif