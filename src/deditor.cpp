
#include "stb/stb_ds.h"
extern "C" {
    #include "deditor.h"
	#include "volk/volk.h"
	#include "dinput.h"
	
	#include "dprofiler.h"
	#include "dparticle.h"
	#include "dentity.h"
}
typedef struct dInputState dInputState;
#include "dgfx.h"
#include "dconfig.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/imconfig.h"

extern f64 dt;//in seconds
extern dgRT composition_rt;
extern dgRT def_rt;
VkDescriptorSet def_desc_sets[DG_MAX_COLOR_ATTACHMENTS];
VkDescriptorSet comp_desc_set;
extern dgDevice dd;
extern dConfig engine_config;
extern dWindow main_window;
extern dInputState dis;
extern dProfiler global_profiler;
extern  mat4 view, proj;


//these are just for test, remove after end TODO TODO
extern dParticleEmitter test_emitter;
extern dParticleEmitterCM particle_emitter_cm;
extern dTransformCM transform_manager;


dEntity e0,e1,e2;

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
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
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
static dEditorStyle deditor_style_default(void)
{
	dEditorStyle style{};
	style.bg_col           = {27/255.0f, 27/255.0f, 28/255.0f,1};
	style.light_bg_col     = {82/255.0f, 82/255.0f, 85/255.0f,1};
	style.very_light_bg_col= {100/255.0f, 100/255.0f, 95/255.0f,1};
	style.title_col        = {7/255.0f, 7/255.0f, 8/255.0f,1};

	style.panel_col        = {11/255.0f, 11/255.0f, 15/255.0f,1};
	style.panel_hover_col  = {29/255.0f, 131/255.0f, 236/255.0f,1};
	style.panel_active_col = {10/255.0f, 119/255.0f, 200/255.0f,1};

	style.text_col         = {255/255.0f, 255/255.0f, 255/255.0f,1};
	style.text_disabled_col= {151/255.0f, 151/255.0f, 151/255.0f,1};
	style.border_col       = {78/255.0f, 78/255.0f, 78/255.0f,1};
	return style;
}

ImVec4 to_imv4(vec4 v)
{
	return ImVec4(v.x,v.y,v.z,v.w);
}
void deditor_update_style(dEditor *editor){
	auto& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	colors[ImGuiCol_Text]                 = to_imv4(editor->style.text_col);
	colors[ImGuiCol_TextDisabled]         = to_imv4(editor->style.text_disabled_col);
	colors[ImGuiCol_TextSelectedBg]       = to_imv4(editor->style.panel_active_col);
	colors[ImGuiCol_WindowBg]             = to_imv4(editor->style.bg_col);
	colors[ImGuiCol_ChildBg]              = to_imv4(editor->style.bg_col);
	colors[ImGuiCol_PopupBg]              = to_imv4(editor->style.bg_col);
	colors[ImGuiCol_Border]               = to_imv4(editor->style.border_col);
	colors[ImGuiCol_BorderShadow]         = to_imv4(editor->style.border_col);
	colors[ImGuiCol_FrameBg]              = to_imv4(editor->style.panel_col);
	colors[ImGuiCol_FrameBgHovered]       = to_imv4(editor->style.panel_hover_col);
	colors[ImGuiCol_FrameBgActive]        = to_imv4(editor->style.panel_active_col);
	colors[ImGuiCol_TitleBg]              = to_imv4(editor->style.title_col);
	colors[ImGuiCol_TitleBgActive]        = to_imv4(editor->style.title_col);
	colors[ImGuiCol_TitleBgCollapsed]     = to_imv4(editor->style.title_col);
	colors[ImGuiCol_MenuBarBg]            = to_imv4(editor->style.panel_col);
	colors[ImGuiCol_ScrollbarBg]          = to_imv4(editor->style.panel_col);
	colors[ImGuiCol_ScrollbarGrab]        = to_imv4(editor->style.light_bg_col);
	colors[ImGuiCol_ScrollbarGrabHovered] = to_imv4(editor->style.very_light_bg_col);
	colors[ImGuiCol_ScrollbarGrabActive]  = to_imv4(editor->style.very_light_bg_col);
	colors[ImGuiCol_CheckMark]            = to_imv4(editor->style.panel_active_col);
	colors[ImGuiCol_SliderGrab]           = to_imv4(editor->style.panel_hover_col);
	colors[ImGuiCol_SliderGrabActive]     = to_imv4(editor->style.panel_active_col);
	colors[ImGuiCol_Button]               = to_imv4(editor->style.panel_col);
	colors[ImGuiCol_ButtonHovered]        = to_imv4(editor->style.panel_hover_col);
	colors[ImGuiCol_ButtonActive]         = to_imv4(editor->style.panel_hover_col);
	colors[ImGuiCol_Header]               = to_imv4(editor->style.panel_col);
	colors[ImGuiCol_HeaderHovered]        = to_imv4(editor->style.panel_hover_col);
	colors[ImGuiCol_HeaderActive]         = to_imv4(editor->style.panel_active_col);
	colors[ImGuiCol_Separator]            = to_imv4(editor->style.border_col);
	colors[ImGuiCol_SeparatorHovered]     = to_imv4(editor->style.border_col);
	colors[ImGuiCol_SeparatorActive]      = to_imv4(editor->style.border_col);
	colors[ImGuiCol_ResizeGrip]           = to_imv4(editor->style.bg_col);
	colors[ImGuiCol_ResizeGripHovered]    = to_imv4(editor->style.panel_col);
	colors[ImGuiCol_ResizeGripActive]     = to_imv4(editor->style.light_bg_col);
	colors[ImGuiCol_PlotLines]            = to_imv4(editor->style.panel_active_col);
	colors[ImGuiCol_PlotLinesHovered]     = to_imv4(editor->style.panel_hover_col);
	colors[ImGuiCol_PlotHistogram]        = to_imv4(editor->style.panel_active_col);
	colors[ImGuiCol_PlotHistogramHovered] = to_imv4(editor->style.panel_hover_col);
	colors[ImGuiCol_DragDropTarget]       = to_imv4(editor->style.bg_col);
	colors[ImGuiCol_NavHighlight]         = to_imv4(editor->style.bg_col);
	colors[ImGuiCol_DockingPreview]       = to_imv4(editor->style.panel_active_col);
	colors[ImGuiCol_Tab]                  = to_imv4(editor->style.bg_col);
	colors[ImGuiCol_TabActive]            = to_imv4(editor->style.panel_active_col);
	colors[ImGuiCol_TabUnfocused]         = to_imv4(editor->style.bg_col);
	colors[ImGuiCol_TabUnfocusedActive]   = to_imv4(editor->style.panel_active_col);
	colors[ImGuiCol_TabHovered]           = to_imv4(editor->style.panel_hover_col);

	style.WindowRounding    = 0.0f;
	style.ChildRounding     = 0.0f;
	style.FrameRounding     = 0.0f;
	style.GrabRounding      = 0.0f;
	style.PopupRounding     = 0.0f;
	style.ScrollbarRounding = 0.0f;
	style.TabRounding       = 0.0f;
}
void deditor_init(dEditor *editor){
	if (editor == NULL)editor = &main_editor;
	dui_init();
	editor->style= deditor_style_default();
	editor->viewport = v4(0,0,dd.swap.extent.width, dd.swap.extent.height);
	editor->editor_open = TRUE;//(DEBUG_BUILD) ? 1 : 0;
	deditor_update_style(editor);
	ImGuiIO& io = ImGui::GetIO();
	for (u32 i = 0; i < 3; ++i)
		def_desc_sets[i] = ImGui_ImplVulkan_AddTexture(def_rt.color_attachments[i].sampler, def_rt.color_attachments[i].view, def_rt.color_attachments[i].image_layout);
	comp_desc_set = ImGui_ImplVulkan_AddTexture(composition_rt.color_attachments[0].sampler, composition_rt.color_attachments[0].view, composition_rt.color_attachments[0].image_layout);

	{
		e0 = dentity_create();
		//u32 ci = dparticle_emitter_cm_add(NULL, e0);
		u32 component_index = dtransform_cm_add(&transform_manager, e0, (dEntity){DENTITY_NOT_FOUND});
		dTransform parent_t = dtransform_default();
		parent_t.trans = v3(3,0,0);
		dtransform_cm_set_local(&transform_manager, component_index, parent_t);

		u32 name_index = ddebug_name_cm_add(NULL, e0);
		char *name = ddebug_name_cm_name(NULL, name_index);
		sprintf(name, "e0");
	}

	{
		e1 = dentity_create();
		//u32 ci = dparticle_emitter_cm_add(NULL, e1);
		u32 component_index = dtransform_cm_add(&transform_manager, e1, e0);
		dTransform parent_t = dtransform_default();
		parent_t.trans = v3(5,0,0);
		dtransform_cm_set_local(&transform_manager, component_index, parent_t);

		u32 name_index = ddebug_name_cm_add(NULL, e1);
		char *name = ddebug_name_cm_name(NULL, name_index);
		sprintf(name, "e1");
	}


	{
		e2 = dentity_create();
		//u32 ci = dparticle_emitter_cm_add(NULL, e2);
		u32 component_index = dtransform_cm_add(&transform_manager, e2, e0);
		dTransform parent_t = dtransform_default();
		parent_t.trans = v3(4,3,0);
		dtransform_cm_set_local(&transform_manager, component_index, parent_t);

		u32 name_index = ddebug_name_cm_add(NULL, e2);
		char *name = ddebug_name_cm_name(NULL, name_index);
		sprintf(name, "e2");
	}

}
void deditor_update(dEditor *editor, float dt){
	if (editor == NULL)editor = &main_editor;
	//TODO resizing the swapchain makes the image descriptor sets invalid and crash
	//one fix would be to make them each frame, but that way the descriptor pool overflows,
	//look a bit into vulkan descriptors AND the backend.
	/*
	ImGui_ImplVulkan_ResetDescriptorPool();
	for (u32 i = 0; i < 3; ++i)
		def_desc_sets[i] = ImGui_ImplVulkan_AddTexture(def_rt.color_attachments[i].sampler, def_rt.color_attachments[i].view, def_rt.color_attachments[i].image_layout);
	comp_desc_set = ImGui_ImplVulkan_AddTexture(composition_rt.color_attachments[0].sampler, composition_rt.color_attachments[0].view, composition_rt.color_attachments[0].image_layout);
	*/
	//deditor_update_style(editor);
	ImGui::SetNextFrameWantCaptureMouse(true);
	ImGuiIO& io = ImGui::GetIO();
	io.AddMouseButtonEvent(0,dkey_down(DK_LMB));
	io.AddMouseButtonEvent(1,dkey_down(DK_RMB));
	io.AddMouseButtonEvent(2,dkey_down(DK_MMB));
}





//We shouldnt search EACH frame for the description and make the UI, TODO fix
b32 deditor_component_view(void *data, dComponentDesc desc){
	b32 edited=0;
	for (u32 i = 0; i < desc.field_count; ++i){
		dComponentField *field = &desc.fields_buf[i];
		switch (field->type){
			case DCOMPONENT_FIELD_TYPE_U32:
				if (ImGui::SliderInt(field->name, (s32*)(data + field->offset), 0, 100, NULL, 0))edited =  TRUE;
				break;
			case DCOMPONENT_FIELD_TYPE_F32:
				if (ImGui::SliderFloat(field->name, (f32*)(data + field->offset), 0, 100, NULL, 0))edited =  TRUE;
				break;
			case DCOMPONENT_FIELD_TYPE_VEC2:
				if (ImGui::SliderFloat2(field->name, (f32*)(data + field->offset), 0, 100, NULL, 0))edited =  TRUE;
				break;
			case DCOMPONENT_FIELD_TYPE_VEC3:
				if (ImGui::SliderFloat3(field->name, (f32*)(data + field->offset), 0, 100, NULL, 0))edited =  TRUE;
				break;
			case DCOMPONENT_FIELD_TYPE_VEC4:
				if (ImGui::SliderFloat4(field->name, (f32*)(data + field->offset), 0, 100, NULL, 0))edited =  TRUE;
				break;
			default:
				break;
		}
	}
	return edited;
}

void deditor_entity_view(dEntity e){
	//[1]. we display our entity, 
	char *name = ddebug_name_cm_name(NULL,ddebug_name_cm_lookup(NULL, e));
	u32 transform_index =  dtransform_cm_lookup(NULL, e); //index of transform component
	dTransform *local_transform= dtransform_cm_local(NULL,transform_index);
	if (ImGui::TreeNode(name)){
		if (ImGui::CollapsingHeader("Transform")){
		if (deditor_component_view(local_transform, transform_manager.component_desc))
			dtransform_cm_set_local(NULL, transform_index, *local_transform);
		}
		//[2]. then we search for a child, and if we have one, we display that INSIDE our TreeNode
		u32 child_index = transform_manager.data.first_child[transform_index];
		if (child_index != DCOMPONENT_NOT_FOUND)
			deditor_entity_view(transform_manager.data.entity[child_index]);

		ImGui::TreePop();
	}
	//[3]. then we see if there is another sibling, and if so, we display it 
	//(and the sibling will search for another one in its own right)
	u32 next_sibling_index = transform_manager.data.next_sibling[transform_index];
		if (next_sibling_index != DCOMPONENT_NOT_FOUND)
			deditor_entity_view(transform_manager.data.entity[next_sibling_index]);
}


void deditor_scene_view(dEntity root){
	ImGui::Begin("Scene", NULL, ImGuiWindowFlags_None);

	deditor_entity_view(root);

	ImGui::End();
}

void deditor_draw(dEditor *editor){
	if (editor == NULL)editor = &main_editor;
	
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Viewport",0,ImGuiWindowFlags_NoMove);
	static float sf = 0.4f;
	ImVec2 win_max, win_min;
	ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
	
	{
		ImVec2 vMin = ImGui::GetWindowContentRegionMin();
		ImVec2 vMax = ImGui::GetWindowContentRegionMax();
		win_max = ImGui::GetWindowSize();
		win_min = ImGui::GetWindowPos();

		vMin.x += ImGui::GetWindowPos().x;
		vMin.y += ImGui::GetWindowPos().y;
		vMax.x += ImGui::GetWindowPos().x;
		vMax.y += ImGui::GetWindowPos().y;
		editor->viewport = v4(vMin.x, vMin.y, vMax.x, vMax.y);
		
		ImGui::GetForegroundDrawList()->AddRect( vMin, vMax, IM_COL32( 255, 255, 0, 255 ) );
		ImGui::Image(comp_desc_set, ImVec2(editor->viewport.z - editor->viewport.x, editor->viewport.w - editor->viewport.y));
	}
	ImGui::SetNextWindowPos(ImVec2(win_max.x, win_max.y),0);
	ImGui::SetNextWindowSize(ImVec2( dd.swap.extent.width-win_max.x, dd.swap.extent.height-win_max.y),0);
	ImGui::End();

	ImGui::Begin("Settings", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
	ImGui::Checkbox("Shadows", (bool*)&dd.shadow_pass_active);
	ImGui::Checkbox("Grid", (bool*)&dd.grid_active);
	static bool anti_aliasing = false; //TODO add to dgDevice
	ImGui::Checkbox("AA", (bool*)&anti_aliasing);
	ImGui::ColorButton("Format", {1,0.2,0.2,1});
	static vec3 color_pick = v3(0.7,0.8,0.3);
	//ImGui::ColorPicker3("Color Picker", (float*)&color_pick);
	ImGui::SetNextWindowPos(ImVec2(0, win_max.y),0);
	ImGui::SetNextWindowSize(ImVec2(win_max.x, dd.swap.extent.height - win_max.y),0);
	ImGui::End();

	ImGui::Begin("Profiler", NULL, ImGuiWindowFlags_NoResize| ImGuiWindowFlags_NoMove);
	ImVec2 vMin = ImGui::GetWindowContentRegionMin();
	ImVec2 vMax = ImGui::GetWindowContentRegionMax();
	win_max = ImGui::GetWindowSize();
	win_min = ImGui::GetWindowPos();

	vMin.x += ImGui::GetWindowPos().x;
	vMin.y += ImGui::GetWindowPos().y;
	vMax.x += ImGui::GetWindowPos().x;
	vMax.y += ImGui::GetWindowPos().y;
	
	ImGui::GetForegroundDrawList()->AddRect( vMin, vMax, IM_COL32( 255, 0, 0, 255 ) );
	/*
	u64 name_hash= hash_str("UPDATE");
	u64 sample_index = hmget(global_profiler.name_hash, name_hash); //-1 for some reason
	if (sample_index != HASH_NOT_FOUND&& 0){ //this is for rendering the UPDATE profiler tag
		dProfilerTag *tag = &global_profiler.tags[sample_index];
		f32 ms_max = 0.f;
		f32 ms_min = FLT_MAX;
		for (u32 i = 0; i < MAX_SAMPLES_PER_NAME; ++i) {
			if (tag->samples[i] > ms_max)ms_max = tag->samples[i];
			if (tag->samples[i] < ms_min)ms_min = tag->samples[i];
		}
		for (u32 i = 0; i < MAX_SAMPLES_PER_NAME; ++i) {
			u32 index= (i +tag->next_sample_to_write) % MAX_SAMPLES_PER_NAME;
			f32 factor = tag->samples[index]/ms_max;
			vec4 col = {1,factor,1,1};
			ImVec2 tl = {vMin.x, vMin.y + ((float)(vMax.y - vMin.y)/MAX_SAMPLES_PER_NAME)*i};
			ImVec2 br = ImVec2(vMax.x * factor*factor*factor,tl.y +10);
			//ImGui::GetForegroundDrawList()->AddRect( top_left, bot_right, IM_COL32( 255, 255*factor, 255, 255 ) );
			static ImU32 some_colors[] = {IM_COL32( 255, 0,0,255 ),IM_COL32( 63, 255,0,255 ),IM_COL32( 0, 122, 254,255 )};
			ImGui::GetForegroundDrawList()->AddQuadFilled({tl.x,tl.y},{tl.x, br.y-4},{br.x, br.y-4},{br.x,tl.y},some_colors[i % array_count(some_colors)]);
		}
	}
	*/
	for (u32 i = 0; i < global_profiler.tag_count; ++i){
		//ImGui::Button(global_profiler.tags[i].name);
		//printf("%f\n", global_profiler.tags[i].samples[0]);
		static f32 fv = 13.0f;
		ImGui::SliderFloat(global_profiler.tags[i].name, &global_profiler.tags[i].samples[global_profiler.tags[i].next_sample_to_write % MAX_SAMPLES_PER_NAME], 0.0f, 16.6, NULL, ImGuiSliderFlags_NoInput);
	}
	
	ImGui::SetNextWindowPos(ImVec2(win_max.x, 0),0);
	ImGui::SetNextWindowSize(ImVec2( dd.swap.extent.width-win_max.x, dd.swap.extent.height - win_max.y),0);
	ImGui::End();

	deditor_scene_view(e0);

	//draw e0
	u32 transform_index = dtransform_cm_lookup(&transform_manager, e0);
	dTransform *wt = dtransform_cm_world(&transform_manager, transform_index);
	draw_cube(&dd, dtransform_to_mat4(*wt));

	//draw e1
	transform_index = dtransform_cm_lookup(&transform_manager, e1);
	wt = dtransform_cm_world(&transform_manager, transform_index);
	draw_cube(&dd, dtransform_to_mat4(*wt));
	
	//draw e2
	transform_index = dtransform_cm_lookup(&transform_manager, e2);
	wt = dtransform_cm_world(&transform_manager, transform_index);
	draw_cube(&dd, dtransform_to_mat4(*wt));
	



	ImGui::Render();

	dg_rendering_begin(&dd, NULL, 1, NULL, DG_RENDERING_SETTINGS_DEPTH_DISABLE);
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), dd.command_buffers[dd.current_frame]);
	dg_rendering_end(&dd);


	//set desc set 0
    //mat4 data[4] = {view, proj, m4d(1.0f),m4d(1.0f)};
    //dg_set_desc_set(&dd,&dd.def_pipe, data, sizeof(data), 0);
}