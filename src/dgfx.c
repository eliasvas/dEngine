#include "dgfx.h"
#define VK_USE_PLATFORM_XLIB_KHR
#define VOLK_IMPLEMENTATION
#include "volk/volk.h"
#include "SDL_vulkan.h"

dgDevice dd;

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
, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME };

static const char *validation_layers[]= {
    "VK_LAYER_KHRONOS_validation"
};


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
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
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

#define DG_QUEUE_FAMILY_MAX 32

//@TODO(ilias) vkGetPHysicalDeviceQueueFamilyProperties doesn't work with static queue_families
static dgQueueFamilyIndices dg_find_queue_families(VkPhysicalDevice device)
{
	dgQueueFamilyIndices indices = {0};

	u32 queue_family_count = 1;
	VkQueueFamilyProperties queue_families[DG_QUEUE_FAMILY_MAX];
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);
	//queue_family_count = minimum(queue_family_count, DG_QUEUE_FAMILY_MAX);
	VkBool32 present_support = VK_FALSE;
	//@FIX(ilias): WHY is queue_family_count 0???????????????????? goddamn!
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
b32 dg_surface_create(dgDevice *ddev,dWindow *window)
{
	//VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR
	SDL_Vulkan_CreateSurface(window->window, ddev->instance, &ddev->surface);
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
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state = { 0 };
    extended_dynamic_state.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    extended_dynamic_state.pNext = NULL;
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
        if (details.formats[i].format == VK_FORMAT_B8G8R8A8_SRGB  &&
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

    create_info.minImageCount = image_count;
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
    printf("new swapchain image_count: %i\n", image_count);
    printf("new swapchain image_dims: %i %i\n", ddev->swap.extent.width, ddev->swap.extent.height);
	//ddev->swap.depth_attachment = dg_create_depth_attachment(ddev->swap.extent.width, ddev->swap.extent.height);
	
	//ddev->swap.rp_begin = create_render_pass(VK_ATTACHMENT_LOAD_OP_CLEAR,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_GENERAL, 1, TRUE);

    return DSUCCESS;
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
    char path[256];
    sprintf(path, "%s%s.spv", engine_config.spirv_path, filename);
	u32 code_size;
	u32 *shader_code = NULL;
	if (read_file(path, &shader_code, &code_size) == -1){return;};
	shader->module = dg_create_shader_module((char*)shader_code, code_size);
	shader->uses_push_constants = FALSE;
    //shader_reflect(shader_code, code_size, &shader->info);
	//printf("Shader: %s has %i input variable(s)!\n", filename, shader->info.input_variable_count);
	shader->stage = stage;
	free(shader_code);
}  


void dg_device_init(void)
{
	assert(dg_create_instance(&dd));
	assert(dg_surface_create(&dd,&main_window));
	assert(dg_pick_physical_device(&dd));
	assert(dg_create_logical_device(&dd));
    assert(dg_create_swapchain(&dd));
    assert(dg_create_swapchain_image_views(&dd));

	printf("Vulkan initialized correctly!\n");
}

b32 dgfx_init(void)
{
	dg_device_init();
	return 1;
}