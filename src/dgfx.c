#include "dgfx.h"
#define VK_USE_PLATFORM_XLIB_KHR
#define VOLK_IMPLEMENTATION
#include "volk/volk.h"
#include "SDL_vulkan.h"

dgDevice dd;

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

typedef struct dQueueFamilyIndices
{
    u32 graphics_family;
    //because we cant ifer whether the vfalue was initialized correctly or is garbage
    u32 graphics_family_found;
    
    u32 present_family;
    u32 present_family_found;
}dQueueFamilyIndices;

#define DG_QUEUE_FAMILY_MAX 32

//@TODO(ilias) vkGetPHysicalDeviceQueueFamilyProperties doesn't work with static queue_families
static dQueueFamilyIndices dg_find_queue_families(VkPhysicalDevice device)
{
	dQueueFamilyIndices indices = {0};

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

static u32 is_device_suitable(VkPhysicalDevice device)
{
	dQueueFamilyIndices indices = dg_find_queue_families(device);
    
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceProperties(device, &device_properties);
    vkGetPhysicalDeviceFeatures(device, &device_features);
    
    u32 extensions_supported = dg_check_device_extension_support(device);
    if (extensions_supported == 0) printf("GRAPHICS DRIVER: some device extension not supported!!\n");
    
    //dSwapChainSupportDetails swapchain_support = query_swapchain_support(device);
    
    //here we can add more requirements for physical device selection
    return indices.graphics_family_found && extensions_supported && indices.present_family_found;
    //(swapchain_support.format_count > 0) && (swapchain_support.present_mode_count > 0);	
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
        if (is_device_suitable(devices[i]))
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
    dQueueFamilyIndices indices = dg_find_queue_families(ddev->physical_device);
    
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

//NOTE(ilias): This is UGLY AF!!!!
extern dWindow main_window;

void dg_device_init(void)
{
	assert(dg_create_instance(&dd));
	assert(dg_surface_create(&dd,&main_window));
	assert(dg_pick_physical_device(&dd));
	assert(dg_create_logical_device(&dd));
	printf("Vulkan initialized correctly!\n");
}

b32 dgfx_init(void)
{
	dg_device_init();
	return 1;
}