#version 450

layout(location = 0) in vec3 f_frag_pos;
layout(location = 1) in vec3 f_normal;
layout(location = 2) in vec2 f_tex_coord;

layout(location = 0) out vec4 g_pos;
layout(location = 1) out vec4 g_normal;
layout(location = 2) out vec4 g_albedo_spec;

layout(set = 0, binding = 0) uniform  GlobalBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} GlobalData;

layout(set = 1, binding = 0) uniform  ObjectBuffer{
	mat4 model;
	vec3 color;
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D base_color_tex;
layout(set = 2, binding = 1) uniform sampler2D orm_tex;
layout(set = 2, binding = 2) uniform sampler2D normal_tex;
layout(set = 2, binding = 3) uniform sampler2D emissive_tex;

vec3 dir_light = vec3(-1,1,0.2);

void main() {
	g_albedo_spec = texture(base_color_tex, f_tex_coord.xy);
	g_normal = vec4(f_normal.xyz,1.0);
	g_pos = vec4(f_frag_pos.xyz,1.0);



}
