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
	vec4 col[2];
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D tex_sampler1;

vec3 dir_light = vec3(-1,1,0.2);

void main() {
	//g_albedo_spec = texture(tex_sampler1, f_tex_coord.xy);
	vec2 size = vec2(4,4);
	float total = floor(f_tex_coord.x*float(size.x)) +
                  floor(f_tex_coord.y*float(size.y));
    bool is_even = mod(total,2.0)==0.0;
	vec4 color = (is_even) ? ObjectData.col[0] : ObjectData.col[1];
	g_albedo_spec = color;
	g_albedo_spec.a = 0.0;
	g_normal = vec4(f_normal.xyz,0.0);
	g_pos = vec4(f_frag_pos.xyz,0.0);
}
