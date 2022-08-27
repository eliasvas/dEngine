

extern "C" {
    #include "deditor.h"
	#include "volk/volk.h"
	#include "dinput.h"
}
typedef struct dInputState dInputState;
#include "dgfx.h"
#include "dconfig.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/imconfig.h"

extern dgDevice dd;
extern dConfig engine_config;
extern dWindow main_window;
extern dInputState dis;
dEditor main_editor;

VkRenderPass imgui_rp;

void dui_init(void)
{
    //1: create descriptor pool for IMGUI
	// the size of the pool is very oversize, but it's copied from imgui demo itself.
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	VkDescriptorPool imguiPool;
	vkCreateDescriptorPool(dd.device, &pool_info, NULL, &imguiPool);


	// 2: initialize imgui library

	//this initializes the core structures of imgui
	ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;       // Enable Mouse Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;
	ImGui::SetNextFrameWantCaptureMouse(true);
    ImGui::StyleColorsDark();

	//this initializes imgui for GLFW
    ImGui_ImplGlfw_InitForVulkan(main_window.gwindow,0);

	//this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = dd.instance;
	init_info.PhysicalDevice = dd.physical_device;
	init_info.Device = dd.device;
	init_info.Queue = dd.graphics_queue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;


    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = dd.swap.image_format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(dd.device, &renderPassInfo, NULL, &imgui_rp) != VK_SUCCESS) {
        printf("failed to create render pass!");
    }

	ImGui_ImplVulkan_Init(&init_info, imgui_rp);
    
    VkCommandBuffer cmd = dg_begin_single_time_commands(&dd);
    ImGui_ImplVulkan_CreateFontsTexture(cmd);
    dg_end_single_time_commands(&dd, cmd);

	//clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}



void deditor_init(dEditor *editor){
	if (editor == NULL)editor = &main_editor;
	dui_init();
	ImGuiIO& io = ImGui::GetIO();
}
void deditor_update(dEditor *editor, float dt){
	ImGui::SetNextFrameWantCaptureMouse(true);
	ImGuiIO& io = ImGui::GetIO();
	io.AddMouseButtonEvent(0,dkey_down(DK_LMB));
}

void deditor_draw(dEditor *editor){
	if (editor == NULL)editor = &main_editor;
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();


    //imgui commands
    ImGui::ShowDemoWindow();
	ImGui::Render();

	dg_rendering_begin(&dd, NULL, 1, NULL, DG_RENDERING_SETTINGS_DEPTH_DISABLE);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), dd.command_buffers[dd.current_frame]);
	dg_rendering_end(&dd);
}