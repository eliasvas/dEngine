#include "dgfx.h"
#define VK_USE_PLATFORM_XLIB_KHR
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "dtime.h"
#include "glfw/include/GLFW/glfw3.h"
#include "shaders.inl"
#include "spirv_reflect/spirv_reflect.h"
#include "dlog.h"
#include "dcamera.h"
#include "dmem.h"
#include "deditor.h"
#include "dmodel.h"
#include "dentity.h" 
#include "dparticle.h"
#include "dprofiler.h"
#include "glslang/glslang/Include/glslang_c_interface.h"
#include "glslang/glslang/Include/glslang_c_shader_types.h"
#include "glslang/glslang/Include/ResourceLimits.h"
#include "glslang/SPIRV/GlslangToSpv.h"

extern void draw_model(dgDevice *ddev, dModel *m, mat4 model);
extern void draw_model_def(dgDevice *ddev, dModel *m, mat4 model);
dModel water_bottle;
dModel fox;

dgDevice dd;
dgBuffer base_vbo;

dgBuffer base_pos;
dgBuffer base_norm;
dgBuffer base_tex;
dgBuffer base_ibo;

dgBuffer sphere_vbo;
dgBuffer sphere_ibo;

dgTexture noise_tex;
dgTexture ssao_tex;
dgTexture hdr_map;
dgTexture cube_tex;
dgTexture irradiance_map;
dgTexture prefilter_map;
dgTexture brdfLUT;
dgRT def_rt; //the G-buffer
dgRT csm_rt; //the Cascaded Shadow map
dgRT composition_rt; //here the def_rt and csm_rt values are combined and the final 3d image is rendered
extern dCamera cam;
extern dEditor main_editor;

//TOODO move these to the editor OR the deviec
mat4 lsm[DG_MAX_CASCADES];
f32 fdist[DG_MAX_CASCADES];
vec3 light_dir;
mat4 proj;
mat4 view;

//NOTE(ilias): This is UGLY AF!!!!
extern dWindow main_window;
extern dAnimator animator;

#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			printf("[LINE: %i] Detected Vulkan error: %i \n",__LINE__, err);            \
		}                                                           \
	} while (0);



static const char *validation_layers[]= {
    "VK_LAYER_KHRONOS_validation"
};

vec3 cube_positions[]  = {
	{0.5f, 0.5f, 0.5f},  {-0.5f, 0.5f, 0.5f},  {-0.5f ,-0.5f, 0.5f}, {0.5f,-0.5f, 0.5f},   // v0,v1,v2,v3 (front)
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
static dgVertex *cube_build_verts(void){
	dgVertex *verts = (dgVertex*)dalloc(sizeof(dgVertex) * 24);
	for (u32 i =0; i < 24; ++i)
	{
		verts[i].pos = cube_positions[i];
		verts[i].normal = cube_normals[i];
		verts[i].tex_coord = cube_tex_coords[i];
	}
	return verts;
}

//for more info: https://www.songho.ca/opengl/gl_sphere.html
static dgVertex *sphere_build_verts(f32 radius, u32 sector_count, u32 stack_count){
	dgVertex *verts = (dgVertex*)dalloc(sizeof(dgVertex) * (sector_count+1) * (stack_count+1));
    u32 vc = 0;

    f32 x, y, z, xy;                              // vertex position
    f32 nx, ny, nz, length_inv = 1.0f / radius;   // vertex normal
    f32 s, t;                                     // vertex normal



    f32 sector_step = 2 * PI / sector_count;
    f32 stack_step = PI / stack_count;
    f32 sector_angle, stack_angle;

	for (u32 i =0; i <= stack_count; ++i)
	{
        stack_angle = PI / 2 - i * stack_step;
        xy = radius * cos(stack_angle);
        z = radius * sin(stack_angle);
        for (u32 j =0; j <= sector_count; ++j){
            sector_angle = j * sector_step;

            x = xy * cos(sector_angle);
            y = xy * sin(sector_angle);
            verts[vc].pos = v3(x,y,z);

            nx = x * length_inv;
            ny = y * length_inv;
            nz = z * length_inv;
            verts[vc].normal = v3(nx,ny,nz);

            s = (f32)j / sector_count;
            t = (f32)i / stack_count;
            verts[vc].tex_coord = v2(s,t);
        
            ++vc;    
        }
	}
    //printf("sphere of %i stacks and %i sectors has: %i vertices\n\n\n\n", stack_count, sector_count, vc);
	return verts;
}

static u16 *sphere_build_index(u32 sector_count, u32 stack_count){

    u16 *indices = (u16*)dalloc(sizeof(u16) * sector_count * stack_count * 8);
    
    u32 idx_count = 0;
    u16 k1, k2;

    for (u32 i = 0; i < stack_count; ++i){
        k1 = i * (sector_count + 1);     // beginning of current stack
        k2 = k1 + sector_count + 1;      // beginning of next stack


        for (int j = 0; j < sector_count; ++j, ++k1, ++k2){
            if (i != 0){
                indices[idx_count++] = k1;
                indices[idx_count++] = k2;
                indices[idx_count++] = k1+1;
            }

            if (i != (stack_count -1)){
                indices[idx_count++] = k1+1;
                indices[idx_count++] = k2;
                indices[idx_count++] = k2+1;
            }
        }

    }
    //printf("sphere of %i stacks and %i sectors has %i indices", sector_count, stack_count, idx_count);
    return indices;
}



//------------------------------DVK, C-Interface Vulkan Layer used by dgfx------------------------------

//DOC: creates a valid VkInstance 
static VkInstance dvk_instance_create(void) {
	VkInstance instance;

	VkApplicationInfo appinfo = {};
	appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appinfo.pApplicationName = "Mk0";
	appinfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
	appinfo.pEngineName = "dEngine";
	appinfo.engineVersion = VK_MAKE_VERSION(1,0,0);
	appinfo.apiVersion = VK_API_VERSION_1_3;
    
    
	VkInstanceCreateInfo create_info = {};
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
    
	
    u32 count;
    glfwGetRequiredInstanceExtensions(&count);
    //create_info.enabledExtensionCount = array_count(base_extensions); 
    //create_info.ppEnabledExtensionNames = (const char**)base_extensions;
    create_info.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&count);
	create_info.enabledExtensionCount = count;
    create_info.enabledLayerCount = 0;


    VkValidationFeatureEnableEXT enabled[] = { VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT };
    /*
    VkValidationFeatureDisableEXT disabled[] = {
    VK_VALIDATION_FEATURE_DISABLE_THREAD_SAFETY_EXT, VK_VALIDATION_FEATURE_DISABLE_API_PARAMETERS_EXT,
        VK_VALIDATION_FEATURE_DISABLE_OBJECT_LIFETIMES_EXT, VK_VALIDATION_FEATURE_DISABLE_CORE_CHECKS_EXT 
    };
    */
    VkValidationFeaturesEXT vf = {};
    vf.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    vf.disabledValidationFeatureCount = 0;
    vf.enabledValidationFeatureCount = 1;
    vf.pEnabledValidationFeatures = enabled;
    vf.pDisabledValidationFeatures = NULL;
    vf.pNext = NULL;

    create_info.pNext = &vf;

    
	VK_CHECK(volkInitialize());

	VK_CHECK(vkCreateInstance(&create_info, NULL, &instance));
    volkLoadInstance(instance);

#if 0
	//(OPTIONAL): extension support TODO memleak fix
	u32 ext_count = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &ext_count, NULL);
	VkExtensionProperties *extensions = (VkExtensionProperties*)dalloc(sizeof(VkExtensionProperties) * ext_count);
	vkEnumerateInstanceExtensionProperties(NULL, &ext_count, extensions);
	for (u32 i = 0; i < ext_count; ++i)printf("EXT: %s\n", extensions[i].extensionName);
    if (extensions)dfree(extensions);
#endif

    assert(instance);
    return instance;
}

//DOC: creates a valid VKSurfaceKHR
static VkSurfaceKHR dvk_surface_create(VkInstance instance, dWindow *window)
{
    VkSurfaceKHR surface;
	//VkResult res = SDL_Vulkan_CreateSurface(window->window, ddev->instance, &ddev->surface);
    VkResult res = glfwCreateWindowSurface(instance, window->gwindow, NULL, &surface);
	assert (surface != VK_NULL_HANDLE);
    return surface;
}


static const char* device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
    VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    VK_KHR_MULTIVIEW_EXTENSION_NAME,
    VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME,
};
//DOC: checks if the physical device supports all device extensions we need
static b32 dvk_check_device_extension_support(VkPhysicalDevice device)
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
            printf("Extension not found: %s\n", device_extensions[i]);
			return FALSE;
		};
    }
	dfree(available_extensions);
    return TRUE;
}

#define DVK_FAMILY_NOT_FOUND 0xFFFFFFFF
//DOC: finds suitable graphics and present family indices
static void dvk_queue_families_find(VkPhysicalDevice pd, VkSurfaceKHR surface, u32 *graphics_family, u32 *present_family)
{
    *graphics_family = DVK_FAMILY_NOT_FOUND;
    *present_family = DVK_FAMILY_NOT_FOUND;

	u32 queue_family_count;
	VkQueueFamilyProperties queue_families[32];
	vkGetPhysicalDeviceQueueFamilyProperties(pd, &queue_family_count, NULL);
	queue_family_count = (queue_family_count > 32) ? 32 : queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &queue_family_count, queue_families);

	
    VkBool32 present_support = VK_FALSE; 
	for (u32 i = 0; i < queue_family_count; ++i) //TODO stop on the first families you see
	{
		vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, surface, &present_support);
		if (present_support)
            *present_family = i;

		if (queue_families[i].queueFlags  & VK_QUEUE_GRAPHICS_BIT)
			*graphics_family = i;
	}

    //assert(*present_family != DVK_FAMILY_NOT_FOUND&& *graphics_family != DVK_FAMILY_NOT_FOUND);
}


typedef struct dvkSwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    u32 format_count;
    VkPresentModeKHR *present_modes;
    u32 present_mode_count;
}dvkSwapChainSupportDetails;
//DOC: finds details about the swap so we can e.g have that info to pick a suitable physical device
static dvkSwapChainSupportDetails dvk_swapchain_support(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    dvkSwapChainSupportDetails details;
    
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.format_count, NULL); 
    if (details.format_count)
    {
        details.formats = (VkSurfaceFormatKHR*)dalloc(sizeof(VkSurfaceFormatKHR) * details.format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.format_count, details.formats);
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.present_mode_count, NULL);
    if (details.present_mode_count)
    {
        details.present_modes = (VkPresentModeKHR*)dalloc(sizeof(VkPresentModeKHR) * details.present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.present_mode_count, details.present_modes);
    }

    return details;
}
static void dvk_swapchain_support_free(dvkSwapChainSupportDetails *s)
{
    if (s->present_modes)dfree(s->present_modes);
    if (s->formats)dfree(s->formats);
}

//DOC: checks if the physical device is gud
static b32 dvk_is_physical_device_suitable(VkPhysicalDevice pd, VkSurfaceKHR surface)
{
    VkPhysicalDeviceProperties p;
	vkGetPhysicalDeviceProperties(pd, &p);
	//dlog(NULL, "VULKAN: checking physical device: %s\n", p.deviceName);

    u32 graphics_family, present_family;
	dvk_queue_families_find(pd, surface, &graphics_family, &present_family);
    
    VkPhysicalDeviceProperties device_properties;
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceProperties(pd, &device_properties);
    vkGetPhysicalDeviceFeatures(pd, &device_features);
    
    b32 extensions_supported = dvk_check_device_extension_support(pd);
    if (!extensions_supported) 
        dlog(NULL, "DVK: some device extension not supported!!\n");
    
    dvkSwapChainSupportDetails swapchain_support = dvk_swapchain_support(pd, surface);
    
    //here we can add more requirements for physical device selection
    return (graphics_family!=DVK_FAMILY_NOT_FOUND) &&  (present_family!=DVK_FAMILY_NOT_FOUND) && extensions_supported
    &&(swapchain_support.format_count > 0) && (swapchain_support.present_mode_count > 0);	
}
//DOC: picks a suitable physical device
static VkPhysicalDevice dvk_physical_device_pick(VkInstance instance, VkSurfaceKHR surface)
{
    VkPhysicalDevice pd;

	u32 device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);

    if (device_count == 0)
	{
        dlog(NULL, "Failed to find GPUs with Vulkan support!");
		return FALSE;
	}
    
    VkPhysicalDevice devices[DG_PHYSICAL_DEVICE_MAX];
    vkEnumeratePhysicalDevices(instance, &device_count, devices);
	//@FIX(ilias): this is 1 here because llvmpipe is 0 and we don't want that! no vulkan 1.3!!
#ifdef BUILD_UNIX
    for (u32 i = 0; i < device_count; ++i)
#else
    for (u32 i = 0; i < device_count; ++i)
#endif
        if (dvk_is_physical_device_suitable(devices[i], surface))
			{
				pd = devices[i];

				VkPhysicalDeviceProperties p;
				vkGetPhysicalDeviceProperties(pd, &p);
                if (strstr(p.deviceName,"pipe")!=NULL)continue;
				dlog(NULL, "VULKAN: physical device picked: %s\n", p.deviceName);
				break;
			}
    
	assert(pd != VK_NULL_HANDLE);
	return pd;
}

//DOC: creates a suitable logical device and initializes the graphics and present queues (TODO maybe split this part?)
static VkDevice dvk_logical_device_create(VkPhysicalDevice pd, VkSurfaceKHR surface, VkQueue *gq, VkQueue*pq)
{
    VkDevice device = VK_NULL_HANDLE;
    u32 gf, pf;
    dvk_queue_families_find(pd, surface, &gf, &pf);
    
    f32 queue_priority = 1.0f;
	VkDeviceQueueCreateInfo queue_create_info[2] = {};

    queue_create_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info[0].queueFamilyIndex = gf;
    queue_create_info[0].queueCount = 1;
    queue_create_info[0].pQueuePriorities = &queue_priority;

    queue_create_info[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info[1].queueFamilyIndex = pf;
    queue_create_info[1].queueCount = 1;
    queue_create_info[1].pQueuePriorities = &queue_priority;
    VkPhysicalDeviceFeatures device_features = {};
    
    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    
    create_info.pQueueCreateInfos = &queue_create_info[0];
    create_info.queueCreateInfoCount = (gf != pf) ? 2 : 1;
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = array_count(device_extensions);
    create_info.ppEnabledExtensionNames = &device_extensions[0];

    if(TRUE)//if (enable_validation_layers)
    {
        create_info.enabledLayerCount = array_count(validation_layers);
        create_info.ppEnabledLayerNames = validation_layers;
    }
    else
        create_info.enabledLayerCount = 0;


    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_state = {};
    dynamic_rendering_state.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
    dynamic_rendering_state.pNext = NULL;
    dynamic_rendering_state.dynamicRendering = VK_TRUE;


    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state = {};
    extended_dynamic_state.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    extended_dynamic_state.pNext = &dynamic_rendering_state;
    extended_dynamic_state.extendedDynamicState = VK_TRUE;

    VkPhysicalDeviceMultiviewFeaturesKHR multiview_features = {};
    multiview_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR;
    multiview_features.multiview = VK_TRUE;
    multiview_features.pNext = &extended_dynamic_state;

    create_info.pNext = &multiview_features;

    VK_CHECK(vkCreateDevice(pd, &create_info, NULL, &device));

    
    vkGetDeviceQueue(device, gf, 0, gq);
    vkGetDeviceQueue(device, pf, 0, pq);

	return device;
}

//DOC: makes a valid command pool for a given surface and physical device
static VkCommandPool dvk_command_pool_create(VkDevice device, VkPhysicalDevice pd, VkSurfaceKHR surface)
{
    VkCommandPool pool;
    u32 gf, pf;
    dvk_queue_families_find(pd, surface, &gf, &pf);
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = gf;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK(vkCreateCommandPool(device, &pool_info, NULL, &pool));
    return pool;
}

//DOC: creates cmd_buffer_count command buffers from the given command pool
static VkCommandBuffer *dvk_command_buffers_create(VkDevice device, VkCommandPool pool, u32 cmd_buffer_count)
{
    VkCommandBuffer *command_buffers = (VkCommandBuffer*)dalloc(sizeof(VkCommandBuffer) * cmd_buffer_count);//ddev->swap.image_count);
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = pool; //where to allocate the buffer from
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = cmd_buffer_count;//ddev->swap.image_count;
    VK_CHECK(vkAllocateCommandBuffers(device, &alloc_info, command_buffers));
    return command_buffers;
}
//DOC: destroys (the memory of) the command_buffers, because the command_buffers don't need Destroy because they are freed along with the pool
static void dvk_command_buffers_destroy(VkCommandBuffer *command_buffers){
    if (command_buffers != NULL)
        dfree(command_buffers);
}


//------------------------------------------------------------------------------------------------------







typedef struct GlobalData
{
    mat4 view;
    mat4 proj;
    mat4 viewproj;
}GlobalData;


static VkFormat dg_to_vk_format(dgImageFormat format)
{
    //the enums are the same for the vulkan driver, for other implementations 
    //just switch all different formats to correct counterpart
    return (VkFormat)format;
}



static VkSurfaceFormatKHR dg_choose_swap_surface_format(dvkSwapChainSupportDetails details)
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

static VkPresentModeKHR dg_choose_swap_present_mode(dvkSwapChainSupportDetails details)
{
    return VK_PRESENT_MODE_FIFO_KHR;
    //return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

static VkExtent2D dg_choose_swap_extent(dvkSwapChainSupportDetails details)
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

    dvkSwapChainSupportDetails swap_details = dvk_swapchain_support(ddev->physical_device, ddev->surface);

    VkSurfaceFormatKHR surface_format = dg_choose_swap_surface_format(swap_details);
    VkPresentModeKHR present_mode = dg_choose_swap_present_mode(swap_details);
    VkExtent2D extent = dg_choose_swap_extent(swap_details);

    u32 image_count = swap_details.capabilities.minImageCount + 1;
    if (swap_details.capabilities.maxImageCount > 0 && image_count > swap_details.capabilities.maxImageCount)
        image_count = swap_details.capabilities.maxImageCount;
    image_count = minimum(image_count, MAX_SWAP_IMAGE_COUNT);

    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = ddev->surface;

    create_info.minImageCount = image_count+1;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    u32 gf,pf;
    dvk_queue_families_find(ddev->physical_device, ddev->surface, &gf, &pf);
    u32 queue_family_indices[] = {gf,pf};
    
    if (gf != pf)
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
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

static VkImageView dg_create_image_view(VkImage image, VkFormat format,VkImageAspectFlags aspect_flags, u32 layer_count, u32 base_layer, u32 mip_levels, u32 base_mip)
{
    VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;
    if (layer_count == 6)
        view_type = VK_IMAGE_VIEW_TYPE_CUBE;
    else if (layer_count > 1)
        view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    
	VkImageView image_view;
	
	VkImageViewCreateInfo view_info = {};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = image;
	view_info.viewType = view_type;
	view_info.format = format;
	view_info.subresourceRange.aspectMask = aspect_flags;
	view_info.subresourceRange.baseMipLevel = base_mip;
	view_info.subresourceRange.levelCount = mip_levels - base_mip;
	view_info.subresourceRange.baseArrayLayer = base_layer;
	view_info.subresourceRange.layerCount = layer_count - base_layer;
	VK_CHECK(vkCreateImageView(dd.device, &view_info, NULL, &image_view));
	return image_view;
}

static void dg_create_texture_sampler(dgDevice *ddev, VkSampler *sampler, u32 mip_levels, u32 clamp)
{
	VkSamplerCreateInfo sampler_info = {};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_NEAREST;//VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_NEAREST;//VK_FILTER_LINEAR;
    if (mip_levels <= 1 && clamp){
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    }else{
        sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
	

    
	
	VkPhysicalDeviceProperties prop = {};
	vkGetPhysicalDeviceProperties(ddev->physical_device, &prop);
	
	sampler_info.anisotropyEnable = VK_FALSE;
	sampler_info.maxAnisotropy = prop.limits.maxSamplerAnisotropy;
	//sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	//sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = (f32)(maximum(mip_levels-1,0));
	VK_CHECK(vkCreateSampler(ddev->device, &sampler_info, NULL, sampler));
}

static b32 dg_create_swapchain_image_views(dgDevice *ddev)
{
    ddev->swap.image_views = (VkImageView*)dalloc(sizeof(VkImageView) * ddev->swap.image_count);
    for (u32 i = 0; i < ddev->swap.image_count; ++i)
		ddev->swap.image_views[i] = dg_create_image_view(ddev->swap.images[i], ddev->swap.image_format,VK_IMAGE_ASPECT_COLOR_BIT,1,0,1,0);
    return DSUCCESS;
}


#include "dconfig.h"
extern dConfig engine_config;

VkShaderModule dg_create_shader_module(char *code, u32 size)
{
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = size;
	create_info.pCode = (u32*)code;
	VkShaderModule shader_module;
	VK_CHECK(vkCreateShaderModule(dd.device, &create_info, NULL, &shader_module));
	return shader_module;
}
static void InitResources(TBuiltInResource &Resources) {
		Resources.maxLights = 32;
		Resources.maxClipPlanes = 6;
		Resources.maxTextureUnits = 32;
		Resources.maxTextureCoords = 32;
		Resources.maxVertexAttribs = 64;
		Resources.maxVertexUniformComponents = 4096;
		Resources.maxVaryingFloats = 64;
		Resources.maxVertexTextureImageUnits = 32;
		Resources.maxCombinedTextureImageUnits = 80;
		Resources.maxTextureImageUnits = 32;
		Resources.maxFragmentUniformComponents = 4096;
		Resources.maxDrawBuffers = 32;
		Resources.maxVertexUniformVectors = 128;
		Resources.maxVaryingVectors = 8;
		Resources.maxFragmentUniformVectors = 16;
		Resources.maxVertexOutputVectors = 16;
		Resources.maxFragmentInputVectors = 15;
		Resources.minProgramTexelOffset = -8;
		Resources.maxProgramTexelOffset = 7;
		Resources.maxClipDistances = 8;
		Resources.maxComputeWorkGroupCountX = 65535;
		Resources.maxComputeWorkGroupCountY = 65535;
		Resources.maxComputeWorkGroupCountZ = 65535;
		Resources.maxComputeWorkGroupSizeX = 1024;
		Resources.maxComputeWorkGroupSizeY = 1024;
		Resources.maxComputeWorkGroupSizeZ = 64;
		Resources.maxComputeUniformComponents = 1024;
		Resources.maxComputeTextureImageUnits = 16;
		Resources.maxComputeImageUniforms = 8;
		Resources.maxComputeAtomicCounters = 8;
		Resources.maxComputeAtomicCounterBuffers = 1;
		Resources.maxVaryingComponents = 60;
		Resources.maxVertexOutputComponents = 64;
		Resources.maxGeometryInputComponents = 64;
		Resources.maxGeometryOutputComponents = 128;
		Resources.maxFragmentInputComponents = 128;
		Resources.maxImageUnits = 8;
		Resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
		Resources.maxCombinedShaderOutputResources = 8;
		Resources.maxImageSamples = 0;
		Resources.maxVertexImageUniforms = 0;
		Resources.maxTessControlImageUniforms = 0;
		Resources.maxTessEvaluationImageUniforms = 0;
		Resources.maxGeometryImageUniforms = 0;
		Resources.maxFragmentImageUniforms = 8;
		Resources.maxCombinedImageUniforms = 8;
		Resources.maxGeometryTextureImageUnits = 16;
		Resources.maxGeometryOutputVertices = 256;
		Resources.maxGeometryTotalOutputComponents = 1024;
		Resources.maxGeometryUniformComponents = 1024;
		Resources.maxGeometryVaryingComponents = 64;
		Resources.maxTessControlInputComponents = 128;
		Resources.maxTessControlOutputComponents = 128;
		Resources.maxTessControlTextureImageUnits = 16;
		Resources.maxTessControlUniformComponents = 1024;
		Resources.maxTessControlTotalOutputComponents = 4096;
		Resources.maxTessEvaluationInputComponents = 128;
		Resources.maxTessEvaluationOutputComponents = 128;
		Resources.maxTessEvaluationTextureImageUnits = 16;
		Resources.maxTessEvaluationUniformComponents = 1024;
		Resources.maxTessPatchComponents = 120;
		Resources.maxPatchVertices = 32;
		Resources.maxTessGenLevel = 64;
		Resources.maxViewports = 16;
		Resources.maxVertexAtomicCounters = 0;
		Resources.maxTessControlAtomicCounters = 0;
		Resources.maxTessEvaluationAtomicCounters = 0;
		Resources.maxGeometryAtomicCounters = 0;
		Resources.maxFragmentAtomicCounters = 8;
		Resources.maxCombinedAtomicCounters = 8;
		Resources.maxAtomicCounterBindings = 1;
		Resources.maxVertexAtomicCounterBuffers = 0;
		Resources.maxTessControlAtomicCounterBuffers = 0;
		Resources.maxTessEvaluationAtomicCounterBuffers = 0;
		Resources.maxGeometryAtomicCounterBuffers = 0;
		Resources.maxFragmentAtomicCounterBuffers = 1;
		Resources.maxCombinedAtomicCounterBuffers = 1;
		Resources.maxAtomicCounterBufferSize = 16384;
		Resources.maxTransformFeedbackBuffers = 4;
		Resources.maxTransformFeedbackInterleavedComponents = 64;
		Resources.maxCullDistances = 8;
		Resources.maxCombinedClipAndCullDistances = 8;
		Resources.maxSamples = 4;
		Resources.maxMeshOutputVerticesNV = 256;
		Resources.maxMeshOutputPrimitivesNV = 512;
		Resources.maxMeshWorkGroupSizeX_NV = 32;
		Resources.maxMeshWorkGroupSizeY_NV = 1;
		Resources.maxMeshWorkGroupSizeZ_NV = 1;
		Resources.maxTaskWorkGroupSizeX_NV = 32;
		Resources.maxTaskWorkGroupSizeY_NV = 1;
		Resources.maxTaskWorkGroupSizeZ_NV = 1;
		Resources.maxMeshViewCountNV = 4;
		Resources.limits.nonInductiveForLoops = 1;
		Resources.limits.whileLoops = 1;
		Resources.limits.doWhileLoops = 1;
		Resources.limits.generalUniformIndexing = 1;
		Resources.limits.generalAttributeMatrixVectorIndexing = 1;
		Resources.limits.generalVaryingIndexing = 1;
		Resources.limits.generalSamplerIndexing = 1;
		Resources.limits.generalVariableIndexing = 1;
		Resources.limits.generalConstantMatrixVectorIndexing = 1;
	}
/*
const glslang_resource_t* glslang_default_resource(void)
{
    return reinterpret_cast<const glslang_resource_t*>(&glslang::DefaultTBuiltInResource);
}
*/
static TBuiltInResource InitResources()
{
    TBuiltInResource Resources;

    Resources.maxLights                                 = 32;
    Resources.maxClipPlanes                             = 6;
    Resources.maxTextureUnits                           = 32;
    Resources.maxTextureCoords                          = 32;
    Resources.maxVertexAttribs                          = 64;
    Resources.maxVertexUniformComponents                = 4096;
    Resources.maxVaryingFloats                          = 64;
    Resources.maxVertexTextureImageUnits                = 32;
    Resources.maxCombinedTextureImageUnits              = 80;
    Resources.maxTextureImageUnits                      = 32;
    Resources.maxFragmentUniformComponents              = 4096;
    Resources.maxDrawBuffers                            = 32;
    Resources.maxVertexUniformVectors                   = 128;
    Resources.maxVaryingVectors                         = 8;
    Resources.maxFragmentUniformVectors                 = 16;
    Resources.maxVertexOutputVectors                    = 16;
    Resources.maxFragmentInputVectors                   = 15;
    Resources.minProgramTexelOffset                     = -8;
    Resources.maxProgramTexelOffset                     = 7;
    Resources.maxClipDistances                          = 8;
    Resources.maxComputeWorkGroupCountX                 = 65535;
    Resources.maxComputeWorkGroupCountY                 = 65535;
    Resources.maxComputeWorkGroupCountZ                 = 65535;
    Resources.maxComputeWorkGroupSizeX                  = 1024;
    Resources.maxComputeWorkGroupSizeY                  = 1024;
    Resources.maxComputeWorkGroupSizeZ                  = 64;
    Resources.maxComputeUniformComponents               = 1024;
    Resources.maxComputeTextureImageUnits               = 16;
    Resources.maxComputeImageUniforms                   = 8;
    Resources.maxComputeAtomicCounters                  = 8;
    Resources.maxComputeAtomicCounterBuffers            = 1;
    Resources.maxVaryingComponents                      = 60;
    Resources.maxVertexOutputComponents                 = 64;
    Resources.maxGeometryInputComponents                = 64;
    Resources.maxGeometryOutputComponents               = 128;
    Resources.maxFragmentInputComponents                = 128;
    Resources.maxImageUnits                             = 8;
    Resources.maxCombinedImageUnitsAndFragmentOutputs   = 8;
    Resources.maxCombinedShaderOutputResources          = 8;
    Resources.maxImageSamples                           = 0;
    Resources.maxVertexImageUniforms                    = 0;
    Resources.maxTessControlImageUniforms               = 0;
    Resources.maxTessEvaluationImageUniforms            = 0;
    Resources.maxGeometryImageUniforms                  = 0;
    Resources.maxFragmentImageUniforms                  = 8;
    Resources.maxCombinedImageUniforms                  = 8;
    Resources.maxGeometryTextureImageUnits              = 16;
    Resources.maxGeometryOutputVertices                 = 256;
    Resources.maxGeometryTotalOutputComponents          = 1024;
    Resources.maxGeometryUniformComponents              = 1024;
    Resources.maxGeometryVaryingComponents              = 64;
    Resources.maxTessControlInputComponents             = 128;
    Resources.maxTessControlOutputComponents            = 128;
    Resources.maxTessControlTextureImageUnits           = 16;
    Resources.maxTessControlUniformComponents           = 1024;
    Resources.maxTessControlTotalOutputComponents       = 4096;
    Resources.maxTessEvaluationInputComponents          = 128;
    Resources.maxTessEvaluationOutputComponents         = 128;
    Resources.maxTessEvaluationTextureImageUnits        = 16;
    Resources.maxTessEvaluationUniformComponents        = 1024;
    Resources.maxTessPatchComponents                    = 120;
    Resources.maxPatchVertices                          = 32;
    Resources.maxTessGenLevel                           = 64;
    Resources.maxViewports                              = 16;
    Resources.maxVertexAtomicCounters                   = 0;
    Resources.maxTessControlAtomicCounters              = 0;
    Resources.maxTessEvaluationAtomicCounters           = 0;
    Resources.maxGeometryAtomicCounters                 = 0;
    Resources.maxFragmentAtomicCounters                 = 8;
    Resources.maxCombinedAtomicCounters                 = 8;
    Resources.maxAtomicCounterBindings                  = 1;
    Resources.maxVertexAtomicCounterBuffers             = 0;
    Resources.maxTessControlAtomicCounterBuffers        = 0;
    Resources.maxTessEvaluationAtomicCounterBuffers     = 0;
    Resources.maxGeometryAtomicCounterBuffers           = 0;
    Resources.maxFragmentAtomicCounterBuffers           = 1;
    Resources.maxCombinedAtomicCounterBuffers           = 1;
    Resources.maxAtomicCounterBufferSize                = 16384;
    Resources.maxTransformFeedbackBuffers               = 4;
    Resources.maxTransformFeedbackInterleavedComponents = 64;
    Resources.maxCullDistances                          = 8;
    Resources.maxCombinedClipAndCullDistances           = 8;
    Resources.maxSamples                                = 4;
    Resources.maxMeshOutputVerticesNV                   = 256;
    Resources.maxMeshOutputPrimitivesNV                 = 512;
    Resources.maxMeshWorkGroupSizeX_NV                  = 32;
    Resources.maxMeshWorkGroupSizeY_NV                  = 1;
    Resources.maxMeshWorkGroupSizeZ_NV                  = 1;
    Resources.maxTaskWorkGroupSizeX_NV                  = 32;
    Resources.maxTaskWorkGroupSizeY_NV                  = 1;
    Resources.maxTaskWorkGroupSizeZ_NV                  = 1;
    Resources.maxMeshViewCountNV                        = 4;

    Resources.limits.nonInductiveForLoops                 = 1;
    Resources.limits.whileLoops                           = 1;
    Resources.limits.doWhileLoops                         = 1;
    Resources.limits.generalUniformIndexing               = 1;
    Resources.limits.generalAttributeMatrixVectorIndexing = 1;
    Resources.limits.generalVaryingIndexing               = 1;
    Resources.limits.generalSamplerIndexing               = 1;
    Resources.limits.generalVariableIndexing              = 1;
    Resources.limits.generalConstantMatrixVectorIndexing  = 1;

    return Resources;
}
void dg_shader_create_dynamic(VkDevice device, dgShader *s, char *filename, VkShaderStageFlagBits stage)
{
    char path[256];
    sprintf(path, "%s%s", engine_config.shader_path, filename);
    char *glsl_text;
    u32 glsl_size;
    //this means that either there is no such shader or that we entered a READY glsl 
    //this is slow though so we should probably have a distinct path for ready shaders 
    int res = read_file(path, (u32**)&glsl_text, &glsl_size);

    //glsl_text = read_whole_file_binary(path, &glsl_size);
	if (res == -1)
    {
        glsl_text = filename;
        glsl_size = str_size(glsl_text);
    }
    glsl_text[glsl_size] = 0;

    EShLanguage lstage = (stage &VK_SHADER_STAGE_VERTEX_BIT ) ? EShLangVertex : EShLangFragment;
    glslang::TShader shader(lstage);
    shader.setEnvInput(glslang::EShSourceGlsl, lstage, glslang::EShClientVulkan,1);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);
    
    glslang::TProgram program;
    const char *shaderStrings[1];
    TBuiltInResource Resources = InitResources();
    //InitResources(Resources);

    // Enable SPIR-V and Vulkan rules when parsing GLSL
    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    shaderStrings[0] = glsl_text;
    shader.setStrings(shaderStrings, 1);

    if (!shader.parse(&Resources, 100, false, messages)) {
        printf("filename: %s\n", filename);
        puts(shader.getInfoLog());
        puts(shader.getInfoDebugLog());
        exit(1);  // something didn't work
    }

    program.addShader(&shader);
    if (!program.link(messages)) {
        puts(shader.getInfoLog());
        puts(shader.getInfoDebugLog());
        fflush(stdout);
        exit(1);
    }

    std::vector<unsigned int> spirv{};
    glslang::GlslangToSpv(*program.getIntermediate(lstage), spirv);

    s->module = dg_create_shader_module((char*)spirv.data(), spirv.size() * sizeof(u32));
	s->uses_push_constants = FALSE;
	s->stage = stage;
    spvReflectCreateShaderModule(spirv.size() * sizeof(u32), (char*)spirv.data(), &s->info);
}  


void dg_shader_create(VkDevice device, dgShader *shader, char *filename, VkShaderStageFlagBits stage)
{
    char path[256];
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
    VkRect2D scissor = {};
    scissor.offset.x = x;
	scissor.offset.y = x;
    scissor.extent = (VkExtent2D){w,h};
    //scissor.extent.height *= fabs(sin(get_time()));
	
    return scissor;

}

VkPipelineDepthStencilStateCreateInfo dg_pipe_depth_stencil_state_create_info_basic(void)
{
    VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
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
	VkPipelineShaderStageCreateInfo info = {};
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

    for (u32 j = 0; j <shader->info.input_variable_count; ++j)
    {
        u32 attr_index = (u32)shader->info.input_variables[j]->location;
        if (shader->info.input_variables[j]->built_in != -1)
            --attribute_count;
    }  
    for (u32 i = 0; i < shader->info.input_variable_count; ++i)
    {
        for (u32 j = 0; j <shader->info.input_variable_count; ++j)
        {
            u32 attr_index = (u32)shader->info.input_variables[j]->location;
            if (shader->info.input_variables[j]->built_in != -1 || shader->info.input_variables[j]->location > 1000)
                continue;
            if (attr_index == i )//Note(inv): we want to write the inputs in order to get the global offset
            {
                SpvReflectInterfaceVariable *input_var = shader->info.input_variables[j];
                memset(&attr_desc[attr_index], 0, sizeof(VkVertexInputAttributeDescription));
                attr_desc[attr_index].binding = (pack_attribs) ? 0 : attr_index;
                attr_desc[attr_index].format = (VkFormat)input_var->format;
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
            if (attr_index > 10000)continue;
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
 
	VkPipelineVertexInputStateCreateInfo info = {};
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
	VkPipelineInputAssemblyStateCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	info.pNext = NULL;
	
	info.topology = topology;
	info.primitiveRestartEnable = VK_FALSE;
	return info;
}

static VkPipelineRasterizationStateCreateInfo dg_pipe_rasterization_state_create_info(VkPolygonMode polygon_mode)
{
	VkPipelineRasterizationStateCreateInfo info = {};
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
	VkPipelineMultisampleStateCreateInfo info = {};
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
	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
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
	VkPipelineColorBlendStateCreateInfo color_blending = {};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.pNext = NULL;
	
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY;
	color_blending.attachmentCount = attachment_count;
	VkAttachmentDescription color_attachment = {};
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
    VkPipelineViewportStateCreateInfo viewport_state = {};
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
            VkDescriptorSetLayoutBinding binding ={};
            binding.binding = current_set.bindings[j]->binding;
            binding.descriptorCount = 1;//current_set.bindings[j]->count;
            binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;
            binding.descriptorType = (VkDescriptorType)current_set.bindings[j]->descriptor_type;

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
            VkDescriptorSetLayoutCreateInfo desc_layout_ci = {};
            desc_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            desc_layout_ci.bindingCount = dbf_len(desc_set_layout_bindings);
            desc_layout_ci.pBindings = desc_set_layout_bindings;

            VK_CHECK(vkCreateDescriptorSetLayout(ddev->device, &desc_layout_ci, NULL, &layouts[current_set.set]));
            dg_descriptor_set_layout_cache_add(&ddev->desc_layout_cache, 
                (dgDescriptorSetLayoutInfo){total_hash, desc_set_layout_bindings}, layouts[current_set.set]);
            //dlog(NULL, "Created (another) DSL!! :( \n");
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
            VkDescriptorSetLayoutBinding binding ={};
            binding.binding = current_set.bindings[j]->binding; 
            binding.descriptorCount = current_set.bindings[j]->count;
            binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            binding.descriptorType = (VkDescriptorType)current_set.bindings[j]->descriptor_type;

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
            VkDescriptorSetLayoutCreateInfo desc_layout_ci = {};
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
    
	VkPipelineLayoutCreateInfo info = {};
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

    VkPipelineDepthStencilStateCreateInfo depth_info = {};

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
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT,
    };
    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.pNext = NULL;
    dynamic_state.dynamicStateCount = array_count(dynamic_state_enables);
    dynamic_state.pDynamicStates = dynamic_state_enables;

    
    VkDescriptorSetLayout layouts[DG_MAX_DESCRIPTOR_SETS];
    dg_pipe_descriptor_set_layout(ddev, &pipe->vert_shader, layouts);

    VkPipelineLayoutCreateInfo pipe_layout_info = 
    dg_pipe_layout_create_info(layouts, pipe->vert_shader.info.descriptor_set_count);

    VK_CHECK(vkCreatePipelineLayout(ddev->device, &pipe_layout_info, NULL, &pipe->pipeline_layout));


    VkGraphicsPipelineCreateInfo pipeCI = {};
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
        //and all RTs are RGBA16 @FIX this when time TODO TODO TODO WHY??!?!?!??
        if (output_var_count > 1 || strstr(vert_name, "ssao.vert") != NULL || strstr(vert_name, "brdf.vert") != NULL|| strstr(vert_name, "blur.vert") != NULL)
            color_formats[i] = VK_FORMAT_R16G16B16A16_SFLOAT;
        else if (strstr(vert_name, "cubemap_conv.vert") != NULL || strstr(vert_name, "skybox_gen.vert") != NULL|| strstr(vert_name, "prefilter_map.vert") != NULL) 
            color_formats[i] = VK_FORMAT_R32G32B32A32_SFLOAT;
        else 
            color_formats[i] = ddev->swap.image_format;
    }
 
    // New create info to define color, depth and stencil attachments at pipeline create time
    VkPipelineRenderingCreateInfoKHR pipe_renderingCI = {};
    pipe_renderingCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    pipe_renderingCI.colorAttachmentCount = output_var_count;
    pipe_renderingCI.pColorAttachmentFormats = color_formats;
    //TODO: fix view mask generation for each pipeline :P, this is a HUGE hack pls pls pls fix before too late
    if (strstr(vert_name, "sm.vert") != NULL)
        pipe_renderingCI.viewMask = 0b00000111;
    if (strstr(vert_name, "cubemap_conv.vert") != NULL || strstr(vert_name, "skybox_gen.vert") != NULL || strstr(vert_name, "prefilter_map.vert") != NULL)
        pipe_renderingCI.viewMask = 0b00111111;
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
    
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fence_info = {};
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
    dg_rt_cleanup(ddev, &composition_rt);
    dg_rt_init(&dd, &composition_rt, 1, TRUE, ddev->swap.extent.width, ddev->swap.extent.height);

    
    //dg_create_command_buffers(ddev);
    dd.command_buffers = dvk_command_buffers_create(ddev->device, ddev->command_pool, ddev->swap.image_count);
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
			VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
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
    VkCommandBufferBeginInfo begin_info = {};
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
    //free(da->pool_sizes);
}

//this is TERRIBLY SLOWWW~!!!!! @FIX (maybe we don't need to fix because a single pool handles a lot of descriptors??? @inspect)
static VkDescriptorPool dg_create_descriptor_pool(dgDescriptorAllocator *da, u32 count, VkDescriptorPoolCreateFlags flags)
{
    VkDescriptorPoolSize *sizes = NULL;
    for (u32 i = 0; i <= 10; ++i)
    {
        dbf_push(sizes, (VkDescriptorPoolSize){VkDescriptorType(i), count * da->pool_sizes[H32_static_get(&da->desc_type_hash, i)]});
    }

    VkDescriptorPoolCreateInfo pool_info = {};
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

    VkDescriptorSetAllocateInfo alloc_info = {};
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

void dg_rendering_begin(dgDevice *ddev, dgTexture *tex, u32 attachment_count, dgTexture *depth_tex, dgRenderingSettings settings)
{
    b32 clear_color = settings & DG_RENDERING_SETTINGS_CLEAR_COLOR; 
    b32 clear_depth = settings & DG_RENDERING_SETTINGS_CLEAR_DEPTH; 
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
        color_attachments[0].clearValue.color = (VkClearColorValue){0.05,0.05,0.05,1};
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
    VkRenderingAttachmentInfoKHR depth_attachment = {};
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


    VkRenderingInfoKHR rendering_info = {};
    rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    if (tex == NULL)
        rendering_info.renderArea = (VkRect2D){0,0, dd.swap.extent.width, dd.swap.extent.height};
    else
        rendering_info.renderArea = (VkRect2D){0,0, tex->width,tex->height};
    rendering_info.layerCount = 1;
    rendering_info.colorAttachmentCount = (tex == NULL) ? 1 : attachment_count;
    rendering_info.pColorAttachments = color_attachments;
    rendering_info.pDepthAttachment = &depth_attachment;

    if (settings & DG_RENDERING_SETTINGS_MULTIVIEW){
        u32 layer_count= 0;
        if (tex)
            layer_count = maximum(tex->layer_count, layer_count);
        if (depth_tex)
            layer_count = maximum(depth_tex->layer_count, layer_count);
        b32 view_mask = (1 << layer_count) - 1;
        rendering_info.viewMask =  view_mask;//DG_MAX_CASCADES-1;
        rendering_info.layerCount = layer_count;
    }
    if (settings & DG_RENDERING_SETTINGS_DEPTH_DISABLE){
        rendering_info.pDepthAttachment = VK_NULL_HANDLE;
    }
    
    //rendering_info.pStencilAttachment = &depth_attachment;
    rendering_info.pStencilAttachment = VK_NULL_HANDLE; //TODO: this should be NULL only if depth+stencil=depth

    vkCmdBeginRenderingKHR(ddev->command_buffers[ddev->current_frame], &rendering_info);
    vkCmdSetDepthTestEnableEXT(ddev->command_buffers[ddev->current_frame],(settings & DG_RENDERING_SETTINGS_DEPTH_DISABLE) ? VK_FALSE : VK_TRUE);
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
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateBuffer(dd.device, &buffer_info, NULL, &(buf->buffer) ));
    
	//[1]: create the memory nacking up the buffer handle
    VkMemoryRequirements mem_req = {};
    vkGetBufferMemoryRequirements(buf->device, (buf->buffer), &mem_req);
    
    VkMemoryAllocateInfo alloc_info = {};
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
    return VK_FORMAT_R8G8B8A8_SRGB;
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

static void dg_create_image(dgDevice *ddev, u32 width, u32 height, VkFormat format,u32 mip_levels, u32 layers, VkImageTiling tiling, 
VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *image_memory)
{
	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1;
	image_info.mipLevels = mip_levels;
	image_info.arrayLayers = layers;
	image_info.format = format;
	image_info.tiling = tiling;
	image_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	image_info.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.flags = (image_info.arrayLayers == 6) ?  VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
	VK_CHECK(vkCreateImage(ddev->device, &image_info, NULL, image));
	
	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(ddev->device, *image, &mem_req);
	
	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = find_mem_type(mem_req.memoryTypeBits, properties);
    VK_CHECK(vkAllocateMemory(ddev->device, &alloc_info, NULL, image_memory));
	vkBindImageMemory(ddev->device, *image, *image_memory, 0);
}


VkCommandBuffer dg_begin_single_time_commands(dgDevice *ddev)
{
	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = ddev->command_pool;
	alloc_info.commandBufferCount = 1;
	
	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(ddev->device, &alloc_info, &command_buffer);
	
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(command_buffer, &begin_info);
	
	return command_buffer;
}

void dg_end_single_time_commands(dgDevice *ddev, VkCommandBuffer command_buffer)
{
	vkEndCommandBuffer(command_buffer);
	
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;
	
	vkQueueSubmit(ddev->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(ddev->graphics_queue);
	vkFreeCommandBuffers(ddev->device, ddev->command_pool, 1, &command_buffer);
}

static dgTexture dg_create_depth_attachment(dgDevice *ddev, u32 width, u32 height, u32 layer_count)
{
	dgTexture depth_attachment = {};
	depth_attachment.format = dg_find_depth_format(ddev);
	
	dg_create_image(ddev,width, height, 
		depth_attachment.format,1, layer_count,VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depth_attachment.image, &depth_attachment.mem);


    depth_attachment.mip_levels = 1;
    depth_attachment.view = dg_create_image_view(depth_attachment.image, depth_attachment.format,VK_IMAGE_ASPECT_DEPTH_BIT,layer_count,0,depth_attachment.mip_levels,0);
    
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

    dg_create_texture_sampler(ddev, &depth_attachment.sampler,depth_attachment.mip_levels,1);
    
	return depth_attachment;
}



static void dg_generate_mips(VkImage image, s32 tex_w, s32 tex_h, u32 mip_levels,u32 layer_count) {
    VkCommandBuffer cmd_buf = dg_begin_single_time_commands(&dd);//beginSingleTimeCommands();

    ///*
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layer_count;
    barrier.subresourceRange.levelCount = 1;
    //*/

    s32 mip_w = tex_w;
    s32 mip_h = tex_h;

    for (u32 i = 1; i < mip_levels;++i){

        //prepare mip i as blit destination (TRANSFER_DST)
        dg_image_memory_barrier(
            cmd_buf,
            image,
            0, 
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            (VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, layer_count }
        );

        //blit from previous level (0->1->2,...)
        VkImageBlit blit = {};
        blit.srcOffsets[0] = (VkOffset3D){ 0, 0, 0 };
        blit.srcOffsets[1] = (VkOffset3D){ mip_w, mip_h, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;//src
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = (VkOffset3D){ 0, 0, 0 };
        blit.dstOffsets[1] = (VkOffset3D){ mip_w > 1 ? mip_w / 2 : 1, mip_h > 1 ? mip_h / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; //mipmapping does not happen in depth textures
        blit.dstSubresource.mipLevel = i;//dst
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;
        vkCmdBlitImage(cmd_buf,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);


        //prepare current mip to be blit src for next level
        dg_image_memory_barrier(
            cmd_buf,
            image,
            VK_ACCESS_TRANSFER_WRITE_BIT, 
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            (VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, i, 1, 0, layer_count }
        );
        
        if (mip_w >1)mip_w/=2;
        if (mip_h >1)mip_h/=2;
    }
    //when we are finished, we trasition all mips from SRC_OPTIMAL to SHADER_READ
    dg_image_memory_barrier(
        cmd_buf,
        image,
        VK_ACCESS_TRANSFER_READ_BIT, 
        VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        (VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, mip_levels, 0, layer_count }
    );

    dg_end_single_time_commands(&dd, cmd_buf);
}

dgTexture dg_create_texture_image_wdata(dgDevice *ddev,void *data, u32 tex_w,u32 tex_h, dgImageFormat format, u32 layer_count, u32 mip_levels)
{
    dgTexture tex;//={}??
	dgBuffer idb;
    u32 format_size;
    VkFormat vk_format = dg_to_vk_format(format);
    format_size = (vk_format == VK_FORMAT_R8_UINT) ? sizeof(u8) : sizeof(u8)*4;
    tex.mip_levels = mip_levels;
    tex.format = vk_format; //TODO: make this format also in-engine

    //TODO, make it possible to have 6-layer regular image arrays, not only cubes
    b32 is_cube = (layer_count == 6);

	dg_create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
	(VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), &idb, tex_w * tex_h * format_size, data);
	dg_create_image(ddev, tex_w, tex_h, vk_format,tex.mip_levels, layer_count,VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT 
		| VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &tex.image, &tex.mem);
	

    VkCommandBuffer cmd = dg_begin_single_time_commands(ddev);
    dg_image_memory_barrier(
        cmd,
        tex.image,
        0, 
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        (VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layer_count }
    );



    //if there is data to be copied, copy it
    if (data != NULL)
    {
        VkBufferImageCopy region = {};
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
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );
    }
    dg_image_memory_barrier(
        cmd,
        tex.image,
        VK_ACCESS_TRANSFER_WRITE_BIT, 
        VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        (VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, layer_count }
    );
    
    dg_end_single_time_commands(ddev, cmd);

    dg_generate_mips(tex.image, tex_w, tex_h, tex.mip_levels, layer_count);
	
    dg_buf_destroy(&idb);
	

	
	tex.width = tex_w;
	tex.height = tex_h;
    tex.image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    tex.layer_count = layer_count;
    tex.view = dg_create_image_view(tex.image, vk_format, VK_IMAGE_ASPECT_COLOR_BIT,layer_count,0, tex.mip_levels,0);
    dg_create_texture_sampler(ddev, &tex.sampler, tex.mip_levels,0);

	return tex;
}

dgTexture dg_create_texture_image(dgDevice *ddev, char *filename, dgImageFormat format)
{
	dgTexture tex;
	//[0]: we read an image and store all the pixels in a pointer
	s32 tex_w, tex_h, tex_c;
	void *pixels;
    b32 is_hdr = strstr(filename, ".hdr") != NULL;
    if (is_hdr)
        pixels = stbi_loadf(filename, &tex_w, &tex_h, &tex_c, STBI_rgb_alpha);
    else
        pixels = stbi_load(filename, &tex_w, &tex_h, &tex_c, STBI_rgb_alpha);
	VkDeviceSize image_size = tex_w * tex_h * 4;
    if (is_hdr)
        image_size = tex_w * tex_h * (sizeof(f32)) * 4;
	tex.mip_levels = (is_hdr) ? 1 : (u32)(floor(log2(maximum(tex_w, tex_h)))) + 1;
	
	//[2]: we create a buffer to hold the pixel information (we also fill it)
	dgBuffer idb;
	if (!pixels)
		printf("Error loading image %s!", filename);
	dg_create_buffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
	(VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), &idb, image_size, pixels);
	//[3]: we free the cpu side image, we don't need it
	stbi_image_free(pixels);
	//[4]: we create the VkImage that is undefined right now
	dg_create_image(ddev, tex_w, tex_h, dg_to_vk_format(format),tex.mip_levels, 1,VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT 
		| VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &tex.image, &tex.mem);
	

    VkCommandBuffer cmd = dg_begin_single_time_commands(ddev);
    dg_image_memory_barrier(
        cmd,
        tex.image,
        0, 
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        (VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    );




    VkBufferImageCopy region = {};
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
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);
    

    dg_image_memory_barrier(
        cmd,
        tex.image,
        VK_ACCESS_TRANSFER_WRITE_BIT, 
        VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        (VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    );

/*
    dg_image_memory_barrier(
        cmd,
        tex.image,
        VK_ACCESS_TRANSFER_READ_BIT, 
        VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        (VkImageSubresourceRange){ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
    );
*/
    dg_end_single_time_commands(ddev, cmd);
    dg_generate_mips(tex.image, tex_w, tex_h, tex.mip_levels, 1);

	dg_buf_destroy(&idb);
	
	
	tex.view = dg_create_image_view(tex.image, dg_to_vk_format(format),VK_IMAGE_ASPECT_COLOR_BIT,1,0, tex.mip_levels,0);
	
	tex.width = tex_w;
	tex.height = tex_h;
    tex.format = dg_to_vk_format(format);
    tex.image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	dg_create_texture_sampler(ddev, &tex.sampler, tex.mip_levels,0);
	sprintf(tex.name, filename);
	return tex;
}

static void dg_rt_init_csm(dgDevice *ddev, dgRT* rt,u32 cascade_count, u32 width, u32 height)
{
    rt->color_attachment_count = 1;
    rt->depth_active = TRUE;
    rt->cascaded_depth = TRUE;
    rt->depth_attachment = dg_create_depth_attachment(ddev, width,height,cascade_count);
    rt->color_attachments[0] = dg_create_texture_image_wdata(ddev, NULL, width, height, DG_IMAGE_FORMAT_RGBA8_SRGB, cascade_count,1);

    rt->cascades_count = cascade_count;
}

static void dg_rt_init(dgDevice *ddev, dgRT* rt, u32 color_count, b32 depth, u32 width, u32 height)
{
    rt->color_attachment_count = color_count;
    rt->depth_active = (depth > 0) ? 1 : 0;
    for (u32 i = 0; i < rt->color_attachment_count; ++i)
    {
        //rt->color_attachments[i] = dg_create_texture_image_basic(ddev,width,height,ddev->swap.image_format);
        //rt->color_attachments[i] = dg_create_texture_image_basic(ddev,width,height,VK_FORMAT_R16G16B16A16_SFLOAT);
        rt->color_attachments[i] = dg_create_texture_image_wdata(ddev, NULL, width, height, (color_count > 1) ? (dgImageFormat)DG_IMAGE_FORMAT_RGBA16_SFLOAT: (dgImageFormat)dd.swap.image_format, 1, 1);
    }
    if (rt->depth_active)
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

    VkWriteDescriptorSet set_write = {};
    set_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    set_write.dstBinding = 0;
    set_write.dstSet = set;
    set_write.descriptorCount = 1;
    set_write.descriptorType= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    set_write.pBufferInfo = &ubo_info;

    vkUpdateDescriptorSets(ddev->device, 1, &set_write, 0, NULL);
}

static void dg_update_desc_set_image(dgDevice *ddev, VkDescriptorSet set, dgTexture **textures, u32 view_count)
{
    assert(textures);
    VkDescriptorImageInfo image_infos[DG_MAX_DESCRIPTOR_SET_BINDINGS];
    for (u32 i = 0; i < view_count; ++i)
    {
        memset(&image_infos[i], 0, sizeof(VkDescriptorImageInfo));
        image_infos[i].imageView = textures[i]->view;
        image_infos[i].sampler = textures[i]->sampler;
        image_infos[i].imageLayout = textures[i]->image_layout;
    }

    VkWriteDescriptorSet set_write = {};
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
    DPROFILER_START("desc_update");
    //first we get the layout, then we 
    VkDescriptorSet desc_set;
    VkDescriptorSetLayout layout;
    layout = dg_get_descriptor_set_layout(ddev, &pipe->vert_shader, set_num);
    dg_descriptor_allocator_allocate(&ddev->desc_alloc[ddev->current_frame], &desc_set, layout);
    if(set_num == 2) //because set number 2 is for images
    {
        dgTexture **tex_data = (dgTexture**)data;
        u32 tex_count = size;
        dg_update_desc_set_image(ddev, desc_set, tex_data, tex_count);
    }
    else
    {
        dg_update_desc_set(ddev, desc_set, data, size);
    }
    vkCmdBindDescriptorSets(ddev->command_buffers[ddev->current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->pipeline_layout, set_num,1, &desc_set,0,NULL); 
    DPROFILER_END();
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
    dg_rendering_begin(ddev, def_rt.color_attachments, 3, &def_rt.depth_attachment, DG_RENDERING_SETTINGS_NONE);
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

void draw_cube_def_shadow(dgDevice *ddev, mat4 model, mat4 *lsms, u32 cascade_index)
{
    if (!ddev->shadow_pass_active)return;
    
    dg_rendering_begin(ddev, &csm_rt.color_attachments[0], 0, &csm_rt.depth_attachment, DG_RENDERING_SETTINGS_MULTIVIEW);
    dg_set_viewport(ddev, 0,0,csm_rt.color_attachments[0].width, csm_rt.color_attachments[0].height);
    dg_set_scissor(ddev, 0,0,csm_rt.color_attachments[0].width, csm_rt.color_attachments[0].height);
    dg_bind_pipeline(ddev, &ddev->shadow_pipe);
    dgBuffer buffers[] = {base_vbo};
    u64 offsets[] = {0};
    dg_bind_vertex_buffers(ddev, buffers, offsets, 1);
    dg_bind_index_buffer(ddev, &base_ibo, 0);

    mat4 object_data[5] = {model};
    memcpy(&object_data[1],lsms,sizeof(mat4)*4);
    //object_data[1]= lsms[cascade_index];
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

extern dEntity child,child2, parent;
extern dTransformCM transform_manager;
void draw_cube(dgDevice *ddev, mat4 model)
{
    dg_rendering_begin(ddev, &composition_rt.color_attachments[0], 1, &def_rt.depth_attachment, DG_RENDERING_SETTINGS_NONE);
    dg_set_viewport(ddev,0,0, composition_rt.color_attachments[0].width, composition_rt.color_attachments[0].height);
    dg_set_scissor(ddev, 0,0, composition_rt.color_attachments[0].width, composition_rt.color_attachments[0].height);
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


void draw_sphere(dgDevice *ddev, mat4 model)
{
    dg_rendering_begin(ddev, &composition_rt.color_attachments[0], 1, &def_rt.depth_attachment, DG_RENDERING_SETTINGS_NONE);
    dg_set_viewport(ddev,0,0, composition_rt.color_attachments[0].width, composition_rt.color_attachments[0].height);
    dg_set_scissor(ddev, 0,0, composition_rt.color_attachments[0].width, composition_rt.color_attachments[0].height);
    dg_bind_pipeline(ddev, &ddev->base_pipe);
    dgBuffer buffers[] = {sphere_vbo};
    u64 offsets[] = {0,0,0,0};
    dg_bind_vertex_buffers(ddev, buffers, offsets, 1);
    dg_bind_index_buffer(ddev, &sphere_ibo, 0);

    //mat4 data[4] = {0.9,(sin(0.02 * dtime_sec(dtime_now()))),0.2,0.2};
    //mat4 object_data = mat4_mul(mat4_translate(v3(0,1 * fabs(sin(5 * dtime_sec(dtime_now()))),-15)), mat4_rotate(90 * dtime_sec(dtime_now()), v3(0.2,0.4,0.7)));
    mat4 object_data[2] = {model, {1.0,1.0,1.0,1.1,0.0,1.0}};
    dg_set_desc_set(ddev,&ddev->base_pipe, object_data, sizeof(object_data), 1);
    dg_draw(ddev, 24,sphere_ibo.size/sizeof(u16));

    dg_rendering_end(ddev);
}

void dg_skybox_prepare(dgDevice *ddev)
{
    dgTexture *texture_slots[DG_MAX_DESCRIPTOR_SET_BINDINGS];
    mat4 capture_proj = perspective_proj(90.0f, 1, 0.01, 100);
    mat4 capture_views[6] =
    {
        look_at(v3(0,0,0),v3(1,0,0), v3(0,-1,0)),
        look_at(v3(0,0,0),v3(-1,0,0), v3(0,-1,0)),
        look_at(v3(0,0,0),v3(0,-1,0), v3(0,0,-1)),
        look_at(v3(0,0,0),v3(0,1,0), v3(0,0,1)),
        look_at(v3(0,0,0),v3(0,0,1), v3(0,-1,0)),
        look_at(v3(0,0,0),v3(0,0,-1), v3(0,-1,0)),
    };


    dg_rendering_begin(ddev, &cube_tex, 1,NULL, DG_RENDERING_SETTINGS_DEPTH_DISABLE | DG_RENDERING_SETTINGS_MULTIVIEW);
    dg_set_viewport(ddev, 0,0,cube_tex.width, cube_tex.height); //TODO: 1024 should be #DEFINE'd as skybox size or sth
    dg_set_scissor(ddev, 0,0,cube_tex.width, cube_tex.height);
    dg_bind_pipeline(ddev, &ddev->cubemap_conv_pipe);
    dgBuffer buffers[] = {base_pos};
    u64 offsets[] = {0};
    dg_bind_vertex_buffers(ddev, buffers, offsets, 1);
    dg_bind_index_buffer(ddev, &base_ibo, 0);
    mat4 cube_data[]= {capture_proj, capture_views[0],capture_views[1],
                        capture_views[2],capture_views[3],capture_views[4],capture_views[5]};
    dg_set_desc_set(ddev,&ddev->cubemap_conv_pipe, cube_data, sizeof(cube_data), 1);
    texture_slots[0] = &hdr_map;
    dg_set_desc_set(ddev,&ddev->cubemap_conv_pipe, texture_slots, 1, 2);
    dg_draw(ddev, 24,base_ibo.size/sizeof(u16));
    dg_rendering_end(ddev);
    
    ///*
    //DONT FORGET DIFFERENT MIPS HAVE LESS SIZE (W/H), KEEP IN MIND FOR SHADER INVOC
    //VkImageView mip_view = dg_create_image_view(prefilter_map.image, prefilter_map.format,VK_IMAGE_ASPECT_COLOR_BIT, prefilter_map.layer_count, 0, prefilter_map.mip_levels, 1);
    //prefilter_map.view = mip_view;
    //*/
    //TODO i < 4 should be #defined
    ///*
    
    for (u32 i = 0; i < 4; ++i){
        float roughness = i / 3.0;
        VkImageView mip_view = dg_create_image_view(prefilter_map.image, prefilter_map.format,VK_IMAGE_ASPECT_COLOR_BIT, prefilter_map.layer_count, 0, prefilter_map.mip_levels, i);
        prefilter_map.view = mip_view;
        mat4 cube_data_mip[]= {capture_proj, capture_views[0],capture_views[1],
                        capture_views[2],capture_views[3],capture_views[4],capture_views[5], {roughness}};
        dg_rendering_begin(ddev, &prefilter_map, 1,NULL, DG_RENDERING_SETTINGS_DEPTH_DISABLE | DG_RENDERING_SETTINGS_MULTIVIEW);
        dg_set_viewport(ddev, 0,0,prefilter_map.width/(i+1.0), prefilter_map.height/(i+1.0));
        dg_set_scissor(ddev, 0,0,prefilter_map.width/(i+1.0), prefilter_map.height/(i+1.0));
        dg_bind_pipeline(ddev, &ddev->prefilter_map_pipe);
        dg_bind_vertex_buffers(ddev, buffers, offsets, 1);
        dg_bind_index_buffer(ddev, &base_ibo, 0);
         
        dg_set_desc_set(ddev,&ddev->prefilter_map_pipe, cube_data_mip, sizeof(cube_data_mip), 1);
        texture_slots[0] = &cube_tex;
        dg_set_desc_set(ddev,&ddev->prefilter_map_pipe, texture_slots, 1, 2);
        dg_draw(ddev, 24,base_ibo.size/sizeof(u16));
        dg_rendering_end(ddev);
    }
    VkImageView mip_view = dg_create_image_view(prefilter_map.image, prefilter_map.format,VK_IMAGE_ASPECT_COLOR_BIT, prefilter_map.layer_count, 0, prefilter_map.mip_levels, 0);
    prefilter_map.view = mip_view;
    //*/
    
    

    ///* Draw the brdfLUT texture
    dg_rendering_begin(ddev, &brdfLUT, 1,NULL, DG_RENDERING_SETTINGS_DEPTH_DISABLE);
    dg_set_viewport(ddev, 0,0,brdfLUT.width, brdfLUT.height);
    dg_set_scissor(ddev, 0,0,brdfLUT.width, brdfLUT.height);
    dg_bind_pipeline(ddev, &ddev->brdf_lut_pipe);
    dg_draw(ddev,4,0);
    dg_rendering_end(ddev);
    //*/

    //TODO we should draw this to a separate cubemap and then apply that with a shader for every render :P
    //Its too expensive done on a per-frame basis
    //draw the skybox/irradiance map! TODO this should be done AFTER deferred composition, but I can't infer fragment depth in comp FS
    dg_rendering_begin(ddev, &irradiance_map, 1,NULL, DG_RENDERING_SETTINGS_MULTIVIEW | DG_RENDERING_SETTINGS_DEPTH_DISABLE);
    dg_set_viewport(ddev, 0,0,64, 64);
    dg_set_scissor(ddev, 0,0,64, 64);
    dg_bind_pipeline(ddev, &ddev->skybox_gen_pipe);
    dg_bind_vertex_buffers(ddev, buffers, offsets, 1);
    dg_bind_index_buffer(ddev, &base_ibo, 0);
    dg_set_desc_set(ddev,&ddev->skybox_gen_pipe, cube_data, sizeof(cube_data), 1);
    texture_slots[0] =&cube_tex;
    dg_set_desc_set(ddev,&ddev->skybox_gen_pipe, texture_slots, 1, 2);
    dg_draw(ddev, 24,base_ibo.size/sizeof(u16));
    dg_rendering_end(ddev);

    //printf("ddone at: %f seconds\n", (f32)dtime_sec(dtime_now()));
    return;
    
}
    

b32 dg_frame_begin(dgDevice *ddev)
{
    vkWaitForFences(ddev->device, 1, &ddev->in_flight_fences[ddev->current_frame], VK_TRUE, UINT64_MAX);
    vkResetFences(ddev->device, 1, &ddev->in_flight_fences[ddev->current_frame]);
    dgTexture *texture_slots[DG_MAX_DESCRIPTOR_SET_BINDINGS];


    VkResult res = vkAcquireNextImageKHR(ddev->device, ddev->swap.swapchain, UINT64_MAX, ddev->image_available_semaphores[ddev->current_frame],
        VK_NULL_HANDLE, &ddev->image_index);

    if (res == VK_ERROR_OUT_OF_DATE_KHR) { dg_recreate_swapchain(ddev); return FALSE; }
    else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)printf("Failed to acquire swapchain image!\n");

    vkResetCommandBuffer(ddev->command_buffers[ddev->current_frame],0);



    dg_ubo_data_buffer_clear(&ddev->ubo_buf, ddev->current_frame);
    dg_descriptor_allocator_reset_pools(&ddev->desc_alloc[ddev->current_frame]);

    dg_prepare_command_buffer(ddev, ddev->command_buffers[ddev->current_frame]);
    //CLEAR ALL FBO's before drawing
    
    //clear deferred FBO
    dg_rendering_begin(ddev, def_rt.color_attachments, 4, &def_rt.depth_attachment, DG_RENDERING_SETTINGS_CLEAR_COLOR|DG_RENDERING_SETTINGS_CLEAR_DEPTH);
    dg_rendering_end(ddev);
    //clear the CSM shadowmap
    dg_rendering_begin(ddev, csm_rt.color_attachments, 1, &csm_rt.depth_attachment,DG_RENDERING_SETTINGS_MULTIVIEW | DG_RENDERING_SETTINGS_CLEAR_COLOR|DG_RENDERING_SETTINGS_CLEAR_DEPTH);
    dg_rendering_end(ddev);
    //clear swapchain?
    dg_rendering_begin(ddev, NULL, 1, NULL, DG_RENDERING_SETTINGS_CLEAR_COLOR | DG_RENDERING_SETTINGS_CLEAR_DEPTH);
    dg_rendering_end(ddev);


    
    view = cam.getViewMatrix();
    proj = perspective_proj(60.0f,(main_editor.viewport.z-main_editor.viewport.x)/(main_editor.viewport.w -main_editor.viewport.y), 0.01, 100);



    //set desc set 0
    mat4 data[4] = {view, proj, m4d(1.0f),m4d(1.0f)};
    dg_set_desc_set(ddev,&ddev->def_pipe, data, sizeof(data), 0);
     
    
    draw_cube_def(ddev, mat4_mul(mat4_translate(v3(1 * fabs(sin(5 * dtime_sec(dtime_now()))),0,0)), 
        mat4_rotate(90 * dtime_sec(dtime_now()), v3(0.2,0.4,0.7))), v4(1,1,1,1), v4(1,0,1,1));
    draw_cube_def(ddev, mat4_mul(mat4_translate(v3(0,-3,0)),mat4_scale(v3(100,1,100))), v4(0.05,0.05,0.05,1), v4(0.9,0.2,0.2,1));
    draw_cube_def(ddev, mat4_translate(v3(4,0,0)), v4(1,0,0,1), v4(0,1,1,1));
    draw_cube_def(ddev, mat4_translate(v3(8,0,0)), v4(1,0,1,1), v4(1,1,0,1));
    draw_cube_def(ddev, mat4_translate(v3(16,0,0)), v4(1,1,0,1), v4(0,1,1,1));
    //draw_model_def(ddev, &water_bottle,mat4_mul(mat4_translate(v3(0,3,0)), mat4_mul(mat4_rotate(0 * dtime_sec(dtime_now()) / 8.0f, v3(0,1,0)),mat4_scale(v3(1,1,1)))));
    
    draw_model_def(ddev, &water_bottle,mat4_mul(mat4_translate(v3(0,3,0)), mat4_mul(mat4_rotate(-90 * dtime_sec(dtime_now()) / 20.f, v3(0,1,0.2)),mat4_mul(mat4_rotate(90, v3(1,0,0)),mat4_scale(v3(2,2,2))))));
    



    light_dir = vec3_mulf(vec3_normalize(v3(0,0.9,0.3)), 1);
    u32 cascade_count = 3;
    dg_calc_lsm(light_dir, proj, view, lsm,fdist, cascade_count);
    //draw to shadow map
    draw_cube_def_shadow(ddev, mat4_mul(mat4_translate(v3(1 * fabs(sin(5 * dtime_sec(dtime_now()))),0,0)), 
        mat4_rotate(90 * dtime_sec(dtime_now()), v3(0.2,0.4,0.7))), lsm,0);
    draw_cube_def_shadow(ddev, mat4_mul(mat4_translate(v3(0,-3,0)),mat4_scale(v3(100,1,100))), lsm,0);
    draw_cube_def_shadow(ddev, mat4_translate(v3(4,0,0)), lsm,0);
    draw_cube_def_shadow(ddev, mat4_translate(v3(8,0,0)), lsm,0);
    draw_cube_def_shadow(ddev, mat4_translate(v3(16,0,0)), lsm,0);
    //draw_model_def_shadow(ddev, &water_bottle,mat4_mul(mat4_translate(v3(0,3,0)), mat4_mul(mat4_rotate(100 * dtime_sec(dtime_now()) / 8.0f, v3(1,1,0)),mat4_scale(v3(10,10,10)))),lsm);
    //draw_model_def_shadow(ddev, &water_bottle,mat4_mul(mat4_translate(v3(0,3,0)), mat4_mul(mat4_rotate(0 * dtime_sec(dtime_now()) / 8.0f, v3(1,1,0)),mat4_scale(v3(1,1,1)))),lsm);
    //draw_model_def_shadow(ddev, &water_bottle,mat4_mul(mat4_translate(v3(0,3,0)), mat4_mul(mat4_rotate(-90 * dtime_sec(dtime_now()) / 20.f, v3(0,1,0.2)),mat4_mul(mat4_rotate(90, v3(1,0,0)),mat4_scale(v3(2,2,2))))), lsm);



    
    


    //SSAO pass, TODO: should we synchronize this with the end of the deferred pass? maybe a barrier
    vec4 ssao_kernel[32];
    for (u32 i = 0; i < 32; ++i){
        vec3 sample = v3( r01() * 2 - 1 , r01() * 2 - 1, r01() * 2);
        sample = vec3_normalize(sample);
        sample = vec3_mulf(sample, r01());
        
        float scale = ((f32)i) / 32.0f;
        sample = vec3_mulf(sample,lerp(0.1f, 1.0f, scale * scale));
        ssao_kernel[i] = v4(sample.x,sample.y,sample.z,1);
    }
    dg_rendering_begin(ddev, &ssao_tex, 1,NULL, DG_RENDERING_SETTINGS_DEPTH_DISABLE);
    dg_set_viewport(ddev, 0,0,ssao_tex.width, ssao_tex.height);
    dg_set_scissor(ddev, 0,0,ssao_tex.width, ssao_tex.height);
    dg_bind_pipeline(ddev, &ddev->ssao_pipe);
    dg_set_desc_set(ddev,&ddev->ssao_pipe, &ssao_kernel, sizeof(vec4) * 32, 1);
    texture_slots[0] = &def_rt.color_attachments[0];
    texture_slots[1] = &def_rt.color_attachments[1];
    texture_slots[2] = &def_rt.depth_attachment;
    texture_slots[3] = &def_rt.depth_attachment;
    dg_set_desc_set(ddev,&ddev->ssao_pipe, texture_slots, 4, 2);
    dg_draw(ddev, 3,0);
    dg_rendering_end(ddev);

///*
    //blur the SSAO
    dg_rendering_begin(ddev, &ssao_tex, 1,NULL, DG_RENDERING_SETTINGS_DEPTH_DISABLE);
    dg_set_viewport(ddev, 0,0,ssao_tex.width, ssao_tex.height);
    dg_set_scissor(ddev, 0,0,ssao_tex.width, ssao_tex.height);
    dg_bind_pipeline(ddev, &ddev->blur_pipe);
    texture_slots[0] =  &ssao_tex;
    dg_set_desc_set(ddev,&ddev->blur_pipe, texture_slots, 1, 2);
    dg_draw(ddev, 3,0);
    dg_rendering_end(ddev);
//*/

    dg_wait_idle(ddev);
    
    mat4 capture_proj = perspective_proj(90.0f, 1, 0.01, 100);
    mat4 capture_views[6] =
    {
        look_at(v3(0,0,0),v3(1,0,0), v3(0,-1,0)),
        look_at(v3(0,0,0),v3(-1,0,0), v3(0,-1,0)),
        look_at(v3(0,0,0),v3(0,-1,0), v3(0,0,-1)),
        look_at(v3(0,0,0),v3(0,1,0), v3(0,0,1)),
        look_at(v3(0,0,0),v3(0,0,1), v3(0,-1,0)),
        look_at(v3(0,0,0),v3(0,0,-1), v3(0,-1,0)),
    };
    dgBuffer buffers[] = {base_pos};
    u64 offsets[] = {0};
    mat4 cube_data[]= {capture_proj, capture_views[0],capture_views[1],
                        capture_views[2],capture_views[3],capture_views[4],capture_views[5]};

    dg_rendering_begin(ddev, &composition_rt.color_attachments[0], 1,NULL, DG_RENDERING_SETTINGS_DEPTH_DISABLE);
    dg_set_viewport(ddev,0,0, composition_rt.color_attachments[0].width, composition_rt.color_attachments[0].height);
    dg_set_scissor(ddev, 0,0, composition_rt.color_attachments[0].width, composition_rt.color_attachments[0].height);
    dg_bind_pipeline(ddev, &ddev->skybox_pipe);
    dg_bind_vertex_buffers(ddev, buffers, offsets, 1);
    dg_bind_index_buffer(ddev, &base_ibo, 0);
    dg_set_desc_set(ddev,&ddev->skybox_pipe, cube_data, sizeof(cube_data), 1);
    texture_slots[0] = &cube_tex;
    dg_set_desc_set(ddev,&ddev->skybox_pipe, texture_slots, 1, 2);
    dg_draw(ddev, 24,base_ibo.size/sizeof(u16));
    dg_rendering_end(ddev);



    
    dg_rendering_begin(ddev, &composition_rt.color_attachments[0], 1,NULL, DG_RENDERING_SETTINGS_DEPTH_DISABLE);
    dg_set_viewport(ddev,0,0, composition_rt.color_attachments[0].width, composition_rt.color_attachments[0].height);
    dg_set_scissor(ddev, 0,0, composition_rt.color_attachments[0].width, composition_rt.color_attachments[0].height);
    dg_bind_pipeline(ddev, &ddev->composition_pipe);
    //FIX:  datafullscreen should be as big as the number of cascades, but im bored
    mat4 data_fullscreen[6] = {lsm[0], lsm[1],lsm[2],lsm[3], (mat4){fdist[0],0,0,0,fdist[1],0,0,0,fdist[2],0,0,0,fdist[3],0,0,0},{light_dir.x,light_dir.y,light_dir.z,0, cam.pos.x,cam.pos.y,cam.pos.z, 1.0} };
    dg_set_desc_set(ddev,&ddev->composition_pipe, &data_fullscreen, sizeof(data_fullscreen), 1);
    texture_slots[0] = &def_rt.color_attachments[0];
    texture_slots[1] = &def_rt.color_attachments[1];
    texture_slots[2] = &def_rt.color_attachments[2];
    texture_slots[3] = &csm_rt.depth_attachment;
    texture_slots[4] = &irradiance_map;
    texture_slots[5] = &prefilter_map;
    texture_slots[6] = &brdfLUT;
    texture_slots[7] = &ssao_tex;
    dg_set_desc_set(ddev,&ddev->composition_pipe, texture_slots, 8, 2);
    dg_draw(ddev, 3,0);
    dg_rendering_end(ddev);

    //draw_model(ddev, &fox,mat4_mul(mat4_translate(v3(10,0,0)), mat4_mul(mat4_mul(mat4_rotate(0,v3(0,-1,0)),mat4_rotate(90, v3(1,0,0))),mat4_scale(v3(0.05,0.05,0.05)))));
        
    //draw the grid ???
    if (ddev->grid_active){
        dg_rendering_begin(ddev, &composition_rt.color_attachments[0], 1, &def_rt.depth_attachment, DG_RENDERING_SETTINGS_NONE);
        dg_set_viewport(ddev,0,0, composition_rt.color_attachments[0].width, composition_rt.color_attachments[0].height);
        dg_set_scissor(ddev, 0,0, composition_rt.color_attachments[0].width, composition_rt.color_attachments[0].height);
        dg_bind_pipeline(ddev, &ddev->grid_pipe);
        //@FIX: why float copy doesn't work due to alignment and we have to copy v4's ?????? (SPIR-V thing)
        vec4 object_data = (vec4){2.0f};
        dg_set_desc_set(ddev,&ddev->grid_pipe, &object_data, sizeof(object_data), 1);
        dg_draw(ddev, 6,0);
        dg_rendering_end(ddev);
    }
    //dui_draw_rect((mu_Rect){100,100,100,100}, (mu_Color){255,0,0,255});

    return TRUE;
}

void dg_frame_end(dgDevice *ddev)
{


    //set desc set 0 again because it might have been redone
    //TODO: should we set this in every drawcall?? and maybe when drawin models we can set it only once
    /*
    mat4 view = dcamera_get_view_matrix(&cam);
    mat4 proj = perspective_proj(60.0f, ddev->swap.extent.width/(f32)ddev->swap.extent.height, 0.01, 100);
    mat4 data[4] = {view, proj, m4d(1.0f),m4d(1.0f)};
    dg_set_desc_set(ddev,&ddev->def_pipe, data, sizeof(data), 0);
    draw_cube(ddev, mat4_translate(v3(2,10,0)));
    */
    

    dg_end_command_buffer(ddev, ddev->command_buffers[ddev->current_frame]);

    VkSubmitInfo si = {};
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
    
    DPROFILER_START("render_submit");
    //vkResetFences(ddev->device, 1, &ddev->in_flight_fences[0]);
    VK_CHECK(vkQueueSubmit(ddev->graphics_queue, 1, &si, ddev->in_flight_fences[ddev->current_frame]));
    DPROFILER_END();

    VkPresentInfoKHR present_info = {};
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
        dlog(NULL, C_TEXT("Failed to present swapchain image!\n"));

    ddev->current_frame = (ddev->current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

}
void dg_device_init(void)
{
    dd.instance = dvk_instance_create();
    dd.surface = dvk_surface_create(dd.instance, &main_window);
    dd.physical_device = dvk_physical_device_pick(dd.instance, dd.surface);
	dd.device = dvk_logical_device_create(dd.physical_device, dd.surface, &dd.graphics_queue, &dd.present_queue);
    dd.command_pool = dvk_command_pool_create(dd.device, dd.physical_device, dd.surface);
    assert(dg_create_swapchain(&dd));
    assert(dg_create_swapchain_image_views(&dd));



    glslang_initialize_process();

    dg_descriptor_set_layout_cache_init(&dd.desc_layout_cache); //the cache needs to be ready before pipeline creation
    assert(dg_create_pipeline(&dd, &dd.def_pipe,C_TEXT("def.vert"), C_TEXT("def.frag"), DG_PIPE_OPTION_PACK_VERTEX_ATTRIBS));
    assert(dg_create_pipeline(&dd, &dd.pbr_def_pipe,C_TEXT("pbr_def.vert"), C_TEXT("pbr_def.frag"), DG_PIPE_OPTION_NONE));
    assert(dg_create_pipeline(&dd, &dd.shadow_pipe,C_TEXT("sm.vert"), C_TEXT("sm.frag"), DG_PIPE_OPTION_PACK_VERTEX_ATTRIBS));
    assert(dg_create_pipeline(&dd, &dd.pbr_shadow_pipe,C_TEXT("sm.vert"), C_TEXT("sm.frag"), DG_PIPE_OPTION_NONE));
    assert(dg_create_pipeline(&dd, &dd.grid_pipe,C_TEXT("grid.vert"), C_TEXT("grid.frag"), DG_PIPE_OPTION_PACK_VERTEX_ATTRIBS | DG_PIPE_OPTION_BLEND));
    assert(dg_create_pipeline(&dd, &dd.ssao_pipe,C_TEXT("ssao.vert"), C_TEXT("ssao.frag"), DG_PIPE_OPTION_PACK_VERTEX_ATTRIBS));
    assert(dg_create_pipeline(&dd, &dd.blur_pipe,C_TEXT("blur.vert"), C_TEXT("blur.frag"), DG_PIPE_OPTION_PACK_VERTEX_ATTRIBS));
    assert(dg_create_pipeline(&dd, &dd.cubemap_conv_pipe,C_TEXT("cubemap_conv.vert"), C_TEXT("cubemap_conv.frag"), DG_PIPE_OPTION_PACK_VERTEX_ATTRIBS));
    assert(dg_create_pipeline(&dd, &dd.prefilter_map_pipe,C_TEXT("prefilter_map.vert"), C_TEXT("prefilter_map.frag"), DG_PIPE_OPTION_PACK_VERTEX_ATTRIBS));
    assert(dg_create_pipeline(&dd, &dd.skybox_pipe,C_TEXT("skybox.vert"), C_TEXT("skybox.frag"), DG_PIPE_OPTION_PACK_VERTEX_ATTRIBS));
    assert(dg_create_pipeline(&dd, &dd.brdf_lut_pipe,C_TEXT("brdf.vert"), C_TEXT("brdf.frag"), DG_PIPE_OPTION_NONE));
    assert(dg_create_pipeline(&dd, &dd.skybox_gen_pipe,C_TEXT("skybox_gen.vert"), C_TEXT("skybox_gen.frag"), DG_PIPE_OPTION_PACK_VERTEX_ATTRIBS));
    //assert(dg_create_pipeline(&dd, &dd.fullscreen_pipe,C_TEXT("fullscreen.vert", "fullscreen.frag", TRUE));
    assert(dg_create_pipeline(&dd, &dd.base_pipe,C_TEXT("base.vert"), C_TEXT("base.frag"), DG_PIPE_OPTION_PACK_VERTEX_ATTRIBS));
    assert(dg_create_pipeline(&dd, &dd.particle_pipe,C_TEXT("particle.vert"), C_TEXT("particle.frag"), DG_PIPE_OPTION_NONE));
    assert(dg_create_pipeline(&dd, &dd.anim_pipe,C_TEXT("anim.vert"), C_TEXT("anim.frag"), DG_PIPE_OPTION_NONE));
    assert(dg_create_pipeline(&dd, &dd.composition_pipe,C_TEXT("composition.vert"), C_TEXT("composition.frag"), DG_PIPE_OPTION_PACK_VERTEX_ATTRIBS));
    //assert(dg_create_pipeline(&dd, &dd.dui_pipe,C_TEXT("dui.vert"), C_TEXT("dui.frag"), DG_PIPE_OPTION_NONE));
    dd.command_buffers = dvk_command_buffers_create(dd.device, dd.command_pool, dd.swap.image_count);

    assert(dg_create_sync_objects(&dd));

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        dg_descriptor_allocator_init(&dd, &dd.desc_alloc[i]);


    dg_ubo_data_buffer_init(&dd, &dd.ubo_buf, sizeof(mat4)*2000);
    dd.shadow_pass_active = FALSE;
    dd.grid_active = FALSE;
	dlog(NULL, C_TEXT("Vulkan initialized correctly!\n"));

}

b32 dgfx_init(void)
{
	dg_device_init();

    u32 sector_count = 32;
    u32 stack_count = 32;
    dgVertex *sphere_verts = sphere_build_verts(1.0, sector_count, stack_count);
    u16 *sphere_indices = sphere_build_index(sector_count, stack_count);
    dg_create_buffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
	(VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
	&sphere_vbo, sizeof(dgVertex) * (sector_count + 1) * (stack_count +1), sphere_verts);
    dg_create_buffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
	(VkMemoryPropertyFlagBits)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT), 
	&sphere_ibo, sizeof(sphere_indices[0]) * stack_count * sector_count * 6, sphere_indices);

    dfree(sphere_verts);
    dfree(sphere_indices);


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
    dg_rt_init(&dd, &composition_rt, 1, TRUE, dd.swap.extent.width, dd.swap.extent.height);
    dg_rt_init_csm(&dd, &csm_rt, 3, 1024,1024);

    vec4 noise_data[32];
    for (u32 i = 0; i < 32; ++i){
        noise_data[i] = v4(r01() * 2 - 1, r01() * 2 - 1, r01() * 2 - 1, r01() * 2 - 1);
    }
    hdr_map = dg_create_texture_image(&dd, C_TEXT("../assets/newport_loft.hdr"), DG_IMAGE_FORMAT_RGBA32_SFLOAT);
    cube_tex = dg_create_texture_image_wdata(&dd, NULL, 1024,1024, DG_IMAGE_FORMAT_RGBA32_SFLOAT, 6,1);
    ssao_tex = dg_create_texture_image_wdata(&dd, NULL, 1024,1024, DG_IMAGE_FORMAT_RGBA16_SFLOAT, 1,1);
    prefilter_map = dg_create_texture_image_wdata(&dd, NULL, 1024,1024, DG_IMAGE_FORMAT_RGBA32_SFLOAT, 6,4);
    irradiance_map = dg_create_texture_image_wdata(&dd, NULL, 64,64, DG_IMAGE_FORMAT_RGBA32_SFLOAT, 6,1);
    //noise_tex = dg_create_texture_image_wdata(&dd,(float*)noise_data, 4,4,DG_IMAGE_FORMAT_RGBA16_SFLOAT, 1,1);//dg_create_texture_image(&dd, "../assets/noise.png", DG_IMAGE_FORMAT_RGBA8_SRGB); //TODO, we should auto generate dis
    //noise_tex = dg_create_texture_image(&dd, "../assets/noise.png", DG_IMAGE_FORMAT_RGBA8_UNORM); //TODO, we should auto generate dis
    brdfLUT = dg_create_texture_image_wdata(&dd, NULL, 128, 128, DG_IMAGE_FORMAT_RGBA16_SFLOAT, 1, 1);
    water_bottle = dmodel_load_gltf(C_TEXT("DamagedHelmet"));
    //fox = dmodel_load_gltf(C_TEXT("untitled"));
    return 1;
}