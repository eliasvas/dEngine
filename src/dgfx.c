#include "dgfx.h"
#define VK_USE_PLATFORM_XLIB_KHR
#define VOLK_IMPLEMENTATION
#include "volk/volk.h"
#include "SDL_vulkan.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "dtime.h"

dgDevice dd;
dgBuffer base_vbo;
dgBuffer base_ibo;
dgBuffer global_ubo;
dgTexture t;
dgRT def_rt;

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
"VK_EXT_extended_dynamic_state"
, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME ,
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

u32 cube_indices[] = {
     0, 1, 2,   2, 3, 0,    // v0-v1-v2, v2-v3-v0 (front)
     4, 5, 6,   6, 7, 4,    // v0-v3-v4, v4-v5-v0 (right)
     8, 9,10,  10,11, 8,    // v0-v5-v6, v6-v1-v0 (top)
    12,13,14,  14,15,12,    // v1-v6-v7, v7-v2-v1 (left)
    16,17,18,  18,19,16,    // v7-v4-v3, v3-v2-v7 (bottom)
    20,21,22,  22,23,20     // v4-v7-v6, v6-v5-v4 (back)
};
static dgVertex *cube_build_verts(void)
{
	dgVertex *verts = (dgVertex*)malloc(sizeof(dgVertex) * 24);
	for (u32 i =0; i < 24; ++i)
	{
		verts[i].pos = cube_positions[i];
		verts[i].normal = cube_normals[i];
		verts[i].tex_coord = cube_tex_coords[i];
	}
	return verts;
}

typedef struct BasePushConstants
{
    vec4 data;
    mat4 render_matrix;
}BasePushConstants;

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
#elif defined (BUILD_WINDOWS)
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
	VkExtensionProperties *extensions = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * ext_count);
	vkEnumerateInstanceExtensionProperties(NULL, &ext_count, extensions);

	//for (u32 i = 0; i < ext_count; ++i)printf("EXT: %s\n", extensions[i].extensionName);
	ddev->instance = instance;

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
    VkExtensionProperties *available_extensions = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * ext_count);
    
    vkEnumerateDeviceExtensionProperties(device, NULL, &ext_count, available_extensions);
    /*
    for (u32 i = 0; i < ext_count; ++i)
        printf("EXT %i: %s\n", i, available_extensions[i]);
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
        details.formats = (VkSurfaceFormatKHR*)malloc(sizeof(VkSurfaceFormatKHR) * format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats);
    }
    u32 present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, NULL);
    details.present_mode_count = present_mode_count;
    
    if (present_mode_count != 0)
    {
        details.present_modes = (VkPresentModeKHR*)malloc(sizeof(VkPresentModeKHR) * present_mode_count);
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
    if (extensions_supported == 0) printf("GRAPHICS DRIVER: some device extension not supported!!\n");
    
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
        printf("Failed to find GPUs with Vulkan support!");
		return FALSE;
	}
    
    VkPhysicalDevice devices[DG_PHYSICAL_DEVICE_MAX];
    vkEnumeratePhysicalDevices(ddev->instance, &device_count, devices);
	//@FIX(ilias): this is 1 here because llvmpipe is 0 and we don't want that! no vulkan 1.3!!
    for (u32 i = 1; i < device_count; ++i)
        if (is_device_suitable(devices[i], ddev->surface))
			{
				ddev->physical_device = devices[i];

				VkPhysicalDeviceProperties p;
				vkGetPhysicalDeviceProperties(ddev->physical_device, &p);
				printf("physical device picked: %s\n", p.deviceName);
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
static dgTexture dg_create_depth_attachment(dgDevice *ddev, u32 width, u32 height);
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
    ddev->swap.images = (VkImage*)malloc(sizeof(VkImage) * image_count);
    vkGetSwapchainImagesKHR(ddev->device, ddev->swap.swapchain, &image_count, ddev->swap.images);
    ddev->swap.image_format = surface_format.format;
    ddev->swap.extent = extent;
    ddev->swap.image_count = image_count;//TODO(ilias): check
    //printf("New swapchain image_count: %i\n", image_count);
    //printf("New swapchain image_dims: %i %i\n", ddev->swap.extent.width, ddev->swap.extent.height);
	ddev->swap.depth_attachment = dg_create_depth_attachment(ddev, ddev->swap.extent.width, ddev->swap.extent.height);
	
    return DSUCCESS;
}

static void dg_cleanup_swapchain(dgDevice *ddev)
{
   //vkDestroyPipeline(ddev->device, ddev->fullscreen_pipe.pipeline, NULL); 
   //vkDestroyPipelineLayout(ddev->device, ddev->fullscreen_pipe.pipeline_layout, NULL);
   for (u32 i = 0; i < ddev->swap.image_count; ++i)
        vkDestroyImageView(ddev->device, ddev->swap.image_views[i], NULL);
    vkDestroySwapchainKHR(ddev->device, ddev->swap.swapchain, NULL);
    //clear depth attachment of swapchain
}

static VkImageView dg_create_image_view(VkImage image, VkFormat format,  VkImageAspectFlags aspect_flags)
{
	VkImageView image_view;
	
	VkImageViewCreateInfo view_info = {0};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = image;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format = format;
	view_info.subresourceRange.aspectMask = aspect_flags;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;
	VK_CHECK(vkCreateImageView(dd.device, &view_info, NULL, &image_view));
	return image_view;
}

static void dg_create_texture_sampler(dgDevice *ddev, VkSampler *sampler)
{
	VkSamplerCreateInfo sampler_info = {0};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	
	VkPhysicalDeviceProperties prop = {0};
	vkGetPhysicalDeviceProperties(ddev->physical_device, &prop);
	
	sampler_info.anisotropyEnable = VK_FALSE;
	sampler_info.maxAnisotropy = prop.limits.maxSamplerAnisotropy;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;
	VK_CHECK(vkCreateSampler(ddev->device, &sampler_info, NULL, sampler));
}

static b32 dg_create_swapchain_image_views(dgDevice *ddev)
{
    ddev->swap.image_views = (VkImageView*)malloc(sizeof(VkImageView) * ddev->swap.image_count);
    for (u32 i = 0; i < ddev->swap.image_count; ++i)
		ddev->swap.image_views[i] = dg_create_image_view(ddev->swap.images[i], ddev->swap.image_format,VK_IMAGE_ASPECT_COLOR_BIT);
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
    char path[128];
    sprintf(path, "%s%s.spv", engine_config.spirv_path, filename);
	u32 code_size;
	u32 *shader_code = NULL;
	if (read_file(path, &shader_code, &code_size) == -1){return;}
	shader->module = dg_create_shader_module((char*)shader_code, code_size);
	shader->uses_push_constants = FALSE;
    //shader_reflect(shader_code, code_size, &shader->info);
	//printf("Shader: %s has %i input variable(s)!\n", filename, shader->info.input_variable_count);
	shader->stage = stage;
    spvReflectCreateShaderModule(code_size, shader_code, &shader->info);
	free(shader_code);

}  

static VkViewport viewport_basic(dgDevice *ddev)
{
    VkViewport viewport = { 0 };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (f32)ddev->swap.extent.width;
    viewport.height = (f32)ddev->swap.extent.height;
    //viewport.height *= fabs(sin(get_time()));
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}

static VkViewport viewport(f32 ww, f32 wh)
{
    VkViewport viewport = { 0 };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = ww;
    viewport.height = wh;
    //viewport.height *= fabs(sin(get_time()));
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    return viewport;
}

static VkRect2D scissor(f32 w, f32 h)
{
    VkRect2D scissor = {0};
    scissor.offset.x = 0;
	scissor.offset.y = 0;
    scissor.extent = (VkExtent2D){w,h};
    //scissor.extent.height *= fabs(sin(get_time()));
	
    return scissor;

}
static VkRect2D scissor_basic(dgDevice *ddev)
{
    VkRect2D scissor = {0};
    scissor.offset.x = 0;
	scissor.offset.y = 0;
    scissor.extent = ddev->swap.extent;
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

static VkVertexInputBindingDescription dg_get_bind_desc_basic(void)
{
    VkVertexInputBindingDescription bind_desc = {0};
    bind_desc.binding = 0;
    bind_desc.stride = sizeof(dgVertex);
    bind_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;//per-vertex
    return bind_desc;
}

static VkVertexInputBindingDescription dg_get_bind_desc(dgShader *shader, u32 vert_size)
{
    VkVertexInputBindingDescription bind_desc = {0};
    bind_desc.binding = 0;
    bind_desc.stride = vert_size;
    bind_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;//per-vertex
    return bind_desc;
}

static u32 dg_get_attr_desc(dgShader *shader, VkVertexInputAttributeDescription *attr_desc, u32 *vert_size)
{
    u32 attr_index = 0;
    u32 global_offset = 0;
    u32 attribute_count = shader->info.input_variable_count;

    memset(attr_desc, 0, sizeof(VkVertexInputAttributeDescription) * DG_VERTEX_INPUT_ATTRIB_MAX);

    for (u32 i = 0; i < shader->info.input_variable_count; ++i)
    {
        for (u32 j = 0; j <shader->info.input_variable_count; ++j)
        {
            attr_index = (u32)shader->info.input_variables[j]->location;
            if (shader->info.input_variables[j]->built_in != -1)
            {
                --attribute_count;
                break;
            }
            if (attr_index == i )//Note(inv): we want to write the inputs in order to get the global offset
            {
                SpvReflectInterfaceVariable *input_var = shader->info.input_variables[attr_index];
                memset(&attr_desc[attr_index], 0, sizeof(VkVertexInputAttributeDescription));
                attr_desc[attr_index].binding = 0; //TODO(inv): check dis shit
                attr_desc[attr_index].format = input_var->format;
                attr_desc[attr_index].location= input_var->location;
                attr_desc[attr_index].offset = global_offset;
                global_offset += input_var->numeric.vector.component_count * sizeof(f32);


                break;
            }

        }

    }
    *vert_size = global_offset;
    return attribute_count;
}

static VkPipelineVertexInputStateCreateInfo 
dg_pipe_vertex_input_state_create_info(dgShader *shader, VkVertexInputBindingDescription *bind_desc, VkVertexInputAttributeDescription *attr_desc)
{
    u32 vert_size;
    u32 attribute_count = dg_get_attr_desc(shader, attr_desc, &vert_size);
    *bind_desc = dg_get_bind_desc(shader, vert_size);
 
	VkPipelineVertexInputStateCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	info.pNext = NULL;
	info.vertexBindingDescriptionCount = (attribute_count >0) ? 1 : 0;
    info.pVertexBindingDescriptions = bind_desc;
	info.vertexAttributeDescriptionCount = attribute_count;
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

static VkPipelineColorBlendAttachmentState dg_pipe_color_blend_attachment_state(void)
{
	VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	color_blend_attachment.blendEnable = VK_FALSE;	

    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
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
//@FIXME(inv):this breaks for more than one descriptor sets!
static VkDescriptorSetLayout dg_pipe_descriptor_set_layout(dgDevice *ddev, dgShader*shader)
{
    //dg_descriptor_set_layout_cache_get(dgDescriptorSetLayoutCache *cache, u64 key)

    u64 total_hash = 0;
    VkDescriptorSetLayout desc_set_layout;
    if (shader->info.descriptor_set_count)
    {
        VkDescriptorSetLayoutBinding *desc_set_layout_bindings = NULL;
        for(u32 i=0;i< shader->info.descriptor_set_count; ++i)
        {
            SpvReflectDescriptorSet current_set = shader->info.descriptor_sets[i];
            for  (u32 j=0;j < current_set.binding_count; ++j)
            {
               VkDescriptorSetLayoutBinding binding ={0};
               binding.binding = current_set.bindings[j]->binding; 
               //binding.descriptorCount = current_set.bindings[j]->count;
               binding.descriptorCount = 1; //no arrays on my watch ;)
               binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
               binding.descriptorType = current_set.bindings[j]->descriptor_type;

               dbf_push(desc_set_layout_bindings, binding);

               //u64 full_binding_hash = current_binding.binding | current_binding.descriptorType << 8 | current_binding.descriptorCount << 16 | current_binding.stageFlags << 24;
               u16 binding_hash = binding.binding | binding.descriptorType << 8; //18 bytes per binding * 4 (MAX) bindings = 64 bits! (one u64, our hash key)
               total_hash |= (binding_hash << (binding.binding * 16));
               //printf("total hash: %lu\n", total_hash);
            }
        }
        desc_set_layout = dg_descriptor_set_layout_cache_get(&ddev->desc_layout_cache, total_hash);
        if(desc_set_layout == VK_NULL_HANDLE)
        {
            //if layout not found we have to create it and add to cache
            VkDescriptorSetLayoutCreateInfo desc_layout_ci = {0};
            desc_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            //desc_layout_ci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
            desc_layout_ci.bindingCount = dbf_len(desc_set_layout_bindings);
            desc_layout_ci.pBindings = desc_set_layout_bindings;

            VK_CHECK(vkCreateDescriptorSetLayout(ddev->device, &desc_layout_ci, NULL, &desc_set_layout));
            dg_descriptor_set_layout_cache_add(&ddev->desc_layout_cache, 
                (dgDescriptorSetLayoutInfo){total_hash, desc_set_layout_bindings}, desc_set_layout);
            printf("Created (another) DSL!! :( \n");
        }
        else
        {
            //we don't need the binding array, so we delete it
            dbf_free(desc_set_layout_bindings);
        }
    }
    return desc_set_layout;
}

//this is pretty much empty
static VkPipelineLayoutCreateInfo dg_pipe_layout_create_info(VkDescriptorSetLayout *layouts, u32 layouts_count, VkPushConstantRange *pc, dgShader *shader)
{
    //maybe for each pipeline layout / descriptor set we should string->int hash the members for fast updates
    
	VkPipelineLayoutCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	info.pNext = NULL;
	
	info.flags = 0;
	info.setLayoutCount = layouts_count;
	info.pSetLayouts = layouts;
    


	info.pushConstantRangeCount = 0;
	info.pPushConstantRanges = NULL;
    if (shader->info.push_constant_block_count)
    {
        pc->offset = shader->info.push_constant_blocks[0].absolute_offset;
        pc->size = shader->info.push_constant_blocks[0].padded_size;
        pc->stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        info.pushConstantRangeCount = 1;
        info.pPushConstantRanges = pc;
    }

	return info;
}

static b32 dg_create_pipeline(dgDevice *ddev, dgPipeline *pipe, char *vert_name, char *frag_name)
{

    VkRect2D s = scissor_basic(ddev);
    VkViewport v = viewport_basic(ddev);


    //read shaders and register them in the pipeline builder
	dg_shader_create(ddev->device, &pipe->vert_shader, vert_name, VK_SHADER_STAGE_VERTEX_BIT); 
	dg_shader_create(ddev->device, &pipe->frag_shader, frag_name, VK_SHADER_STAGE_FRAGMENT_BIT);
	u32 shader_stages_count = 2;
    VkPipelineShaderStageCreateInfo shader_stages[2];
	shader_stages[0] = dg_pipe_shader_stage_create_info(pipe->vert_shader.stage, pipe->vert_shader.module);
	shader_stages[1] = dg_pipe_shader_stage_create_info(pipe->frag_shader.stage, pipe->frag_shader.module);

 
    VkVertexInputBindingDescription bind_desc[4];
    VkVertexInputAttributeDescription attr_desc[DG_VERTEX_INPUT_ATTRIB_MAX];
    VkPipelineVertexInputStateCreateInfo vert_input_state = 
    dg_pipe_vertex_input_state_create_info(&pipe->vert_shader, bind_desc, attr_desc);



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
        blend_attachment_states[i] = dg_pipe_color_blend_attachment_state();
    }

    VkPipelineColorBlendStateCreateInfo color_blend_state =  
    dg_pipe_color_blend_state_create_info(&blend_attachment_states[0],pipe->frag_shader.info.output_variable_count);

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

    
    VkDescriptorSetLayout desc_set_layout = dg_pipe_descriptor_set_layout(ddev, &pipe->vert_shader);

    VkPushConstantRange pc;
    VkPipelineLayoutCreateInfo pipe_layout_info = 
    dg_pipe_layout_create_info(&desc_set_layout, pipe->vert_shader.info.descriptor_binding_count, &pc, &pipe->vert_shader);

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
        color_formats[i] = ddev->swap.image_format;
    }
 
    // New create info to define color, depth and stencil attachments at pipeline create time
    VkPipelineRenderingCreateInfoKHR pipe_renderingCI = {0};
    pipe_renderingCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    pipe_renderingCI.colorAttachmentCount = pipe->frag_shader.info.output_variable_count;
    pipe_renderingCI.pColorAttachmentFormats = color_formats;
    //TODO(ilias): set these for when we want depth 
    pipe_renderingCI.depthAttachmentFormat = ddev->swap.depth_attachment.format;
    pipe_renderingCI.stencilAttachmentFormat = ddev->swap.depth_attachment.format;
    // Chain into the pipeline creat einfo
    pipeCI.pNext = &pipe_renderingCI;


    VK_CHECK(vkCreateGraphicsPipelines(ddev->device, VK_NULL_HANDLE, 1, &pipeCI, NULL, &pipe->pipeline));



    return DSUCCESS;
}

static b32 dg_create_sync_objects(dgDevice *ddev)
{
    ddev->image_available_semaphores = (VkSemaphore*)malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    ddev->render_finished_semaphores = (VkSemaphore*)malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
    ddev->in_flight_fences = (VkFence*)malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);
    ddev->images_in_flight = (VkFence*)malloc(sizeof(VkFence) * ddev->swap.image_count);
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
    ddev->command_buffers = (VkCommandBuffer*)malloc(sizeof(VkCommandBuffer) * ddev->swap.image_count);
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = ddev->command_pool; //where to allocate the buffer from
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = ddev->swap.image_count;
    VK_CHECK(vkAllocateCommandBuffers(ddev->device, &alloc_info, ddev->command_buffers));
    return DSUCCESS;
}

static void dg_recreate_swapchain(dgDevice *ddev)
{
    vkDeviceWaitIdle(ddev->device);
    //in case of window minimization (w = 0, h = 0) we wait until we get a proper window again
    u32 width = main_window.width;
    u32 height = main_window.height;
    
    
    dg_cleanup_swapchain(ddev);
    dg_create_swapchain(ddev);
    dg_create_swapchain_image_views(ddev);
    
    //dg_create_pipeline(ddev, &ddev->fullscreen_pipe, "fullscreen.vert", "fullscreen.frag");
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
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
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

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &command_buffer;

    VkFenceCreateInfo fenceCI= {};
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
    free(da->pool_sizes);
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


static void dg_rendering_begin(dgDevice *ddev, dgTexture *tex, u32 attachment_count, dgTexture *depth_tex, b32 clear)
{
    VkRenderingAttachmentInfoKHR color_attachments[DG_MAX_COLOR_ATTACHMENTS];
    VkAttachmentLoadOp load_op = (clear > 0) ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    memset(color_attachments, 0, sizeof(VkRenderingAttachmentInfoKHR) * DG_MAX_COLOR_ATTACHMENTS);

    if (tex == NULL)
    {
        color_attachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        color_attachments[0].imageView = dd.swap.image_views[dd.image_index];
        color_attachments[0].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
        color_attachments[0].loadOp = load_op;
        color_attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachments[0].clearValue.color = (VkClearColorValue){0.f,0.f,0.f,0.f};
    }
    else
    {
        for (u32 i = 0; i < attachment_count; ++i)
        {
            color_attachments[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
            color_attachments[i].imageView = tex[i].view;
            color_attachments[i].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
            color_attachments[i].loadOp = load_op;
            color_attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            color_attachments[i].clearValue.color = (VkClearColorValue){0.f,0.f,0.f,0.f};
        }
    }

    VkRenderingAttachmentInfoKHR depth_attachment = {0};
    depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR; 
    depth_attachment.pNext = NULL; 
    if (depth_tex == NULL)
        depth_attachment.imageView = ddev->swap.depth_attachment.view;
    else
        depth_attachment.imageView = depth_tex->view;
    depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
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
    rendering_info.pStencilAttachment = &depth_attachment;

    vkCmdBeginRenderingKHR(ddev->command_buffers[ddev->current_frame], &rendering_info);
}

static void dg_rendering_end(dgDevice *ddev)
{
    vkCmdEndRenderingKHR(ddev->command_buffers[ddev->current_frame]);
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


    dg_descriptor_allocator_reset_pools(&ddev->desc_alloc[ddev->current_frame]);
    dg_prepare_command_buffer(ddev, ddev->command_buffers[ddev->current_frame]);
    ///*
    dg_rendering_begin(ddev, NULL, 1, NULL, TRUE);
    //dg_rendering_begin(ddev, def_rt.color_attachments, 2, &def_rt.depth_attachment, TRUE);

    /*
    VkViewport view = viewport(def_rt.color_attachments[0].width, def_rt.color_attachments[0].height);
    vkCmdSetViewport(dd.command_buffers[ddev->current_frame], 0, 1, &view);

    VkRect2D sci = scissor(def_rt.color_attachments[0].width, def_rt.color_attachments[0].height);
    vkCmdSetScissor(dd.command_buffers[ddev->current_frame], 0, 1, &sci);
    */

///*
    VkViewport view = viewport(dd.swap.extent.width,dd.swap.extent.height);
    vkCmdSetViewport(dd.command_buffers[ddev->current_frame], 0, 1, &view);

    VkRect2D sci = scissor(dd.swap.extent.width, dd.swap.extent.height);
    vkCmdSetScissor(dd.command_buffers[ddev->current_frame], 0, 1, &sci);
    //*/
    vkCmdBindPipeline(dd.command_buffers[ddev->current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, dd.fullscreen_pipe.pipeline);
    vkCmdDraw(dd.command_buffers[ddev->current_frame], 3, 1, 0, 0);


    //drawcall 2
    vkCmdBindPipeline(dd.command_buffers[ddev->current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, dd.base_pipe.pipeline);
    VkBuffer vertex_buffers[] = {base_vbo.buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(dd.command_buffers[ddev->current_frame], 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(dd.command_buffers[ddev->current_frame], base_ibo.buffer, 0, VK_INDEX_TYPE_UINT32);

    BasePushConstants pc;
    pc.data = v4(1 * fabs(sin(dtime_sec(dtime_now()))),0.4,0.3,1);
    vkCmdPushConstants(dd.command_buffers[ddev->current_frame], dd.base_pipe.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BasePushConstants), &pc);

    VkDescriptorBufferInfo gubo_info = {0};
    gubo_info.buffer = global_ubo.buffer;
    gubo_info.offset = 0;
    gubo_info.range = global_ubo.size;

    VkDescriptorSet desc_set = VK_NULL_HANDLE;
    VkDescriptorSetLayout desc_set_layout = dg_pipe_descriptor_set_layout(ddev, &ddev->base_pipe.vert_shader);
    dg_descriptor_allocator_allocate(&ddev->desc_alloc[ddev->current_frame], &desc_set, desc_set_layout);

    VkWriteDescriptorSet set_write = {0};
    set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set_write.dstBinding = 0;
    set_write.dstSet = desc_set;
    set_write.descriptorCount = 1;
    set_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    set_write.pBufferInfo = &gubo_info;
    vkUpdateDescriptorSets(dd.device, 1, &set_write, 0, NULL);

    vkCmdBindDescriptorSets(ddev->command_buffers[ddev->current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, ddev->base_pipe.pipeline_layout, 0, 1, &desc_set, 0, NULL); 

    vkCmdDrawIndexed(dd.command_buffers[ddev->current_frame], base_ibo.size / sizeof(u32), 1, 0, 0, 0);
 
    //drawcall 3
    pc.data = v4(1 - fabs(sin(5 *dtime_sec(dtime_now()))),0.2,0.2,1);
    vkCmdPushConstants(dd.command_buffers[ddev->current_frame], dd.base_pipe.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BasePushConstants), &pc);
    vkCmdDrawIndexed(dd.command_buffers[ddev->current_frame], base_ibo.size / sizeof(u32), 1, 0, 0, 0);

    dg_rendering_end(ddev);

    dg_end_command_buffer(ddev, ddev->command_buffers[ddev->current_frame]);


}

void dg_frame_end(dgDevice *ddev)
{
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
        printf("Failed to present swapchain image!\n");

    ddev->current_frame = (ddev->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

}

//attaches ALLOCATED memory block to buffer!
static void dg_buf_bind(dgBuffer *buf, VkDeviceSize offset)
{
	vkBindBufferMemory(dd.device, buf->buffer, buf->mem, offset);
}

static void dg_buf_copy_to(dgBuffer *src,void *data, VkDeviceSize size)
{
	assert(src->mapped);
	memcpy(src->mapped, data, size);
}

static void dg_buf_setup_descriptor(dgBuffer *buf, VkDeviceSize size, VkDeviceSize offset)
{
	buf->desc.offset = offset;
	buf->desc.buffer = buf->buffer;
	buf->desc.range = size;
}
static VkResult dg_buf_map(dgBuffer *buf, VkDeviceSize size, VkDeviceSize offset)
{
    //printf("buf->mem = %i\n\n", buf->mem);
    //fflush(stdout);
	return vkMapMemory(dd.device, buf->mem, offset, size, 0, &buf->mapped);//@check @check @check @check
}

static void dg_buf_unmap(dgBuffer *buf)
{
	if (buf->mapped)
	{
		vkUnmapMemory(dd.device, buf->mem);
		buf->mapped = NULL;
	}
}

static void dg_buf_destroy(dgBuffer *buf)
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
    printf("Failed to find suitable memory type!");
}

static void dg_create_buffer(VkBufferUsageFlagBits usage, VkMemoryPropertyFlagBits mem_flags, dgBuffer*buf, VkDeviceSize size, void *data)
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
}


static u32 dg_has_stencil_component(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
static VkFormat dg_find_depth_format(dgDevice *ddev)
{	VkFormat c[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
	return dg_find_supported_format(
                                ddev,
                                c,
                                VK_IMAGE_TILING_OPTIMAL,
                                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,3);
    
}

static void dg_create_image(dgDevice *ddev, u32 width, u32 height, VkFormat format, VkImageTiling tiling, 
VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *image_memory)
{
	VkImageCreateInfo image_info = {0};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
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


static dgTexture dg_create_depth_attachment(dgDevice *ddev, u32 width, u32 height)
{
	dgTexture depth_attachment = {0};
	depth_attachment.format = dg_find_depth_format(ddev);
	
	dg_create_image(ddev,width, height, 
		depth_attachment.format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depth_attachment.image, &depth_attachment.mem);

	depth_attachment.view = dg_create_image_view(depth_attachment.image, depth_attachment.format, VK_IMAGE_ASPECT_DEPTH_BIT);
    depth_attachment.width = width;
    depth_attachment.height = height;
    dg_create_texture_sampler(ddev, &depth_attachment.sampler);
    
	return depth_attachment;
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

static dgTexture dg_create_texture_basic(dgDevice *ddev, VkFormat format)
{
	dgTexture tex;
	s32 tex_w = 256; 
    s32 tex_h = 256; 
    s32 tex_c = 4;
	VkDeviceSize image_size = tex_w * tex_h * 4;
    typedef struct iv4
    {
        u8 pixel[4];
    }iv4;

    iv4 *pixels = malloc(sizeof(iv4) * tex_w * tex_h*40);
    /*
    for (u32 i = 0; i < tex_w; ++i)
    {
        for (u32 j = 0; j < tex_h; ++j)
        {
            if (j % 2 == 0)
                pixels[i * tex_w + j] = (vec4){1,1,1,1};
            else
                pixels[i * tex_w + j] = (vec4){1,0.1,0.2,1.0};
        }
    }
    */
    
	dgBuffer idb;
	dg_create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
	(VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), &idb, image_size, pixels);
	dg_create_image(ddev, tex_w, tex_h, format, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_DST_BIT 
		| VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &tex.image, &tex.mem);
	

    VkCommandBuffer cmd = dg_begin_single_time_commands(ddev);

    VkImageMemoryBarrier imageMemoryBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageMemoryBarrier.image = tex.image;
    imageMemoryBarrier.subresourceRange =(VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, NULL,
        0, NULL,
        1, &imageMemoryBarrier);


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

	dg_buf_destroy(&idb);
	
	
	dg_create_texture_sampler(ddev, &tex.sampler);
	
	tex.view = dg_create_image_view(tex.image, format, VK_IMAGE_ASPECT_COLOR_BIT);
	tex.mip_levels = 0;
	tex.width = tex_w;
	tex.height = tex_h;
	
	return tex;
}

static dgTexture dg_create_texture_image(dgDevice *ddev, char *filename, VkFormat format)
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
	dg_create_image(ddev, tex_w, tex_h, format, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_DST_BIT 
		| VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &tex.image, &tex.mem);
	

    VkCommandBuffer cmd = dg_begin_single_time_commands(ddev);
    /*
    //first transition to layout general
    dg_image_memory_barrier(
        cmd,
        ddev->swap.images[ddev->image_index], 
        0, 
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
        (VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    );
    */
    VkImageMemoryBarrier imageMemoryBarrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageMemoryBarrier.image = tex.image;
    imageMemoryBarrier.subresourceRange =(VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, NULL,
        0, NULL,
        1, &imageMemoryBarrier);


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

	dg_buf_destroy(&idb);
	
	
	dg_create_texture_sampler(ddev, &tex.sampler);
	
	tex.view = dg_create_image_view(tex.image, format, VK_IMAGE_ASPECT_COLOR_BIT);
	tex.mip_levels = 0;
	tex.width = tex_w;
	tex.height = tex_h;
	
	return tex;
}

static void dg_rt_init(dgDevice *ddev, dgRT* rt, u32 color_count, b32 depth)
{
    rt->color_attachment_count = color_count;
    rt->depth_active = (depth > 0) ? 1 : 0;
    for (u32 i = 0; i < rt->color_attachment_count; ++i)
    {
        rt->color_attachments[i] = dg_create_texture_image(ddev,"../assets/sample.png",ddev->swap.image_format);
    }
    if (rt->depth_active)
        rt->depth_attachment = dg_create_depth_attachment(ddev, DG_DEPTH_SIZE, DG_DEPTH_SIZE);
}


void dg_device_init(void)
{
	assert(dg_create_instance(&dd));
	assert(dg_surface_create(&dd,&main_window));
	assert(dg_pick_physical_device(&dd));
	assert(dg_create_logical_device(&dd));
    assert(dg_create_swapchain(&dd));
    assert(dg_create_swapchain_image_views(&dd));
    dg_descriptor_set_layout_cache_init(&dd.desc_layout_cache); //the cache needs to be ready before pipeline creation
    assert(dg_create_pipeline(&dd, &dd.fullscreen_pipe,"fullscreen.vert", "fullscreen.frag"));
    assert(dg_create_pipeline(&dd, &dd.base_pipe,"base.vert", "base.frag"));
    assert(dg_create_command_pool(&dd));
    assert(dg_create_command_buffers(&dd));
    assert(dg_create_sync_objects(&dd));

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        dg_descriptor_allocator_init(&dd, &dd.desc_alloc[i]);

	printf("Vulkan initialized correctly!\n");
}

b32 dgfx_init(void)
{
	dg_device_init();

    dgVertex *cube_vertices = cube_build_verts();
	//create vertex buffer @check first param
	dg_create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
	(VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
	&base_vbo, sizeof(dgVertex) * 24, cube_vertices);
	
	
	
	//create index buffer
	dg_create_buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
	(VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
	&base_ibo, sizeof(cube_indices[0]) * array_count(cube_indices), cube_indices);

    mat4 data[3] = {0.3};
	//create global UBO 
	dg_create_buffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
	(VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
	&global_ubo, sizeof(mat4) * 3, data);



    //dg_rt_init(&dd, &def_rt, 4, TRUE);

    //t = dg_create_texture_image(&dd, "assets/sample.png", VK_FORMAT_R8G8B8A8_SRGB);
	return 1;
}