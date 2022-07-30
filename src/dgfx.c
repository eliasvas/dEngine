#include "dgfx.h"
#define VK_USE_PLATFORM_XLIB_KHR
//#define VOLK_IMPLEMENTATION we don't define this because volk.c is compiled as object file
#include "volk/volk.h"
#include "SDL_vulkan.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "dtime.h"
#include "../ext/microui/microui.h"
#include "../ext/microui/atlas.inl"
#include "vulkan/shaderc.h"
#include "shaders.inl"
#include "dlog.h"
#include "dcamera.h"
#include "dmem.h"
#include "dmodel.h"
#include "dentity.h"

extern void draw_model(dgDevice *ddev, dModel *m, mat4 model);
extern void draw_model_def(dgDevice *ddev, dModel *m, mat4 model);
dModel water_bottle;
dModel fox;

dgBuffer pos_vbo;
dgBuffer normal_vbo;
dgBuffer tex_vbo;

dgDevice dd;
dgBuffer base_vbo;

dgBuffer base_pos;
dgBuffer base_norm;
dgBuffer base_tex;

dgBuffer base_ibo;
dgTexture t1;
dgTexture t2;
dgRT def_rt;
dgRT shadow_rt;
dgRT csm_rt;
dCamera cam;

//NOTE(ilias): This is UGLY AF!!!!
extern dWindow main_window;


#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			printf("[LINE: %i] Detected Vulkan error: %i \n",__LINE__, err);            \
		}                                                           \
	} while (0);

static const char* device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
    VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    VK_KHR_MULTIVIEW_EXTENSION_NAME,
};

static const char *validation_layers[]= {
    "VK_LAYER_KHRONOS_validation"
};

vec3 cube_positions[]  = {
	{0.5f, 0.5f, 0.5f},  {-0.5f, 0.5f, 0.5f},  {-0.5f,-0.5f, 0.5f}, {0.5f,-0.5f, 0.5f},   // v0,v1,v2,v3 (front)
    {0.5f, 0.5f, 0.5f},  {0.5f,-0.5f, 0.5f},   {0.5f,-0.5f,-0.5f},  {0.5f, 0.5f,-0.5f},   // v0,v3,v4,v5 (right)
    {0.5f, 0.5f, 0.5f},  {0.5f, 0.5f,-0.5f},   {-0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f, 0.5f},   // v0,v5,v6,v1 (top)
    {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f},  {-0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f, 0.5f},   // v1,v6,v7,v2 (left)
    {-0.5f,-0.5f,-0.5f}, {0.5f,-0.5f,-0.5f},   {0.5f,-0.5f, 0.5f},  {-0.5f,-0.5f, 0.5f},   // v7,v4,v3,v2 (bottom)
    {0.5f,-0.5f,-0.5f},  {-0.5f,-0.5f,-0.5f},  {-0.5f, 0.5f,-0.5f}, {0.5f, 0.5f,-0.5f},    // v4,v7,v6,v5 (back)
};

// normal array
vec3 cube_normals[] = {
	{0, 0, 1},   {0, 0, 1},   {0, 0, 1},   {0, 0, 1},  // v0,v1,v2,v3 (front)
	{1, 0, 0},   {1, 0, 0},   {1, 0, 0},   {1, 0, 0},  // v0,v3,v4,v5 (right)
	{0, 1, 0},   {0, 1, 0},   {0, 1, 0},   {0, 1, 0},  // v0,v5,v6,v1 (top)
	{-1, 0, 0},  {-1, 0, 0},  {-1, 0, 0},  {-1, 0, 0},  // v1,v6,v7,v2 (left)
	{0,-1, 0},   {0,-1, 0},   {0,-1, 0},   {0,-1, 0},  // v7,v4,v3,v2 (bottom)
	{0, 0,-1},   {0, 0,-1},   {0, 0,-1},   {0, 0,-1},   // v4,v7,v6,v5 (back)
};

// texCoord array
vec2 cube_tex_coords[] = {
    {1, 0},   {0, 0},   {0, 1},   {1, 1},               // v0,v1,v2,v3 (front)
	{0, 0},   {0, 1},   {1, 1},   {1, 0},               // v0,v3,v4,v5 (right)
	{1, 1},   {1, 0},   {0, 0},   {0, 1},               // v0,v5,v6,v1 (top)
	{1, 0},   {0, 0},   {0, 1},   {1, 1},               // v1,v6,v7,v2 (left)
	{0, 1},   {1, 1},   {1, 0},   {0, 0},               // v7,v4,v3,v2 (bottom)
	{0, 1},   {1, 1},   {1, 0},   {0, 0}                // v4,v7,v6,v5 (back)
};

u16 cube_indices[] = {
     0, 1, 2,   2, 3, 0,    // v0-v1-v2, v2-v3-v0 (front)
     4, 5, 6,   6, 7, 4,    // v0-v3-v4, v4-v5-v0 (right)
     8, 9,10,  10,11, 8,    // v0-v5-v6, v6-v1-v0 (top)
    12,13,14,  14,15,12,    // v1-v6-v7, v7-v2-v1 (left)
    16,17,18,  18,19,16,    // v7-v4-v3, v3-v2-v7 (bottom)
    20,21,22,  22,23,20     // v4-v7-v6, v6-v5-v4 (back)
};
static dgVertex *cube_build_verts(void)
{
	dgVertex *verts = (dgVertex*)dalloc(sizeof(dgVertex) * 24);
	for (u32 i =0; i < 24; ++i)
	{
		verts[i].pos = cube_positions[i];
		verts[i].normal = cube_normals[i];
		verts[i].tex_coord = cube_tex_coords[i];
	}
	return verts;
}

typedef struct GlobalData
{
    mat4 view;
    mat4 proj;
    mat4 viewproj;
}GlobalData;

b32 dg_create_instance(dgDevice *ddev) {
	VkInstance instance;

	VkApplicationInfo appinfo = {0};
	appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appinfo.pApplicationName = "Mk0";
	appinfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
	appinfo.pEngineName = "dEngine";
	appinfo.engineVersion = VK_MAKE_VERSION(1,0,0);
	appinfo.apiVersion = VK_API_VERSION_1_3;
    
    
	VkInstanceCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &appinfo;
    
	u32 instance_ext_count = 0;
	char **instance_extensions; //array of strings with names of VK extensions needed!
    char *base_extensions[] = {
		"VK_KHR_surface",
#if defined(BUILD_UNIX)
        "VK_KHR_xlib_surface",
#elif defined (BUILD_WIN)
        "VK_KHR_win32_surface",
#endif
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
	};
    
	create_info.enabledExtensionCount = array_count(base_extensions); create_info.ppEnabledExtensionNames = (const char**)base_extensions;
	create_info.enabledLayerCount = 0;

    
	VK_CHECK(volkInitialize());

	VK_CHECK(vkCreateInstance(&create_info, NULL, &instance));
    volkLoadInstance(instance);

	//(OPTIONAL): extension support
	u32 ext_count = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &ext_count, NULL);
	VkExtensionProperties *extensions = (VkExtensionProperties*)dalloc(sizeof(VkExtensionProperties) * ext_count);
	vkEnumerateInstanceExtensionProperties(NULL, &ext_count, extensions);

	//for (u32 i = 0; i < ext_count; ++i)printf("EXT: %s\n", extensions[i].extensionName);
	ddev->instance = instance;
    assert(instance);

	return TRUE;
}

typedef struct dgQueueFamilyIndices
{
    u32 graphics_family;
    //because we cant ifer whether the vfalue was initialized correctly or is garbage
    u32 graphics_family_found;
    
    u32 present_family;
    u32 present_family_found;
}dgQueueFamilyIndices;


//@TODO(ilias) vkGetPHysicalDeviceQueueFamilyProperties doesn't work with static queue_families
static dgQueueFamilyIndices dg_find_queue_families(VkPhysicalDevice device)
{
	dgQueueFamilyIndices indices = {0};

	u32 queue_family_count = 1;
	VkQueueFamilyProperties queue_families[DG_QUEUE_FAMILY_MAX];
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);
	//queue_family_count = minimum(queue_family_count, DG_QUEUE_FAMILY_MAX);
	VkBool32 present_support = VK_FALSE;
	//@FIX(ilias): WHY is queue_family_count 0???????????????????? goddamn! linux 
	for (u32 i = 0; i < queue_family_count; ++i)
	{
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, dd.surface, &present_support);
		if (present_support){indices.present_family = i; indices.present_family_found = TRUE;}

		if (queue_families[i].queueFlags  & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphics_family = i;
			indices.graphics_family_found = TRUE;
		}
	}
	return indices;
}

static u32 dg_check_device_extension_support(VkPhysicalDevice device)
{
    u32 ext_count;
    vkEnumerateDeviceExtensionProperties(device, NULL, &ext_count, NULL);
    VkExtensionProperties *available_extensions = (VkExtensionProperties*)dalloc(sizeof(VkExtensionProperties) * ext_count);
    
    vkEnumerateDeviceExtensionProperties(device, NULL, &ext_count, available_extensions);
    /*
    for (u32 i = 0; i < ext_count; ++i)
        dlog(NULL, "EXT %i: %s\n", i, available_extensions[i]);
	*/
    for (u32 i = 0; i < array_count(device_extensions); ++i)
    {
        u32 ext_found = FALSE;
        for (u32 j = 0; j < ext_count; ++j)
        {
            if (strcmp(device_extensions[i], available_extensions[j].extensionName)==0)
            {
                ext_found = TRUE;
                break;
            }
        }
        if (ext_found == FALSE)
		{
			free(available_extensions);
			return FALSE;
		};
    }
	free(available_extensions);
    return TRUE;
}
typedef struct dgSwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    u32 format_count;
    VkPresentModeKHR *present_modes;
    u32 present_mode_count;
}dgSwapChainSupportDetails;

static void dg_swapchain_support_details_free(dgSwapChainSupportDetails *s)
{
    if (s->present_modes)free(s->present_modes);
    if (s->formats)free(s->formats);
}

static dgSwapChainSupportDetails dg_query_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    dgSwapChainSupportDetails details = {0};
    
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    
    u32 format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, NULL);
    details.format_count = format_count;
    
    if (format_count != 0)
    {
        details.formats = (VkSurfaceFormatKHR*)dalloc(sizeof(VkSurfaceFormatKHR) * format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats);
    }
    u32 present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, NULL);
    details.present_mode_count = present_mode_count;
    
    if (present_mode_count != 0)
    {
        details.present_modes = (VkPresentModeKHR*)dalloc(sizeof(VkPresentModeKHR) * present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes);
    }
    
    
    return details;
}
static u32 is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	dgQueueFamilyIndices indices = dg_find_queue_families(device);
    
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceProperties(device, &device_properties);
    vkGetPhysicalDeviceFeatures(device, &device_features);
    
    u32 extensions_supported = dg_check_device_extension_support(device);
    if (extensions_supported == 0) dlog(NULL, "GRAPHICS DRIVER: some device extension not supported!!\n");
    
    dgSwapChainSupportDetails swapchain_support = dg_query_swapchain_support(device, surface);
    
    //here we can add more requirements for physical device selection
    return indices.graphics_family_found && extensions_supported && indices.present_family_found
    &&(swapchain_support.format_count > 0) && (swapchain_support.present_mode_count > 0);	
}

b32 dg_pick_physical_device(dgDevice *ddev)
{
	u32 device_count = 0;
    vkEnumeratePhysicalDevices(ddev->instance, &device_count, NULL);

    if (device_count == 0)
	{
        dlog(NULL, "Failed to find GPUs with Vulkan support!");
		return FALSE;
	}
    
    VkPhysicalDevice devices[DG_PHYSICAL_DEVICE_MAX];
    vkEnumeratePhysicalDevices(ddev->instance, &device_count, devices);
	//@FIX(ilias): this is 1 here because llvmpipe is 0 and we don't want that! no vulkan 1.3!!
#ifdef BUILD_UNIX
    for (u32 i = 0; i < device_count; ++i)
#else
    for (u32 i = 0; i < device_count; ++i)
#endif
        if (is_device_suitable(devices[i], ddev->surface))
			{
				ddev->physical_device = devices[i];

				VkPhysicalDeviceProperties p;
				vkGetPhysicalDeviceProperties(ddev->physical_device, &p);
				dlog(NULL, "VULKAN: physical device picked: %s\n", p.deviceName);
				break;
			}
	
	return TRUE;
}

//NOTE(ilias): the window must be initialized and have a vulkan compatible surface
static b32 dg_surface_create(dgDevice *ddev,dWindow *window)
{
	//VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR
    //printf("1%s\n", SDL_GetError());
	VkResult res = SDL_Vulkan_CreateSurface(window->window, ddev->instance, &ddev->surface);
    //printf("2%s\n", SDL_GetError());
	return TRUE;
}

b32 dg_create_logical_device(dgDevice *ddev)
{
    dgQueueFamilyIndices indices = dg_find_queue_families(ddev->physical_device);
    
    f32 queue_priority = 1.0f;
	VkDeviceQueueCreateInfo queue_create_info[2] = {0};

    queue_create_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info[0].queueFamilyIndex = indices.graphics_family;
    queue_create_info[0].queueCount = 1;
    queue_create_info[0].pQueuePriorities = &queue_priority;

    queue_create_info[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info[1].queueFamilyIndex = indices.present_family;
    queue_create_info[1].queueCount = 1;
    queue_create_info[1].pQueuePriorities = &queue_priority;
    VkPhysicalDeviceFeatures device_features = {0};
    
    VkDeviceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    
    create_info.pQueueCreateInfos = &queue_create_info[0];
    create_info.queueCreateInfoCount = (indices.graphics_family != indices.present_family) ? 2 : 1;
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = array_count(device_extensions);
    create_info.ppEnabledExtensionNames = &device_extensions[0];
    //printf("graphics fam: %i, present fam: %i, createinfocount: %i\n", 
    //       indices.graphics_family, indices.present_family, create_info.queueCreateInfoCount);
    if(TRUE)//if (enable_validation_layers)
    {
        create_info.enabledLayerCount = array_count(validation_layers);
        create_info.ppEnabledLayerNames = validation_layers;
    }
    else
        create_info.enabledLayerCount = 0;


    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_state = {0};
    dynamic_rendering_state.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
    dynamic_rendering_state.pNext = NULL;
    dynamic_rendering_state.dynamicRendering = VK_TRUE;


    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state = { 0 };
    extended_dynamic_state.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    extended_dynamic_state.pNext = &dynamic_rendering_state;
    extended_dynamic_state.extendedDynamicState = VK_TRUE;

    create_info.pNext = &extended_dynamic_state;

    VK_CHECK(vkCreateDevice(ddev->physical_device, &create_info, NULL, &ddev->device));

    
    vkGetDeviceQueue(ddev->device, indices.graphics_family, 0, &ddev->graphics_queue);
    vkGetDeviceQueue(ddev->device, indices.present_family, 0, &ddev->present_queue);

	return TRUE;
}

static VkSurfaceFormatKHR dg_choose_swap_surface_format(dgSwapChainSupportDetails details)
{
    for (u32 i = 0; i < details.format_count; ++i)
    {
        if (details.formats[i].format == VK_FORMAT_R8G8B8A8_SRGB  &&
        //if (details.formats[i].format == VK_FORMAT_R16G16B16A16_SFLOAT &&
            details.formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return details.formats[i];
        }
    }
    return details.formats[0];
}

static VkPresentModeKHR dg_choose_swap_present_mode(dgSwapChainSupportDetails details)
{
    return VK_PRESENT_MODE_FIFO_KHR;
    //return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

static VkExtent2D dg_choose_swap_extent(dgSwapChainSupportDetails details)
{
    if (details.capabilities.currentExtent.width != UINT32_MAX)
    {
        return details.capabilities.currentExtent;
    }
    else
    {
        u32 width, height;
        width = main_window.width;
        height = main_window.height;

        VkExtent2D actual_extent = {width, height};
        actual_extent.height=maximum(details.capabilities.maxImageExtent.height,
                                     minimum(details.capabilities.minImageExtent.height,actual_extent.height));
        actual_extent.width=maximum(details.capabilities.maxImageExtent.width,
                                    minimum(details.capabilities.minImageExtent.width,actual_extent.width));
        return actual_extent;
    }
}

#define MAX_SWAP_IMAGE_COUNT 2
static dgTexture dg_create_depth_attachment(dgDevice *ddev, u32 width, u32 height, u32 layer_count);
static b32 dg_create_swapchain(dgDevice *ddev)
{

    dgSwapChainSupportDetails swap_details = dg_query_swapchain_support(ddev->physical_device, ddev->surface);

    VkSurfaceFormatKHR surface_format = dg_choose_swap_surface_format(swap_details);
    VkPresentModeKHR present_mode = dg_choose_swap_present_mode(swap_details);
    VkExtent2D extent = dg_choose_swap_extent(swap_details);

    u32 image_count = swap_details.capabilities.minImageCount + 1;
    if (swap_details.capabilities.maxImageCount > 0 && image_count > swap_details.capabilities.maxImageCount)
        image_count = swap_details.capabilities.maxImageCount;
    image_count = minimum(image_count, MAX_SWAP_IMAGE_COUNT);

    VkSwapchainCreateInfoKHR create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = ddev->surface;

    create_info.minImageCount = image_count+1;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    dgQueueFamilyIndices indices = dg_find_queue_families(ddev->physical_device);
    u32 queue_family_indices[] = {indices.graphics_family, indices.present_family};
    
    if (indices.graphics_family != indices.present_family)
    {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = NULL;
    }
    create_info.preTransform = swap_details.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(ddev->device, &create_info, NULL, &ddev->swap.swapchain));

    vkGetSwapchainImagesKHR(ddev->device, ddev->swap.swapchain, &image_count, NULL);
    ddev->swap.images = (VkImage*)dalloc(sizeof(VkImage) * image_count);
    vkGetSwapchainImagesKHR(ddev->device, ddev->swap.swapchain, &image_count, ddev->swap.images);
    ddev->swap.image_format = surface_format.format;
    ddev->swap.extent = extent;
    ddev->swap.image_count = image_count;
	ddev->swap.depth_attachment = dg_create_depth_attachment(ddev, ddev->swap.extent.width, ddev->swap.extent.height,1);
	
    return DSUCCESS;
}

static void dg_cleanup_swapchain(dgDevice *ddev)
{
   for (u32 i = 0; i < ddev->swap.image_count; ++i)
        vkDestroyImageView(ddev->device, ddev->swap.image_views[i], NULL);
    vkDestroySwapchainKHR(ddev->device, ddev->swap.swapchain, NULL);
    //clear depth attachment of swapchain
}

static void dg_cleanup_texture(dgDevice *ddev, dgTexture *tex)
{
    vkDestroyImage(ddev->device, tex->image,NULL);
    vkDestroyImageView(ddev->device, tex->view,NULL);
    vkDestroySampler(ddev->device, tex->sampler, NULL);
}

static VkImageView dg_create_image_view(VkImage image, VkFormat format, VkImageViewType view_type,VkImageAspectFlags aspect_flags, u32 layer_count, u32 base_layer)
{
	VkImageView image_view;
	
	VkImageViewCreateInfo view_info = {0};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = image;
	view_info.viewType = view_type;
	view_info.format = format;
	view_info.subresourceRange.aspectMask = aspect_flags;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseArrayLayer = base_layer;
	view_info.subresourceRange.layerCount = layer_count;
	VK_CHECK(vkCreateImageView(dd.device, &view_info, NULL, &image_view));
	return image_view;
}

static void dg_create_texture_sampler(dgDevice *ddev, VkSampler *sampler)
{
	VkSamplerCreateInfo sampler_info = {0};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_NEAREST;
	sampler_info.minFilter = VK_FILTER_NEAREST;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	
	VkPhysicalDeviceProperties prop = {0};
	vkGetPhysicalDeviceProperties(ddev->physical_device, &prop);
	
	sampler_info.anisotropyEnable = VK_FALSE;
	sampler_info.maxAnisotropy = prop.limits.maxSamplerAnisotropy;
	//sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	//sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;
	VK_CHECK(vkCreateSampler(ddev->device, &sampler_info, NULL, sampler));
}

static b32 dg_create_swapchain_image_views(dgDevice *ddev)
{
    ddev->swap.image_views = (VkImageView*)dalloc(sizeof(VkImageView) * ddev->swap.image_count);
    for (u32 i = 0; i < ddev->swap.image_count; ++i)
		ddev->swap.image_views[i] = dg_create_image_view(ddev->swap.images[i], ddev->swap.image_format,VK_IMAGE_VIEW_TYPE_2D,VK_IMAGE_ASPECT_COLOR_BIT,1,0);
    return DSUCCESS;
}


#include "dconfig.h"
extern dConfig engine_config;

VkShaderModule dg_create_shader_module(char *code, u32 size)
{
	VkShaderModuleCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = size;
	create_info.pCode = (u32*)code;
	VkShaderModule shader_module;
	VK_CHECK(vkCreateShaderModule(dd.device, &create_info, NULL, &shader_module));
	return shader_module;
}

void dg_shader_create(VkDevice device, dgShader *shader, const char *filename, VkShaderStageFlagBits stage)
{
    if (str_size(filename) > 128)
        return dg_shader_create_dynamic_ready(device, shader, filename, stage);
    char path[128];
    sprintf(path, "%s%s.spv", engine_config.spirv_path, filename);
	u32 code_size;
	u32 *shader_code = NULL;
	if (read_file(path, &shader_code, &code_size) == -1){
        //if .SPV not found, create the shader dynamically! (from glsl source file)
        return dg_shader_create_dynamic(device, shader, filename, stage);
    }
	shader->module = dg_create_shader_module((char*)shader_code, code_size);
	shader->uses_push_constants = FALSE;
    //shader_reflect(shader_code, code_size, &shader->info);
	//printf("Shader: %s has %i input variable(s)!\n", filename, shader->info.input_variable_count);
	shader->stage = stage;
    spvReflectCreateShaderModule(code_size, shader_code, &shader->info);
	free(shader_code);

}  
void dg_shader_create_dynamic_ready(VkDevice device, dgShader *s, const char *filename, VkShaderStageFlagBits stage)
{
    char *glsl_text = filename;
    u32 glsl_size = str_size(glsl_text);

    shaderc_compiler_t compiler = shaderc_compiler_initialize();
    shaderc_shader_kind shader_kind = (stage & VK_SHADER_STAGE_VERTEX_BIT) ? shaderc_glsl_vertex_shader : shaderc_glsl_fragment_shader;
    shaderc_compilation_result_t result = shaderc_compile_into_spv(compiler, glsl_text, glsl_size,
                                         shader_kind, "file",
                                         "main", NULL);


    assert(shaderc_result_get_compilation_status(result)== shaderc_compilation_status_success);

	s->module = dg_create_shader_module((char*)shaderc_result_get_bytes(result), shaderc_result_get_length(result));
	s->uses_push_constants = FALSE;
	s->stage = stage;
    spvReflectCreateShaderModule(shaderc_result_get_length(result), shaderc_result_get_bytes(result), &s->info);
	//free(glsl_text);
    shaderc_result_release(result);
    shaderc_compiler_release(compiler);
}  


void dg_shader_create_dynamic(VkDevice device, dgShader *s, const char *filename, VkShaderStageFlagBits stage)
{
    char path[128];
    sprintf(path, "%s%s", engine_config.shader_path, filename);
    char *glsl_text;
    u32 glsl_size;
    //this means that either there is no such shader or that we entered a READY glsl 
    //this is slow though so we should probably have a distinct path for ready shaders 
	if (read_file(path, &glsl_text, &glsl_size) == -1)
    {
        glsl_text = filename;
        glsl_size = str_size(glsl_text);
    }

    shaderc_compiler_t compiler = shaderc_compiler_initialize();
    shaderc_shader_kind shader_kind = (stage & VK_SHADER_STAGE_VERTEX_BIT) ? shaderc_glsl_vertex_shader : shaderc_glsl_fragment_shader;
    shaderc_compilation_result_t result = shaderc_compile_into_spv(compiler, glsl_text, glsl_size,
                                         shader_kind, "file",
                                         "main", NULL);


    assert(shaderc_result_get_compilation_status(result)== shaderc_compilation_status_success);

	s->module = dg_create_shader_module((char*)shaderc_result_get_bytes(result), shaderc_result_get_length(result));
	s->uses_push_constants = FALSE;
	s->stage = stage;
    spvReflectCreateShaderModule(shaderc_result_get_length(result), shaderc_result_get_bytes(result), &s->info);
	//free(glsl_text);
    shaderc_result_release(result);
    shaderc_compiler_release(compiler);
}  


static VkViewport viewport(f32 x, f32 y, f32 w, f32 h)
{
    VkViewport viewport = { 0 };
    viewport.x = x;
    viewport.y = y;
    viewport.width = w;
    viewport.height = h;
    //viewport.height *= fabs(sin(get_time()));
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}

static VkRect2D scissor(f32 x, f32 y, f32 w, f32 h)
{
    VkRect2D scissor = {0};
    scissor.offset.x = x;
	scissor.offset.y = x;
    scissor.extent = (VkExtent2D){w,h};
    //scissor.extent.height *= fabs(sin(get_time()));
	
    return scissor;

}

VkPipelineDepthStencilStateCreateInfo dg_pipe_depth_stencil_state_create_info_basic(void)
{
    VkPipelineDepthStencilStateCreateInfo depth_stencil = {0};
	depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
	depth_stencil.depthWriteEnable = VK_TRUE;
	depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depth_stencil.depthBoundsTestEnable = VK_FALSE;
	depth_stencil.minDepthBounds = 0.0f;
	depth_stencil.maxDepthBounds = 1.0f;
	depth_stencil.stencilTestEnable = VK_FALSE;
    return depth_stencil;
}
VkPipelineShaderStageCreateInfo 
	dg_pipe_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shader_module)
{
	VkPipelineShaderStageCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.pNext = NULL;
	
	info.stage = stage;
	info.module = shader_module;
	info.pName = "main";
	return info;
}


static u32 dg_get_vertex_desc(dgShader *shader, VkVertexInputAttributeDescription *attr_desc, VkVertexInputBindingDescription *bind_desc, b32 pack_attribs)
{
    u32 global_offset = 0;
    u32 attribute_count = shader->info.input_variable_count;
    memset(attr_desc, 0, sizeof(VkVertexInputAttributeDescription) * DG_VERTEX_INPUT_ATTRIB_MAX);

    for (u32 i = 0; i < shader->info.input_variable_count; ++i)
    {
        for (u32 j = 0; j <shader->info.input_variable_count; ++j)
        {
            u32 attr_index = (u32)shader->info.input_variables[j]->location;
            if (shader->info.input_variables[j]->built_in != -1)
            {
                --attribute_count;
                break;
            }
            if (attr_index == i )//Note(inv): we want to write the inputs in order to get the global offset
            {
                SpvReflectInterfaceVariable *input_var = shader->info.input_variables[j];
                memset(&attr_desc[attr_index], 0, sizeof(VkVertexInputAttributeDescription));
                attr_desc[attr_index].binding = (pack_attribs) ? 0 : attr_index;
                attr_desc[attr_index].format = input_var->format;
                attr_desc[attr_index].location = input_var->location;
                //attr_desc[attr_index].offset = (pack_attribs) ? global_offset : input_var->numeric.vector.component_count * sizeof(f32) ;
                attr_desc[attr_index].offset = (pack_attribs) ? global_offset : 0;
                global_offset += input_var->numeric.vector.component_count * sizeof(f32);

                bind_desc[i].stride = input_var->numeric.vector.component_count * sizeof(f32);

                break;
            }

        }

        for (u32 i = 0; i < attribute_count;++i)
        {
            u32 attr_index = (u32)shader->info.input_variables[i]->location;
            bind_desc[attr_index].inputRate= VK_VERTEX_INPUT_RATE_VERTEX;
            if (pack_attribs)
            {
                bind_desc[attr_index].binding = 0;
                bind_desc[attr_index].stride = global_offset;
            }
            else
            {
                bind_desc[attr_index].binding = attr_index;
                //bind_desc[i].stride is already calculated above :D
            }
        }

    }

    return attribute_count;
}

static VkPipelineVertexInputStateCreateInfo 
dg_pipe_vertex_input_state_create_info(dgShader *shader, VkVertexInputBindingDescription *bind_desc, VkVertexInputAttributeDescription *attr_desc, b32 pack_attribs)
{
    u32 vert_size;
    //fills attribute AND binding descriptions for our pipe's vertices
    u32 binding_count = dg_get_vertex_desc(shader, attr_desc, bind_desc,pack_attribs);
 
	VkPipelineVertexInputStateCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	info.pNext = NULL;
    if (pack_attribs)
        info.vertexBindingDescriptionCount = (binding_count >0) ? 1 : 0;
    else
        info.vertexBindingDescriptionCount = (binding_count >0) ? binding_count : 0;
    info.pVertexBindingDescriptions = bind_desc;
	info.vertexAttributeDescriptionCount = binding_count;
    info.pVertexAttributeDescriptions = attr_desc;

	return info;
}

static VkPipelineInputAssemblyStateCreateInfo dg_pipe_input_assembly_create_info(VkPrimitiveTopology topology)
{
	VkPipelineInputAssemblyStateCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	info.pNext = NULL;
	
	info.topology = topology;
	info.primitiveRestartEnable = VK_FALSE;
	return info;
}

static VkPipelineRasterizationStateCreateInfo dg_pipe_rasterization_state_create_info(VkPolygonMode polygon_mode)
{
	VkPipelineRasterizationStateCreateInfo info = {0};
	info.sType= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	info.pNext = NULL;
	
	info.depthClampEnable = VK_FALSE;
	info.rasterizerDiscardEnable = VK_FALSE;
	info.polygonMode = polygon_mode;
	info.lineWidth = 1.0f;
	info.cullMode = VK_CULL_MODE_NONE;
	info.frontFace = VK_FRONT_FACE_CLOCKWISE;
	//no depth bias (for now)
	info.depthBiasEnable = VK_FALSE;
	info.depthBiasConstantFactor = 0.0f;
	info.depthBiasClamp = 0.0f;
	info.depthBiasSlopeFactor = 0.0f;
	
	return info;
}


static VkPipelineMultisampleStateCreateInfo dg_pipe_multisampling_state_create_info(void)
{
	VkPipelineMultisampleStateCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	info.pNext = NULL;
	
	info.sampleShadingEnable = VK_FALSE;
	info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	info.minSampleShading = 1.0f;
	info.pSampleMask = NULL;
	info.alphaToCoverageEnable = VK_FALSE;
	info.alphaToOneEnable = VK_FALSE;
	return info;
}

static VkPipelineColorBlendAttachmentState dg_pipe_color_blend_attachment_state(b32 blend_enabled)
{
	VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	color_blend_attachment.blendEnable = (blend_enabled > 0) ? VK_TRUE : VK_FALSE;	

    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.alphaBlendOp= VK_BLEND_OP_ADD;
	return color_blend_attachment;
}

static VkPipelineColorBlendStateCreateInfo dg_pipe_color_blend_state_create_info(VkPipelineColorBlendAttachmentState *color_blend_attachments, u32 attachment_count)
{
    //dummy color blending
	VkPipelineColorBlendStateCreateInfo color_blending = {0};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.pNext = NULL;
	
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = attachment_count;
	VkAttachmentDescription color_attachment = {0};
    color_attachment.format = dd.swap.image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	color_blending.pAttachments = color_blend_attachments;

    return color_blending;
}

static VkPipelineViewportStateCreateInfo dg_pipe_viewport_state_create_info(VkViewport *v, VkRect2D *s)
{    
    VkPipelineViewportStateCreateInfo viewport_state = {0};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.pNext = NULL;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = v;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = s;
 

    return viewport_state;
}


static void dg_descriptor_set_layout_cache_init(dgDescriptorSetLayoutCache *cache)
{
    cache->layout_count = 0;
    H32_static_init(&cache->desc_set_layout_cache, DG_MAX_DESCRIPTOR_SET_LAYOUTS);
}

//adds a layout in the cache for future use
static void dg_descriptor_set_layout_cache_add(dgDescriptorSetLayoutCache *cache, dgDescriptorSetLayoutInfo info, VkDescriptorSetLayout layout)
{
    //we first put the layout info in an array to be able to compare with incoming queries 
    //to find out if the layout is the same with another coming our way
    cache->desc_set_layout_infos[cache->layout_count] = info;
    H32_static_set(&cache->desc_set_layout_cache, info.hash, cache->layout_count);
    //and then we add the layout in an array to have it ready
    cache->desc_set_layouts[cache->layout_count] = layout;
    ++cache->layout_count;

}

static VkDescriptorSetLayout dg_descriptor_set_layout_cache_get(dgDescriptorSetLayoutCache *cache, u64 key)
{
    u32 layout_index = H32_static_get(&cache->desc_set_layout_cache, key);
    if (key == cache->desc_set_layout_infos[layout_index].hash)
    {
        return cache->desc_set_layouts[layout_index];
    }else
    {
        //could not find layout in cache
        return VK_NULL_HANDLE;
    }
}


//first we check the hash to find the descriptor layout, if we can't find it, 
//we create it and submit in cache  @TODO(inv): the hashing should be better, investigate!
static u32 dg_pipe_descriptor_set_layout(dgDevice *ddev, dgShader*shader, VkDescriptorSetLayout *layouts)
{
    u32 layout_binding_bits = 0; //0x0 means that descriptor set 0 is valid
    u64 total_hash = 0;
    if (shader->info.descriptor_set_count == 0)return 0; 

    for(u32 i=0;i< shader->info.descriptor_set_count; ++i)
    {
        VkDescriptorSetLayoutBinding *desc_set_layout_bindings = NULL;
        SpvReflectDescriptorSet current_set = shader->info.descriptor_sets[i];
        layout_binding_bits |= (1 << current_set.set);
        for  (u32 j=0;j < current_set.binding_count; ++j)
        {
            VkDescriptorSetLayoutBinding binding ={0};
            binding.binding = current_set.bindings[j]->binding; 
            binding.descriptorCount = 1;//current_set.bindings[j]->count;
            binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;
            binding.descriptorType = current_set.bindings[j]->descriptor_type;

            dbf_push(desc_set_layout_bindings, binding);

            //u64 full_binding_hash = current_binding.binding | current_binding.descriptorType << 8 | current_binding.descriptorCount << 16 | current_binding.stageFlags << 24;
            u16 binding_hash = current_set.set | (binding.descriptorType << 8); //18 bytes per binding * 4 (MAX) bindings = 64 bits! (one u64, our hash key)
            total_hash |= (binding_hash << (binding.binding * 16));
            //printf("total hash: %lu\n", total_hash);
        }
        total_hash = (dbf_len(desc_set_layout_bindings) << 16) | desc_set_layout_bindings[0].descriptorType; 

        layouts[current_set.set] = dg_descriptor_set_layout_cache_get(&ddev->desc_layout_cache, total_hash);
        //if layout not found in cache, we create it and add it to the cache
        if(layouts[current_set.set] == VK_NULL_HANDLE)
        {
            VkDescriptorSetLayoutCreateInfo desc_layout_ci = {0};
            desc_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            desc_layout_ci.bindingCount = dbf_len(desc_set_layout_bindings);
            desc_layout_ci.pBindings = desc_set_layout_bindings;

            VK_CHECK(vkCreateDescriptorSetLayout(ddev->device, &desc_layout_ci, NULL, &layouts[current_set.set]));
            dg_descriptor_set_layout_cache_add(&ddev->desc_layout_cache, 
                (dgDescriptorSetLayoutInfo){total_hash, desc_set_layout_bindings}, layouts[current_set.set]);
            dlog(NULL, "Created (another) DSL!! :( \n");
        }
        else
        {
            //we don't need the binding array, so we delete it
            dbf_free(desc_set_layout_bindings);
        }

    }
    return layout_binding_bits;
}

static VkDescriptorSetLayout dg_get_descriptor_set_layout(dgDevice *ddev, dgShader*shader, u32 set_num)
{
    VkDescriptorSetLayout layout;
    u64 total_hash = 0;
    if (shader->info.descriptor_set_count == 0)return 0; 

    for(u32 i=0;i< shader->info.descriptor_set_count; ++i)
    {
        VkDescriptorSetLayoutBinding *desc_set_layout_bindings = NULL;
        SpvReflectDescriptorSet current_set = shader->info.descriptor_sets[i];
        if (current_set.set != set_num)continue;
        for  (u32 j=0;j < current_set.binding_count; ++j)
        {
            VkDescriptorSetLayoutBinding binding ={0};
            binding.binding = current_set.bindings[j]->binding; 
            binding.descriptorCount = current_set.bindings[j]->count;
            binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            binding.descriptorType = current_set.bindings[j]->descriptor_type;

            dbf_push(desc_set_layout_bindings, binding);

            //u64 full_binding_hash = current_binding.binding | current_binding.descriptorType << 8 | current_binding.descriptorCount << 16 | current_binding.stageFlags << 24;
            u16 binding_hash = current_set.set | (binding.descriptorType << 8); //18 bytes per binding * 4 (MAX) bindings = 64 bits! (one u64, our hash key)
            total_hash |= (binding_hash << (binding.binding * 16));
            //printf("total hash: %lu\n", total_hash);
        }
        total_hash = (dbf_len(desc_set_layout_bindings) << 16) | desc_set_layout_bindings[0].descriptorType; 

        layout = dg_descriptor_set_layout_cache_get(&ddev->desc_layout_cache, total_hash);
        //if layout not found in cache, we create it and add it to the cache
        if(layout == VK_NULL_HANDLE)
        {
            VkDescriptorSetLayoutCreateInfo desc_layout_ci = {0};
            desc_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            desc_layout_ci.bindingCount = dbf_len(desc_set_layout_bindings);
            desc_layout_ci.pBindings = desc_set_layout_bindings;

            VK_CHECK(vkCreateDescriptorSetLayout(ddev->device, &desc_layout_ci, NULL, &layout));
            dg_descriptor_set_layout_cache_add(&ddev->desc_layout_cache, 
                (dgDescriptorSetLayoutInfo){total_hash, desc_set_layout_bindings}, layout);
            dlog(NULL, "Created (another) DSL!! :( \n");
        }
        else
        {
            //we don't need the binding array, so we delete it
            dbf_free(desc_set_layout_bindings);
        }

    }
    return layout;
}

//this is pretty much empty
static VkPipelineLayoutCreateInfo dg_pipe_layout_create_info(VkDescriptorSetLayout *layouts, u32 layouts_count)
{
    //maybe for each pipeline layout / descriptor set we should string->int hash the members for fast updates
    
	VkPipelineLayoutCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.pNext = NULL;
	
	info.flags = 0;
	info.setLayoutCount = layouts_count;
	info.pSetLayouts = layouts;
    

	return info;
}

static b32 dg_create_pipeline(dgDevice *ddev, dgPipeline *pipe, char *vert_name, char *frag_name, dgPipeOptions pipe_options)
{
    pipe->options = pipe_options;
    //these are dummies, we bind our scissors and viewports per drawcall
    VkRect2D s = scissor(0,0,0,0);
    VkViewport v = viewport(0,0,0,0);


    //read shaders and register them in the pipeline builder
	dg_shader_create(ddev->device, &pipe->vert_shader, vert_name, VK_SHADER_STAGE_VERTEX_BIT); 
	dg_shader_create(ddev->device, &pipe->frag_shader, frag_name, VK_SHADER_STAGE_FRAGMENT_BIT);
	u32 shader_stages_count = 2;
    VkPipelineShaderStageCreateInfo shader_stages[3];
	shader_stages[0] = dg_pipe_shader_stage_create_info(pipe->vert_shader.stage, pipe->vert_shader.module);
	shader_stages[1] = dg_pipe_shader_stage_create_info(pipe->frag_shader.stage, pipe->frag_shader.module);

    u32 output_var_count = pipe->frag_shader.info.output_variable_count;
    //we cut all builtin out variables like gl_FragDepth and stuff
    for (u32 i = 0; i < pipe->frag_shader.info.output_variable_count; ++i)
        if (pipe->frag_shader.info.output_variables[i]->built_in != -1)
            --output_var_count;



 
    VkVertexInputBindingDescription bind_desc[DG_VERTEX_INPUT_ATTRIB_MAX];
    VkVertexInputAttributeDescription attr_desc[DG_VERTEX_INPUT_ATTRIB_MAX];
    VkPipelineVertexInputStateCreateInfo vert_input_state = 
    dg_pipe_vertex_input_state_create_info(&pipe->vert_shader, bind_desc, attr_desc, pipe->options & DG_PIPE_OPTION_PACK_VERTEX_ATTRIBS);



    VkPipelineInputAssemblyStateCreateInfo input_asm_state = 
    dg_pipe_input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    VkPipelineViewportStateCreateInfo viewport_state = 
    dg_pipe_viewport_state_create_info(&v, &s);

    VkPipelineRasterizationStateCreateInfo rasterization_state = 
    dg_pipe_rasterization_state_create_info(VK_POLYGON_MODE_FILL);

    VkPipelineMultisampleStateCreateInfo multisampling_info = 
    dg_pipe_multisampling_state_create_info();

    VkPipelineDepthStencilStateCreateInfo depth_info = {0};

    VkPipelineColorBlendAttachmentState blend_attachment_states[DG_MAX_COLOR_ATTACHMENTS];
    for (u32 i = 0; i < 4; ++i)
    {
        blend_attachment_states[i] = dg_pipe_color_blend_attachment_state(pipe->options & DG_PIPE_OPTION_BLEND);
    }

    VkPipelineColorBlendStateCreateInfo color_blend_state =  
    dg_pipe_color_blend_state_create_info(&blend_attachment_states[0],output_var_count);

    VkPipelineDepthStencilStateCreateInfo ds_state = 
    dg_pipe_depth_stencil_state_create_info_basic();

    VkDynamicState dynamic_state_enables[] =
    {   VK_DYNAMIC_STATE_VIEWPORT, 
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamic_state = { 0 };
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.pNext = NULL;
    dynamic_state.dynamicStateCount = array_count(dynamic_state_enables);
    dynamic_state.pDynamicStates = dynamic_state_enables;

    
    VkDescriptorSetLayout layouts[DG_MAX_DESCRIPTOR_SETS];
    dg_pipe_descriptor_set_layout(ddev, &pipe->vert_shader, layouts);

    VkPipelineLayoutCreateInfo pipe_layout_info = 
    dg_pipe_layout_create_info(layouts, pipe->vert_shader.info.descriptor_set_count);

    VK_CHECK(vkCreatePipelineLayout(ddev->device, &pipe_layout_info, NULL, &pipe->pipeline_layout));


    VkGraphicsPipelineCreateInfo pipeCI = {0};
    pipeCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeCI.stageCount = shader_stages_count;
    pipeCI.pStages = shader_stages;
    pipeCI.pVertexInputState = &vert_input_state;
    pipeCI.pInputAssemblyState = &input_asm_state;
    pipeCI.pViewportState = &viewport_state;
    pipeCI.pRasterizationState = &rasterization_state;
    pipeCI.pMultisampleState = &multisampling_info;
    pipeCI.pDepthStencilState = &ds_state;
    pipeCI.pColorBlendState = &color_blend_state;
    pipeCI.pDynamicState = &dynamic_state;
    pipeCI.layout = pipe->pipeline_layout;

    VkFormat color_formats[DG_MAX_COLOR_ATTACHMENTS];
    for (u32 i = 0; i< DG_MAX_COLOR_ATTACHMENTS; ++i)
    {
        //if only one output, it means we write to swapchain, else, we write in some Render Target, 
        //and all RTs are RGBA16 @FIX this when time
        if (output_var_count > 1)
            color_formats[i] = VK_FORMAT_R16G16B16A16_SFLOAT;
        else 
            color_formats[i] = ddev->swap.image_format;
    }
 
    // New create info to define color, depth and stencil attachments at pipeline create time
    VkPipelineRenderingCreateInfoKHR pipe_renderingCI = {0};
    pipe_renderingCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    pipe_renderingCI.colorAttachmentCount = output_var_count;
    pipe_renderingCI.pColorAttachmentFormats = color_formats;
    pipe_renderingCI.depthAttachmentFormat = ddev->swap.depth_attachment.format;
    pipe_renderingCI.stencilAttachmentFormat = ddev->swap.depth_attachment.format;
    // Chain into the pipeline create info
    pipeCI.pNext = &pipe_renderingCI;


    VK_CHECK(vkCreateGraphicsPipelines(ddev->device, VK_NULL_HANDLE, 1, &pipeCI, NULL, &pipe->pipeline));



    return DSUCCESS;
}

static b32 dg_create_sync_objects(dgDevice *ddev)
{
    ddev->image_available_semaphores = (VkSemaphore*)dalloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    ddev->render_finished_semaphores = (VkSemaphore*)dalloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    ddev->in_flight_fences = (VkFence*)dalloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);
    ddev->images_in_flight = (VkFence*)dalloc(sizeof(VkFence) * ddev->swap.image_count);
    for (u32 i = 0; i < ddev->swap.image_count; ++i)ddev->images_in_flight[i] = VK_NULL_HANDLE;
    
    VkSemaphoreCreateInfo semaphore_info = {0};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkCreateSemaphore(ddev->device, &semaphore_info, NULL, &ddev->image_available_semaphores[i])|| 
            vkCreateSemaphore(ddev->device, &semaphore_info, NULL, &ddev->render_finished_semaphores[i])||
            vkCreateFence(ddev->device, &fence_info, NULL, &ddev->in_flight_fences[i]);
    }

    return DSUCCESS;
}

static b32 dg_create_command_pool(dgDevice *ddev)
{
    dgQueueFamilyIndices queue_family_indices = dg_find_queue_families(ddev->physical_device);
    VkCommandPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = queue_family_indices.graphics_family;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(ddev->device, &pool_info, NULL, &ddev->command_pool));
    return DSUCCESS;
}

static b32 dg_create_command_buffers(dgDevice *ddev)
{
    ddev->command_buffers = (VkCommandBuffer*)dalloc(sizeof(VkCommandBuffer) * ddev->swap.image_count);
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = ddev->command_pool; //where to allocate the buffer from
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = ddev->swap.image_count;
    VK_CHECK(vkAllocateCommandBuffers(ddev->device, &alloc_info, ddev->command_buffers));
    return DSUCCESS;
}
void dg_wait_idle(dgDevice *ddev)
{
    vkDeviceWaitIdle(ddev->device);
}

static void dg_rt_init(dgDevice *ddev, dgRT* rt, u32 color_count, b32 depth, u32 width, u32 height);
static void dg_rt_cleanup(dgDevice *ddev, dgRT* rt);
static void dg_recreate_swapchain(dgDevice *ddev)
{
    vkDeviceWaitIdle(ddev->device);
    //in case of window minimization (w = 0, h = 0) we wait until we get a proper window again
    
    
    dg_cleanup_swapchain(ddev);
    dg_create_swapchain(ddev);
    dg_create_swapchain_image_views(ddev);
    main_window.width = ddev->swap.extent.width;
    main_window.height = ddev->swap.extent.height;

    //(optional): recreate the deferred render target so it can be scaled
    dg_rt_cleanup(ddev, &def_rt);
    dg_rt_init(&dd, &def_rt, 4, TRUE, ddev->swap.extent.width, ddev->swap.extent.height);

    
    dg_create_command_buffers(ddev);
}


static void dg_image_memory_barrier(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkAccessFlags srcAccessMask,
			VkAccessFlags dstAccessMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask,
			VkImageSubresourceRange subresourceRange)
		{
			VkImageMemoryBarrier imageMemoryBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
			imageMemoryBarrier.srcAccessMask = srcAccessMask;
			imageMemoryBarrier.dstAccessMask = dstAccessMask;
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;

			vkCmdPipelineBarrier(
				cmdbuffer,
				srcStageMask,
				dstStageMask,
				0,
				0, NULL,
				0, NULL,
				1, &imageMemoryBarrier);
		}


static void dg_prepare_command_buffer(dgDevice *ddev, VkCommandBuffer c)
{
    VkCommandBufferBeginInfo begin_info = {0};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = 0;
	//vkBeginCommandBuffer(command_buffer, &begin_info);
    
    vkBeginCommandBuffer(c, &begin_info);

    //transition color texture for drawing
    dg_image_memory_barrier(
        c,
        ddev->swap.images[ddev->image_index], 
        0, 
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        (VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    );


    //transition depth texture
    dg_image_memory_barrier(
        c,
        ddev->swap.depth_attachment.image, 
        0, 
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,  //@check: should this be LAYOUT_GENERAL mb???
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        (VkImageSubresourceRange){ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 }
    );



}

static void dg_end_command_buffer(dgDevice *ddev, VkCommandBuffer c)
{
    dg_image_memory_barrier(
        c,
        dd.swap.images[dd.image_index], 
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        0, 
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        (VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    );

    vkEndCommandBuffer(c);

}

static void dg_flush_command_buffer(dgDevice *ddev, VkCommandBuffer command_buffer)
{
    VK_CHECK(vkEndCommandBuffer(command_buffer));

    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &command_buffer;

    VkFenceCreateInfo fenceCI= {0};
    fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCI.flags = 0;
    VkFence fence;
    VK_CHECK(vkCreateFence(ddev->device, &fenceCI, NULL, &fence));

    VK_CHECK(vkQueueSubmit(ddev->graphics_queue, 1, &submitInfo, fence));
    VK_CHECK(vkWaitForFences(ddev->device, 1, &fence, VK_TRUE, INT64_MAX));

    vkDestroyFence(ddev->device, fence, NULL);
    vkFreeCommandBuffers(ddev->device, ddev->command_pool, 1, &command_buffer);
}

static void dg_descriptor_allocator_init(dgDevice *ddev, dgDescriptorAllocator *da)
{
    da->device = ddev->device;
    da->current_pool = VK_NULL_HANDLE;
    da->used_pool_count = 0;
    da->free_pool_count = 0;
    u32 pool_index = 0;
    da->pool_sizes[0] = 0.5f;
    da->pool_sizes[1] = 1.0f;
    da->pool_sizes[2] = 2.0f;
    da->pool_sizes[3] = 4.0f;
    


    H32_static_init(&da->desc_type_hash, 16);
    H32_static_set(&da->desc_type_hash, VK_DESCRIPTOR_TYPE_SAMPLER, 0);    
    H32_static_set(&da->desc_type_hash, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3);    
    H32_static_set(&da->desc_type_hash, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 3);    
    H32_static_set(&da->desc_type_hash, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1);    
    H32_static_set(&da->desc_type_hash, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1);    
    H32_static_set(&da->desc_type_hash, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1);    
    H32_static_set(&da->desc_type_hash, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2);    
    H32_static_set(&da->desc_type_hash, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2);    
    H32_static_set(&da->desc_type_hash, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1);    
    H32_static_set(&da->desc_type_hash, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1);    
    H32_static_set(&da->desc_type_hash, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0);    
}

static void dg_descriptor_allocator_cleanup(dgDescriptorAllocator *da)
{
    for (u32 i = 0; i< da->free_pool_count; ++i)
    {
        vkDestroyDescriptorPool(da->device, da->free_pools[i], NULL);
    }
    for (u32 i = 0; i< da->used_pool_count; ++i)
    {
        vkDestroyDescriptorPool(da->device, da->used_pools[i], NULL);
    }
    H32_static_clear(&da->desc_type_hash);
    //free(da->pool_sizes);
}

//this is TERRIBLY SLOWWW~!!!!! @FIX (maybe we don't need to fix because a single pool handles a lot of descriptors??? @inspect)
static VkDescriptorPool dg_create_descriptor_pool(dgDescriptorAllocator *da, u32 count, VkDescriptorPoolCreateFlags flags)
{
    VkDescriptorPoolSize *sizes = NULL;
    for (u32 i = 0; i <= 10; ++i)
    {
        dbf_push(sizes, (VkDescriptorPoolSize){i, count * da->pool_sizes[H32_static_get(&da->desc_type_hash, i)]});
    }

    VkDescriptorPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = flags | VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = count;
    pool_info.poolSizeCount = 11;
    pool_info.pPoolSizes = sizes;
    
    VkDescriptorPool descriptor_pool;
    VK_CHECK(vkCreateDescriptorPool(da->device, &pool_info, NULL, &descriptor_pool));

    assert(descriptor_pool != VK_NULL_HANDLE);
    return descriptor_pool;
}

static VkDescriptorPool dg_descriptor_allocator_grab_pool(dgDescriptorAllocator *da)
{
    if (da->free_pool_count > 0)
    {
        //we get the last free pool and shrink the array to not contain it
        VkDescriptorPool pool = da->free_pools[--da->free_pool_count];
        return pool;
    }
    else
    {
        //if we have no available (free) pools, we create a new one and get that one
        return dg_create_descriptor_pool(da, 1000, 0);
    }
}

static b32 dg_descriptor_allocator_allocate(dgDescriptorAllocator *da, VkDescriptorSet *set, VkDescriptorSetLayout layout)
{
    if (da->current_pool == VK_NULL_HANDLE)
    {
        da->current_pool = dg_descriptor_allocator_grab_pool(da);
        da->used_pools[da->used_pool_count++] = da->current_pool;
    }

    VkDescriptorSetAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.pSetLayouts = &layout;
    alloc_info.descriptorPool = da->current_pool;
    alloc_info.descriptorSetCount = 1;

    //this allocation may fail if the descriptor pool isn't large enough
    VkResult alloc_result = vkAllocateDescriptorSets(da->device, &alloc_info, set);
    b32 need_realloc = FALSE;

    switch(alloc_result)
    {
        case VK_SUCCESS:
            //just return everything is OK
            return TRUE;
        case VK_ERROR_FRAGMENTED_POOL:
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            //reallocate the pool!
            need_realloc = TRUE;
            break;
        default:
            return FALSE;
    }

    if (need_realloc)
    {
        da->current_pool = dg_descriptor_allocator_grab_pool(da);        
        da->used_pools[da->used_pool_count++] = da->current_pool;
        alloc_result = vkAllocateDescriptorSets(da->device, &alloc_info, set);

        if (alloc_result == VK_SUCCESS)
            return TRUE;
    }
    return FALSE;
}

static void dg_descriptor_allocator_reset_pools(dgDescriptorAllocator *da)
{
    for (u32 i = 0; i < da->used_pool_count; ++i)
    {
        vkResetDescriptorPool(da->device, da->used_pools[i], 0);
        da->free_pools[da->free_pool_count++] = da->used_pools[i];
    }
    da->used_pool_count = 0;
    da->current_pool = VK_NULL_HANDLE;
}


void dg_rendering_begin(dgDevice *ddev, dgTexture *tex, u32 attachment_count, dgTexture *depth_tex, b32 clear_color, b32 clear_depth)
{
    VkRenderingAttachmentInfoKHR color_attachments[DG_MAX_COLOR_ATTACHMENTS];
    VkAttachmentLoadOp cload_op = (clear_color > 0) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    memset(color_attachments, 0, sizeof(VkRenderingAttachmentInfoKHR) * DG_MAX_COLOR_ATTACHMENTS);

    if (tex == NULL)
    {
        color_attachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        color_attachments[0].imageView = dd.swap.image_views[dd.image_index];
        color_attachments[0].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        color_attachments[0].loadOp = cload_op;
        color_attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachments[0].clearValue.color = (VkClearColorValue){0.f,0.f,0.f,0.f};
    }
    else
    {
        for (u32 i = 0; i < attachment_count; ++i)
        {
            color_attachments[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            color_attachments[i].imageView = tex[i].view;
            color_attachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            color_attachments[i].loadOp = cload_op;
            color_attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            color_attachments[i].clearValue.color = (VkClearColorValue){0.f,0.f,0.f,0.f};
        }
    }

    VkAttachmentLoadOp dload_op = (clear_depth> 0) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    VkRenderingAttachmentInfoKHR depth_attachment = {0};
    depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR; 
    depth_attachment.pNext = NULL; 
    if (depth_tex == NULL)
        depth_attachment.imageView = ddev->swap.depth_attachment.view;
    else
        depth_attachment.imageView = depth_tex->view;
    depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

    //depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.loadOp = dload_op;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.clearValue.depthStencil = (VkClearDepthStencilValue){1.0f, 0.0f};


    VkRenderingInfoKHR rendering_info = {0};
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    if (tex == NULL)
        rendering_info.renderArea = (VkRect2D){0,0, dd.swap.extent.width, dd.swap.extent.height};
    else
        rendering_info.renderArea = (VkRect2D){0,0, tex->width,tex->height};
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = (tex == NULL) ? 1 : attachment_count;
    rendering_info.pColorAttachments = color_attachments;
    rendering_info.pDepthAttachment = &depth_attachment;
    //rendering_info.pStencilAttachment = &depth_attachment;
    rendering_info.pStencilAttachment = NULL; //TODO: this should be NULL only if depth+stencil=depth

    vkCmdBeginRenderingKHR(ddev->command_buffers[ddev->current_frame], &rendering_info);
}

void dg_rendering_end(dgDevice *ddev)
{
    vkCmdEndRenderingKHR(ddev->command_buffers[ddev->current_frame]);
}


//attaches ALLOCATED memory block to buffer!
void dg_buf_bind(dgBuffer *buf, VkDeviceSize offset)
{
	vkBindBufferMemory(dd.device, buf->buffer, buf->mem, offset);
}

void dg_buf_copy_to(dgBuffer *src,void *data, VkDeviceSize size)
{
	assert(src->mapped);
	memcpy(src->mapped, data, size);
}

void dg_buf_setup_descriptor(dgBuffer *buf, VkDeviceSize size, VkDeviceSize offset)
{
	buf->desc.offset = offset;
	buf->desc.buffer = buf->buffer;
	buf->desc.range = size;
}
VkResult dg_buf_map(dgBuffer *buf, VkDeviceSize size, VkDeviceSize offset)
{
    //printf("buf->mem = %i\n\n", buf->mem);
    //fflush(stdout);
	return vkMapMemory(dd.device, buf->mem, offset, size, 0, &buf->mapped);//@check @check @check @check
}

void dg_buf_unmap(dgBuffer *buf)
{
	if (buf->mapped)
	{
		vkUnmapMemory(dd.device, buf->mem);
		buf->mapped = NULL;
	}
}

void dg_buf_destroy(dgBuffer *buf)
{
    if (!buf->active)return;
	if (buf->buffer)
	{
		vkDestroyBuffer(dd.device, buf->buffer, NULL);
	}
	if (buf->mem)
	{
		vkFreeMemory(dd.device, buf->mem, NULL);
	}
    buf->active = FALSE;
}

#define TYPE_OK(TYPE_FILTER, FILTER_INDEX) (TYPE_FILTER & (1 << FILTER_INDEX)) 
static u32 find_mem_type(u32 type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(dd.physical_device, &mem_properties);
    for (u32 i = 0; i < mem_properties.memoryTypeCount; ++i)
    {
        if (TYPE_OK(type_filter, i) && 
            (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    dlog(NULL, "Failed to find suitable memory type!");
    return 0;
}

void dg_create_buffer(VkBufferUsageFlagBits usage, VkMemoryPropertyFlagBits mem_flags, dgBuffer*buf, VkDeviceSize size, void *data)
{
	buf->device = dd.device;
    buf->active = TRUE;
	
	//[0]: create buffer handle
    VkBufferCreateInfo buffer_info = {0};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateBuffer(dd.device, &buffer_info, NULL, &(buf->buffer) ));
    
	//[1]: create the memory nacking up the buffer handle
    VkMemoryRequirements mem_req = {0};
    vkGetBufferMemoryRequirements(buf->device, (buf->buffer), &mem_req);
    
    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_req.size;
    alloc_info.memoryTypeIndex = find_mem_type(mem_req.memoryTypeBits, mem_flags);
    VK_CHECK(vkAllocateMemory(dd.device, &alloc_info, NULL, &buf->mem));
	
	//[2]: set some important data fields
	buf->alignment = mem_req.alignment;
	buf->size = size;
	buf->usage_flags = usage;
	//buf->memory_property_flags = properties; //HOST_COHERENT HOST_VISIBLE etc.
	
	//[3]:if data pointer has data, we map the buffer and copy those data over to OUR buffer
	if (data != NULL)
	{
		VK_CHECK(dg_buf_map(buf, size, 0));
		memcpy(buf->mapped, data, size);
		dg_buf_unmap(buf);
	}
	
	//[4]: we initialize a default descriptor that covers the whole buffer size
	dg_buf_setup_descriptor(buf, size, 0);
	
	//[5]: we attach the memory to the buffer object
    vkBindBufferMemory(dd.device, (buf->buffer), (buf->mem), 0);
}


static VkFormat dg_find_supported_format(dgDevice *ddev, VkFormat *candidates, VkImageTiling tiling, VkFormatFeatureFlags features, u32 candidates_count)
{
	for (u32 i = 0; i < candidates_count; ++i)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(ddev->physical_device, candidates[i], &props);
		
		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return candidates[i];
		}else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return candidates[i];
		}
	}
	printf("Couldn't find desired depth buffer format!");
    return 0;
}


static u32 dg_has_stencil_component(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
static VkFormat dg_find_depth_format(dgDevice *ddev)
{	VkFormat c[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
	return dg_find_supported_format(
                                ddev,
                                c, VK_IMAGE_TILING_OPTIMAL,
                                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,3);
    
}

static void dg_create_image(dgDevice *ddev, u32 width, u32 height, VkFormat format,u32 layers, VkImageTiling tiling, 
VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *image_memory)
{
	VkImageCreateInfo image_info = {0};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = layers;
	image_info.format = format;
	image_info.tiling = tiling;
	image_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	image_info.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.flags = 0;
	VK_CHECK(vkCreateImage(ddev->device, &image_info, NULL, image));
	
	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(ddev->device, *image, &mem_req);
	
	VkMemoryAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = find_mem_type(mem_req.memoryTypeBits, properties);
    VK_CHECK(vkAllocateMemory(ddev->device, &alloc_info, NULL, image_memory));
	vkBindImageMemory(ddev->device, *image, *image_memory, 0);
}


static VkCommandBuffer dg_begin_single_time_commands(dgDevice *ddev)
{
	VkCommandBufferAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = ddev->command_pool;
	alloc_info.commandBufferCount = 1;
	
	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(ddev->device, &alloc_info, &command_buffer);
	
	VkCommandBufferBeginInfo begin_info = {0};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(command_buffer, &begin_info);
	
	return command_buffer;
}

static void dg_end_single_time_commands(dgDevice *ddev, VkCommandBuffer command_buffer)
{
	vkEndCommandBuffer(command_buffer);
	
	VkSubmitInfo submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;
	
	vkQueueSubmit(ddev->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(ddev->graphics_queue);
	vkFreeCommandBuffers(ddev->device, ddev->command_pool, 1, &command_buffer);
}

static dgTexture dg_create_depth_attachment(dgDevice *ddev, u32 width, u32 height, u32 layer_count)
{
	dgTexture depth_attachment = {0};
	depth_attachment.format = dg_find_depth_format(ddev);
	
	dg_create_image(ddev,width, height, 
		depth_attachment.format, layer_count,VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depth_attachment.image, &depth_attachment.mem);

    if (layer_count > 1)
        depth_attachment.view = dg_create_image_view(depth_attachment.image, depth_attachment.format,VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_DEPTH_BIT,layer_count,0);
    else
        depth_attachment.view = dg_create_image_view(depth_attachment.image, depth_attachment.format,VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_DEPTH_BIT,1,0);
    depth_attachment.width = width;
    depth_attachment.height = height;
    depth_attachment.image_layout = VK_IMAGE_LAYOUT_GENERAL; //@FIX: why general ??????

    //transition depth image to layout general (because im lazy :D)
    VkCommandBuffer cmd = dg_begin_single_time_commands(ddev);
    dg_image_memory_barrier(
        cmd,
        depth_attachment.image,
        VK_ACCESS_HOST_WRITE_BIT, 
        VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_PREINITIALIZED,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        (VkImageSubresourceRange){ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, layer_count }
    );
    dg_end_single_time_commands(ddev, cmd);

    dg_create_texture_sampler(ddev, &depth_attachment.sampler);
    
	return depth_attachment;
}

dgTexture dg_create_texture_image_wdata(dgDevice *ddev,void *data, u32 tex_w,u32 tex_h, VkFormat format)
{
    dgTexture tex;
	dgBuffer idb;
	dg_create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
	(VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), &idb, tex_w * tex_h * sizeof(u8), data);
	dg_create_image(ddev, tex_w, tex_h, format, 1,VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_DST_BIT 
		| VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &tex.image, &tex.mem);
	

    VkCommandBuffer cmd = dg_begin_single_time_commands(ddev);
    dg_image_memory_barrier(
        cmd,
        tex.image,
        VK_ACCESS_HOST_WRITE_BIT, 
        VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_PREINITIALIZED,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        (VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    );


    //if there is data to be copied, copy it
    if (data != NULL)
    {
        VkBufferImageCopy region = {0};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = (VkOffset3D){0, 0, 0};
        region.imageExtent = (VkExtent3D){
            tex_w,
            tex_h,
            1
        };
        vkCmdCopyBufferToImage(
            cmd,
            idb.buffer,
            tex.image,
            VK_IMAGE_LAYOUT_GENERAL,
            1,
            &region
        );
    }
    
    dg_end_single_time_commands(ddev, cmd);

	dg_buf_destroy(&idb);
	
	
	dg_create_texture_sampler(ddev, &tex.sampler);
	
	tex.view = dg_create_image_view(tex.image, format, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT,1,0);
	tex.mip_levels = 0;
	tex.width = tex_w;
	tex.height = tex_h;
    tex.image_layout = VK_IMAGE_LAYOUT_GENERAL;
	return tex;
}

dgTexture dg_create_texture_image(dgDevice *ddev, char *filename, VkFormat format)
{
	dgTexture tex;
	//[0]: we read an image and store all the pixels in a pointer
	s32 tex_w, tex_h, tex_c;
	stbi_uc *pixels = stbi_load(filename, &tex_w, &tex_h, &tex_c, STBI_rgb_alpha);
	VkDeviceSize image_size = tex_w * tex_h * 4;
	
	
	//[2]: we create a buffer to hold the pixel information (we also fill it)
	dgBuffer idb;
	if (!pixels)
		printf("Error loading image %s!", filename);
	dg_create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
	(VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), &idb, image_size, pixels);
	//[3]: we free the cpu side image, we don't need it
	stbi_image_free(pixels);
	//[4]: we create the VkImage that is undefined right now
	dg_create_image(ddev, tex_w, tex_h, format, 1,VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_DST_BIT 
		| VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &tex.image, &tex.mem);
	

    VkCommandBuffer cmd = dg_begin_single_time_commands(ddev);
    dg_image_memory_barrier(
        cmd,
        tex.image,
        VK_ACCESS_HOST_WRITE_BIT, 
        VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_PREINITIALIZED,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        (VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    );




    VkBufferImageCopy region = {0};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = (VkOffset3D){0, 0, 0};
	region.imageExtent = (VkExtent3D){
		tex_w,
		tex_h,
		1
	};
	vkCmdCopyBufferToImage(
		cmd,
		idb.buffer,
		tex.image,
		VK_IMAGE_LAYOUT_GENERAL,
		1,
		&region
	);
    
    dg_end_single_time_commands(ddev, cmd);
    tex.width = tex_w;
    tex.height = tex_w;

	dg_buf_destroy(&idb);
	
	
	dg_create_texture_sampler(ddev, &tex.sampler);
	
	tex.view = dg_create_image_view(tex.image, format,VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT,1,0);
	tex.mip_levels = 0;
	tex.width = tex_w;
	tex.height = tex_h;
    tex.image_layout = VK_IMAGE_LAYOUT_GENERAL;
	
	sprintf(tex.name, filename);
	return tex;
}

static void dg_rt_init_csm(dgDevice *ddev, dgRT* rt,u32 cascade_count, u32 width, u32 height)
{
    rt->color_attachment_count = cascade_count;
    rt->depth_active = TRUE;
    rt->cascaded_depth = TRUE;
    rt->depth_attachment = dg_create_depth_attachment(ddev, width,height,cascade_count);
    for (u32 i =0; i < cascade_count; ++i)
    {
        rt->cascade_views[i] = dg_create_image_view(rt->depth_attachment.image, rt->depth_attachment.format, 
        VK_IMAGE_VIEW_TYPE_2D_ARRAY,VK_IMAGE_ASPECT_DEPTH_BIT,1,i);
    }
}

static void dg_rt_init(dgDevice *ddev, dgRT* rt, u32 color_count, b32 depth, u32 width, u32 height)
{
    rt->color_attachment_count = color_count;
    rt->depth_active = (depth > 0) ? 1 : 0;
    for (u32 i = 0; i < rt->color_attachment_count; ++i)
    {
        //rt->color_attachments[i] = dg_create_texture_image_basic(ddev,width,height,ddev->swap.image_format);
        //rt->color_attachments[i] = dg_create_texture_image_basic(ddev,width,height,VK_FORMAT_R16G16B16A16_SFLOAT);
        rt->color_attachments[i] = dg_create_texture_image_wdata(ddev, NULL, width, height,VK_FORMAT_R16G16B16A16_SFLOAT);
    }
    if (rt->depth_active)
        //rt->depth_attachment = dg_create_depth_attachment(ddev, 1024, 1024);
        rt->depth_attachment = dg_create_depth_attachment(ddev, width,height,1);
}
static void dg_rt_cleanup(dgDevice *ddev, dgRT* rt)
{
    for (u32 i = 0; i < rt->color_attachment_count; ++i)
    {
        dg_cleanup_texture(ddev,&rt->color_attachments[i]);
    }
    if (rt->depth_active)
        dg_cleanup_texture(ddev,&rt->depth_attachment);
}


static void dg_ubo_data_buffer_clear(dgUBODataBuffer *buf, u32 frame_num)
{
    buf->buffer_offsets[frame_num] = 0;
}

static dgBuffer* dg_ubo_data_buffer_get_buf(dgDevice *ddev, dgUBODataBuffer *buf)
{
    return &buf->buffers[ddev->current_frame];
}


static b32 dg_ubo_data_buffer_init(dgDevice *ddev, dgUBODataBuffer *buf, u32 buffer_size)
{
    for (u32 i = 0; i< MAX_FRAMES_IN_FLIGHT; ++i)
    {
        buf->buffer_offsets[i] = 0;
        dg_create_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
        (VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
        &buf->buffers[i], buffer_size, NULL);
    }
    return TRUE;
}
static u32 dg_ubo_data_buffer_copy(dgDevice *ddev, dgUBODataBuffer *buf, void *src, u32 size)
{
    dgBuffer *buffer = &buf->buffers[ddev->current_frame];
    VK_CHECK(dg_buf_map(buffer, size, buf->buffer_offsets[ddev->current_frame]));
    memcpy(buffer->mapped, src, size);
    dg_buf_unmap(buffer);
    u32 copy_offset = buf->buffer_offsets[ddev->current_frame];
    buf->buffer_offsets[ddev->current_frame] += size;
    return copy_offset;

}

static void dg_update_desc_set(dgDevice *ddev, VkDescriptorSet set, void *data, u32 size)
{
    u32 offset = dg_ubo_data_buffer_copy(ddev, &ddev->ubo_buf, data, size);
    VkDescriptorBufferInfo ubo_info = {dg_ubo_data_buffer_get_buf(ddev, &ddev->ubo_buf)->buffer, offset, size};

    VkWriteDescriptorSet set_write = {0};
    set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set_write.dstBinding = 0;
    set_write.dstSet = set;
    set_write.descriptorCount = 1;
    set_write.descriptorType= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    set_write.pBufferInfo = &ubo_info;

    vkUpdateDescriptorSets(ddev->device, 1, &set_write, 0, NULL);
}

static void dg_update_desc_set_image(dgDevice *ddev, VkDescriptorSet set, dgTexture *textures, u32 view_count)
{
    VkDescriptorImageInfo image_infos[DG_MAX_DESCRIPTOR_SET_BINDINGS];
    for (u32 i = 0; i < view_count; ++i)
    {
        memset(&image_infos[i], 0, sizeof(VkDescriptorImageInfo));
        image_infos[i].imageView = textures[i].view;
        image_infos[i].sampler = textures[i].sampler;
        image_infos[i].imageLayout = textures[i].image_layout;
    }

    VkWriteDescriptorSet set_write = {0};
    set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set_write.dstBinding = 0;
    set_write.dstSet = set;
    set_write.descriptorCount = view_count;
    set_write.descriptorType= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    set_write.pImageInfo = image_infos;

    vkUpdateDescriptorSets(ddev->device, 1, &set_write, 0, NULL);
}
void dg_set_scissor(dgDevice *ddev,f32 x, f32 y, f32 width, f32 height) 
{
    VkRect2D sci = scissor(x, y, width,height);
    vkCmdSetScissor(ddev->command_buffers[ddev->current_frame], 0, 1, &sci);
}
void dg_set_viewport(dgDevice *ddev,f32 x, f32 y, f32 width, f32 height)
{
    VkViewport view = viewport(x,y,width,height);
    vkCmdSetViewport(dd.command_buffers[ddev->current_frame], 0, 1, &view);
}


void dg_get_frustum_cornersWS(vec4 *corners, mat4 proj, mat4 view)
{
    mat4 inv = mat4_inv(mat4_mul(proj, view));
    u32 corner_counter = 0;
    for (u32 x = 0; x < 2; ++x)
    {
        for (u32 y = 0; y < 2; ++y)
        {
            for (u32 z = 0; z < 2; ++z)
            {
                vec4 pt = mat4_mulv(inv, v4(2.0f * x - 1.0f, 2.0f * y - 1.0f, z, 1.0f));
                corners[corner_counter++] = vec4_mulf(pt, 1.0f/pt.w);;
            }
        }
    }
    assert(corner_counter == 8);
}

void dg_set_desc_set(dgDevice *ddev,dgPipeline *pipe, void *data, u32 size, u32 set_num)
{ 
    //first we get the layout, then we 
    VkDescriptorSet desc_set;
    VkDescriptorSetLayout layout;
    layout = dg_get_descriptor_set_layout(ddev, &pipe->vert_shader, set_num);
    dg_descriptor_allocator_allocate(&ddev->desc_alloc[ddev->current_frame], &desc_set, layout);
    if(set_num == 2) //because set number 2 is for images
    {
        dgTexture *tex_data = (dgTexture*)data;
        u32 tex_count = size;
        dg_update_desc_set_image(ddev, desc_set, tex_data, size);
    }
    else
    {
        dg_update_desc_set(ddev, desc_set, data, size);
    }
    vkCmdBindDescriptorSets(ddev->command_buffers[ddev->current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->pipeline_layout, set_num,1, &desc_set,0,NULL); 
}

void dg_bind_pipeline(dgDevice *ddev, dgPipeline *pipe)
{
    vkCmdBindPipeline(ddev->command_buffers[ddev->current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->pipeline);
}
//maybe usea static array... dynamic array slow!
void dg_bind_vertex_buffers(dgDevice *ddev, dgBuffer* vbo, u64 *offsets, u32 vbo_count)
{
    for (u32 i = 0; i < vbo_count; ++i)
        vkCmdBindVertexBuffers(ddev->command_buffers[ddev->current_frame], i, 1, &vbo[i].buffer, &offsets[i]);
}
void dg_bind_index_buffer(dgDevice *ddev, dgBuffer* ibo, u32 ibo_offset)
{
    vkCmdBindIndexBuffer(ddev->command_buffers[ddev->current_frame], ibo->buffer, ibo_offset, VK_INDEX_TYPE_UINT16);
}

void dg_draw(dgDevice *ddev, u32 vertex_count,u32 index_count)
{

    if(index_count)
        vkCmdDrawIndexed(ddev->command_buffers[ddev->current_frame], index_count, 1, 0, 0, 0);
    else
        vkCmdDraw(ddev->command_buffers[ddev->current_frame], vertex_count, 1, 0, 0);
}

void draw_cube_def(dgDevice *ddev, mat4 model,vec4 col0, vec4 col1)
{

    dg_rendering_begin(ddev, def_rt.color_attachments, 3, &def_rt.depth_attachment, FALSE, FALSE);
    dg_set_viewport(ddev, 0,0,def_rt.color_attachments[0].width, def_rt.color_attachments[0].height);
    dg_set_scissor(ddev, 0,0,def_rt.color_attachments[0].width, def_rt.color_attachments[0].height);
    dg_bind_pipeline(ddev, &ddev->def_pipe);
    dgBuffer buffers[] = {base_vbo};
    u64 offsets[] = {0};
    dg_bind_vertex_buffers(ddev, buffers, offsets, 1);
    dg_bind_index_buffer(ddev, &base_ibo, 0);

    mat4 data[2] = {model, (mat4){col0.x,col0.y,col0.z,col0.w,col1.x,col1.y,col1.z,col1.w}};
    dg_set_desc_set(ddev,&ddev->def_pipe, data, sizeof(mat4) + 2 * sizeof(vec4), 1);
    dg_draw(ddev, 24,base_ibo.size/sizeof(u16));

    dg_rendering_end(ddev);

}

/*
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
*/

void draw_cube_def_shadow(dgDevice *ddev, mat4 model, mat4 lsm, u32 cascade_index)
{
    if (!ddev->shadow_pass_active)return;
    
    dgTexture depth_tex_to_write = csm_rt.depth_attachment;
    depth_tex_to_write.view = csm_rt.cascade_views[cascade_index];
    dg_rendering_begin(ddev, shadow_rt.color_attachments, 0, &depth_tex_to_write, FALSE, FALSE);
    dg_set_viewport(ddev, 0,0,shadow_rt.color_attachments[0].width, shadow_rt.color_attachments[0].height);
    dg_set_scissor(ddev, 0,0,shadow_rt.color_attachments[0].width, shadow_rt.color_attachments[0].height);
    dg_bind_pipeline(ddev, &ddev->shadow_pipe);
    dgBuffer buffers[] = {base_vbo};
    u64 offsets[] = {0};
    dg_bind_vertex_buffers(ddev, buffers, offsets, 1);
    dg_bind_index_buffer(ddev, &base_ibo, 0);

    mat4 object_data[2] = {model, lsm};
    dg_set_desc_set(ddev,&ddev->shadow_pipe, object_data, sizeof(object_data), 1);
    dg_draw(ddev, 24,base_ibo.size/sizeof(u16));
    dg_rendering_end(ddev);
}

//calculates the cascaded shadow map's light space matrix
static void dg_calc_lsm(vec3 ld, mat4 proj, mat4 view, mat4 *lsm, f32 *frustum_dists,u32 frustum_count)
{
    frustum_count = minimum(frustum_count, DG_MAX_CASCADES);
    u32 current_frustum = 0;
    f32 near_plane  = proj.elements[3][2]/proj.elements[2][2];
    f32 far_plane = (near_plane * proj.elements[2][2])/(1+proj.elements[2][2]);
    f32 cascade_limits[DG_MAX_CASCADES+1];
    cascade_limits[0] = near_plane;
    cascade_limits[frustum_count] = far_plane;
    for(u32 i = 1; i < frustum_count; ++i)
        cascade_limits[i] =  
            cascade_limits[0]+(cascade_limits[frustum_count]-cascade_limits[0])/((frustum_count+1) - i);
    
    for (u32 f=0; f < frustum_count; ++f)
    {
        f32 cascade_near = cascade_limits[f]; 
        f32 cascade_far = cascade_limits[f+1]; 

        mat4 cascade_proj = proj;
        cascade_proj.elements[2][2] =cascade_far/(cascade_near-cascade_far);
        cascade_proj.elements[3][2] =(cascade_near* cascade_far)/(cascade_near-cascade_far);
        
        vec4 corners[8];
        dg_get_frustum_cornersWS(corners, cascade_proj, view);
        vec3 frustum_center = v3(0,0,0);
        for (u32 i = 0; i < 8; ++i)
        {
            frustum_center = vec3_add(frustum_center, v3(corners[i].x, corners[i].y, corners[i].z));
        }
        frustum_center= vec3_mulf(frustum_center, 1.0f/8.0f);

        mat4 light_view = look_at(frustum_center,vec3_sub(frustum_center,ld),v3(0,1,0));


        f32 minZ = FLT_MAX;
        f32 minX = FLT_MAX;
        f32 minY = FLT_MAX;
        f32 maxY =-FLT_MAX;
        f32 maxX =-FLT_MAX;
        f32 maxZ =-FLT_MAX;
        for (u32 i = 0; i < 8;++i)
        {
            //find in terms of the light's  view matrix, what are the max and min coordinates of the Frustum
            vec4 trf = mat4_mulv(light_view, v4(corners[i].x, corners[i].y, corners[i].z, 1.0f));
            minX = minimum(minX, trf.x);
            minY = minimum(minY, trf.y);
            minZ = minimum(minZ, trf.z);
            maxX = maximum(maxX, trf.x);
            maxY = maximum(maxY, trf.y);
            maxZ = maximum(maxZ, trf.z);
        }
        float z_mul = 10.0f;
        if (minZ < 0)
        {
            minZ *= z_mul;
        }
        else
        {
            minZ /= z_mul;
        }

        if (maxZ < 0)
        {
            maxZ /= z_mul;
        }
        else
        {
            maxZ *= z_mul;
        }

        mat4 light_ortho = orthographic_proj(minX, maxX, minY, maxY, minZ, maxZ);

        frustum_dists[current_frustum] = cascade_far;
        lsm[current_frustum++] = mat4_mul(light_ortho, light_view);
    }
}

extern dEntity child, parent;
extern dTransformCM transform_manager;
void draw_cube(dgDevice *ddev, mat4 model)
{
    dg_rendering_begin(ddev, NULL, 1, &def_rt.depth_attachment, FALSE, FALSE);
    dg_set_viewport(ddev, 0,0,ddev->swap.extent.width, ddev->swap.extent.height);
    dg_set_scissor(ddev, 0,0,ddev->swap.extent.width, ddev->swap.extent.height);
    dg_bind_pipeline(ddev, &ddev->base_pipe);
    dgBuffer buffers[] = {base_vbo};
    u64 offsets[] = {0,0,0,0};
    dg_bind_vertex_buffers(ddev, buffers, offsets, 1);
    dg_bind_index_buffer(ddev, &base_ibo, 0);

    //mat4 data[4] = {0.9,(sin(0.02 * dtime_sec(dtime_now()))),0.2,0.2};
    //mat4 object_data = mat4_mul(mat4_translate(v3(0,1 * fabs(sin(5 * dtime_sec(dtime_now()))),-15)), mat4_rotate(90 * dtime_sec(dtime_now()), v3(0.2,0.4,0.7)));
    mat4 object_data[2] = {model, {1.0,1.0,1.0,1.1,0.0,1.0}};
    dg_set_desc_set(ddev,&ddev->base_pipe, object_data, sizeof(object_data), 1);
    dg_draw(ddev, 24,base_ibo.size/sizeof(u16));

    dg_rendering_end(ddev);
}


void dg_frame_begin(dgDevice *ddev)
{
    vkWaitForFences(ddev->device, 1, &ddev->in_flight_fences[ddev->current_frame], VK_TRUE, UINT64_MAX);
    vkResetFences(ddev->device, 1, &ddev->in_flight_fences[ddev->current_frame]);



    VkResult res = vkAcquireNextImageKHR(ddev->device, ddev->swap.swapchain, UINT64_MAX, ddev->image_available_semaphores[ddev->current_frame],
        VK_NULL_HANDLE, &ddev->image_index);

    if (res == VK_ERROR_OUT_OF_DATE_KHR) { dg_recreate_swapchain(ddev); return; }
    else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)printf("Failed to acquire swapchain image!\n");

    vkResetCommandBuffer(ddev->command_buffers[ddev->current_frame],0);



    dg_ubo_data_buffer_clear(&ddev->ubo_buf, ddev->current_frame);
    dg_descriptor_allocator_reset_pools(&ddev->desc_alloc[ddev->current_frame]);

    dg_prepare_command_buffer(ddev, ddev->command_buffers[ddev->current_frame]);
    //CLEAR ALL FBO's before drawing
    
    //clear deferred FBO
    dg_rendering_begin(ddev, def_rt.color_attachments, 3, &def_rt.depth_attachment, TRUE, TRUE);
    dg_rendering_end(ddev);
    //clear the shadowmap
    dgTexture depth_tex_to_write = csm_rt.depth_attachment;
    for (u32 i = 0; i < DG_MAX_CASCADES; ++i)
    {
        depth_tex_to_write.view = csm_rt.cascade_views[i];
        dg_rendering_begin(ddev, shadow_rt.color_attachments, 0, &depth_tex_to_write, TRUE, TRUE);
        dg_rendering_end(ddev);
    }
    //clear swapchain?
    //dg_rendering_begin(ddev, NULL, 1, NULL, TRUE, TRUE);



    dcamera_update(&cam);
    mat4 view = dcamera_get_view_matrix(&cam);
    mat4 proj = perspective_proj(60.0f, ddev->swap.extent.width/(f32)ddev->swap.extent.height, 0.01, 100);



    //set desc set 0
    mat4 data[4] = {view, proj, m4d(1.0f),m4d(1.0f)};
    dg_set_desc_set(ddev,&ddev->def_pipe, data, sizeof(data), 0);
     
    
    draw_cube_def(ddev, mat4_mul(mat4_translate(v3(1 * fabs(sin(5 * dtime_sec(dtime_now()))),0,0)), 
        mat4_rotate(90 * dtime_sec(dtime_now()), v3(0.2,0.4,0.7))), v4(1,1,1,1), v4(1,0,1,1));
    draw_cube_def(ddev, mat4_mul(mat4_translate(v3(0,-3,0)),mat4_scale(v3(100,1,100))), v4(0.05,0.05,0.05,1), v4(0.9,0.2,0.2,1));
    draw_cube_def(ddev, mat4_translate(v3(4,0,0)), v4(1,0,0,1), v4(0,1,1,1));
    draw_cube_def(ddev, mat4_translate(v3(8,0,0)), v4(1,0,1,1), v4(1,1,0,1));
    draw_cube_def(ddev, mat4_translate(v3(16,0,0)), v4(1,1,0,1), v4(0,1,1,1));
    //draw_model_def(ddev, &water_bottle,mat4_mul(mat4_translate(v3(0,3,0)), mat4_mul(mat4_rotate(0 * dtime_sec(dtime_now()) / 8.0f, v3(0,1,0)),mat4_scale(v3(0.05,0.05,0.05)))));
    draw_model_def(ddev, &water_bottle,mat4_mul(mat4_translate(v3(0,3,0)), mat4_mul(mat4_rotate(0 * dtime_sec(dtime_now()) / 8.0f, v3(0,1,0)),mat4_scale(v3(10,10,10)))));




    mat4 lsm[DG_MAX_CASCADES];
    f32 fdist[DG_MAX_CASCADES];
    vec3 light_dir = vec3_mulf(vec3_normalize(v3(0,0.9,0.3)), 1);
    u32 cascade_count = 3;
    dg_calc_lsm(light_dir, proj, view, lsm,fdist, cascade_count);
    //draw to shadow map
    for (u32 i = 0;i < cascade_count;++i)
    {
        draw_cube_def_shadow(ddev, mat4_mul(mat4_translate(v3(1 * fabs(sin(5 * dtime_sec(dtime_now()))),0,0)), 
            mat4_rotate(90 * dtime_sec(dtime_now()), v3(0.2,0.4,0.7))), lsm[i],i);
        draw_cube_def_shadow(ddev, mat4_mul(mat4_translate(v3(0,-3,0)),mat4_scale(v3(100,1,100))), lsm[i],i);
        draw_cube_def_shadow(ddev, mat4_translate(v3(4,0,0)), lsm[i],i);
        draw_cube_def_shadow(ddev, mat4_translate(v3(8,0,0)), lsm[i],i);
        draw_cube_def_shadow(ddev, mat4_translate(v3(16,0,0)), lsm[i],i);
    }




    dg_wait_idle(ddev);
    {
        dg_rendering_begin(ddev, NULL, 1, NULL, FALSE, FALSE);
        dg_set_viewport(ddev, 0,0,ddev->swap.extent.width, ddev->swap.extent.height);
        dg_set_scissor(ddev, 0,0,ddev->swap.extent.width, ddev->swap.extent.height);

        mat4 data[4] = {0.9,(sin(0.02 * dtime_sec(dtime_now()))),0.2,0.2};
        dg_bind_pipeline(ddev, &ddev->composition_pipe);
        //FIX:  datafullscreen should be as big as the number of cascades, but im bored
        mat4 data_fullscreen[6] = {lsm[0], lsm[1],lsm[2],lsm[3], (mat4){fdist[0],0,0,0,fdist[1],0,0,0,fdist[2],0,0,0,fdist[3],0,0,0},{light_dir.x,light_dir.y,light_dir.z,0, cam.pos.x,cam.pos.y,cam.pos.z, 1.0} };
        dg_set_desc_set(ddev,&ddev->composition_pipe, &data_fullscreen, sizeof(data_fullscreen), 1);
        dgTexture textures[4];
        textures[0] = def_rt.color_attachments[0];
        textures[1] = def_rt.color_attachments[1];
        textures[2] = def_rt.color_attachments[2];
        textures[3] = csm_rt.depth_attachment;
        dg_set_desc_set(ddev,&ddev->composition_pipe, textures, 4, 2);
        dg_draw(ddev, 3,0);

        dg_rendering_end(ddev);
    }
    //draw_model(ddev, &fox,mat4_mul(mat4_translate(v3(3,0,0)), mat4_mul(mat4_mul(mat4_rotate(90,v3(0,-1,0)),mat4_rotate(90, v3(-1,0,0))),mat4_scale(v3(5,5,5)))));
    draw_model(ddev, &fox,mat4_mul(mat4_translate(v3(10,0,0)), mat4_mul(mat4_mul(mat4_rotate(0,v3(0,-1,0)),mat4_rotate(90, v3(1,0,0))),mat4_scale(v3(0.05,0.05,0.05)))));
    
    //draw the grid ???
    if (ddev->grid_active){
        dg_rendering_begin(ddev, NULL, 1, &def_rt.depth_attachment, FALSE, FALSE);
        dg_set_viewport(ddev, 0,0,ddev->swap.extent.width, ddev->swap.extent.height);
        dg_set_scissor(ddev, 0,0,ddev->swap.extent.width, ddev->swap.extent.height);
        dg_bind_pipeline(ddev, &ddev->grid_pipe);
        //@FIX: why float copy doesn't work due to alignment and we have to copy v4's ?????? (SPIR-V thing)
        vec4 object_data = (vec4){2.0f};
        dg_set_desc_set(ddev,&ddev->grid_pipe, &object_data, sizeof(object_data), 1);
        dg_draw(ddev, 6,0);
        dg_rendering_end(ddev);
    }
    //draw_model_def(ddev, &water_bottle,mat4_mul(mat4_translate(v3(3,3,0)), mat4_scale(v3(10,10,10))));
    
    dTransform * pt = dtransform_cm_world(&transform_manager, parent.id);
    draw_cube(ddev, dtransform_to_mat4(*pt));
    dTransform * ct = dtransform_cm_world(&transform_manager, child.id);
    draw_cube(ddev, dtransform_to_mat4(*ct));
    if (dkey_down(DK_1)){
        u32 component_index = dtransform_cm_lookup(&transform_manager, parent);
        dTransform nt = *pt;
        nt.trans.y +=1.0;
        dtransform_cm_set_local(&transform_manager, component_index, nt);
    }
}

void dg_frame_end(dgDevice *ddev)
{
    draw_cube(ddev, mat4_translate(v3(2,10,0)));

    dg_end_command_buffer(ddev, ddev->command_buffers[ddev->current_frame]);

    VkSubmitInfo si = {0};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {ddev->image_available_semaphores[ddev->current_frame]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = wait_semaphores;
    si.pWaitDstStageMask = wait_stages;

    si.commandBufferCount = 1;
    si.pCommandBuffers= &ddev->command_buffers[ddev->current_frame];
    VkSemaphore signal_semaphores[] = { ddev->render_finished_semaphores[ddev->current_frame] };
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = signal_semaphores;

    //vkResetFences(ddev->device, 1, &ddev->in_flight_fences[0]);
    VK_CHECK(vkQueueSubmit(ddev->graphics_queue, 1, &si, ddev->in_flight_fences[ddev->current_frame]));

    VkPresentInfoKHR present_info = { 0 };
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;


    VkSwapchainKHR swapchains[] = { ddev->swap.swapchain };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &ddev->image_index;
    present_info.pResults = NULL;


    VkResult res = vkQueuePresentKHR(ddev->present_queue, &present_info);

    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
    {
        dg_recreate_swapchain(ddev);
    }
    else if (res != VK_SUCCESS)
        dlog(NULL, "Failed to present swapchain image!\n");

    ddev->current_frame = (ddev->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

}
void dg_device_init(void)
{
	assert(dg_create_instance(&dd));
	assert(dg_surface_create(&dd,&main_window));
	assert(dg_pick_physical_device(&dd));
	assert(dg_create_logical_device(&dd));
    assert(dg_create_command_pool(&dd));
    assert(dg_create_swapchain(&dd));
    assert(dg_create_swapchain_image_views(&dd));
    dg_descriptor_set_layout_cache_init(&dd.desc_layout_cache); //the cache needs to be ready before pipeline creation
    assert(dg_create_pipeline(&dd, &dd.def_pipe,"def.vert", "def.frag", DG_PIPE_OPTION_PACK_VERTEX_ATTRIBS));
    assert(dg_create_pipeline(&dd, &dd.pbr_def_pipe,"pbr_def.vert", "pbr_def.frag", FALSE));
    assert(dg_create_pipeline(&dd, &dd.shadow_pipe,"sm.vert", "sm.frag", DG_PIPE_OPTION_PACK_VERTEX_ATTRIBS));
    assert(dg_create_pipeline(&dd, &dd.grid_pipe,"grid.vert", "grid.frag", DG_PIPE_OPTION_PACK_VERTEX_ATTRIBS | DG_PIPE_OPTION_BLEND));
    //assert(dg_create_pipeline(&dd, &dd.fullscreen_pipe,"fullscreen.vert", "fullscreen.frag", TRUE));
    assert(dg_create_pipeline(&dd, &dd.base_pipe,"base.vert", "base.frag", DG_PIPE_OPTION_PACK_VERTEX_ATTRIBS));
    assert(dg_create_pipeline(&dd, &dd.anim_pipe,"anim.vert", "anim.frag", FALSE));
    assert(dg_create_pipeline(&dd, &dd.composition_pipe,"composition.vert", "composition.frag", DG_PIPE_OPTION_PACK_VERTEX_ATTRIBS));
    assert(dg_create_pipeline(&dd, &dd.dui_pipe,DUI_VERT, DUI_FRAG, DG_PIPE_OPTION_PACK_VERTEX_ATTRIBS| DG_PIPE_OPTION_BLEND | DG_PIPE_OPTION_READY_SHADERS));
    assert(dg_create_command_buffers(&dd));
    assert(dg_create_sync_objects(&dd));

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        dg_descriptor_allocator_init(&dd, &dd.desc_alloc[i]);


    dg_ubo_data_buffer_init(&dd, &dd.ubo_buf, sizeof(mat4)*200);
    dd.shadow_pass_active = FALSE;
    dd.grid_active = FALSE;
	dlog(NULL, "Vulkan initialized correctly!\n");

    dcamera_init(&cam);
}

b32 dgfx_init(void)
{
	dg_device_init();

    dgVertex *cube_vertices = cube_build_verts();
	//create vertex buffer @check first param
	dg_create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
	(VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
	&base_vbo, sizeof(dgVertex) * 24, cube_vertices);
	
	dg_create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
	(VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
	&base_pos, sizeof(vec3) * 24, cube_positions);
	
	dg_create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
	(VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
	&base_norm, sizeof(vec3) * 24, cube_normals);

    dg_create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
	(VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
	&base_tex, sizeof(vec2) * 24, cube_tex_coords);
	
	//create index buffer
	dg_create_buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
	(VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
	&base_ibo, sizeof(cube_indices[0]) * array_count(cube_indices), cube_indices);

    dg_rt_init(&dd, &def_rt, 4, TRUE,dd.swap.extent.width, dd.swap.extent.height);
    dg_rt_init(&dd, &shadow_rt, 1, TRUE, 512,512);
    dg_rt_init_csm(&dd, &csm_rt, 3, 512,512);

    water_bottle = dmodel_load_gltf("WaterBottle");
    fox = dmodel_load_gltf("untitled");
	return 1;
}