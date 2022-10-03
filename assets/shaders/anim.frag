#version 450

layout(location = 0) in vec3 f_frag_pos;
layout(location = 1) in vec3 f_normal;
layout(location = 2) in vec2 f_tex_coord;
layout(location = 3) in vec4 f_color;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform  GlobalBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} GlobalData;

layout(set = 1, binding = 0) uniform  ObjectBuffer{
	mat4 model;
	mat4 joint_mat[31];
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D base_color_tex;
layout(set = 2, binding = 1) uniform sampler2D orm_tex;
layout(set = 2, binding = 2) uniform sampler2D normal_tex;
layout(set = 2, binding = 3) uniform sampler2D emissive_tex;

vec3 light_dir = vec3(0,0.9,0.3);

void main() {
	vec3 normal = texture(normal_tex, f_tex_coord).xyz;
	vec3 base_color = texture(base_color_tex, f_tex_coord).xyz;
	vec3 emissive = texture(emissive_tex, f_tex_coord).xyz;
	vec3 orm = texture(orm_tex, f_tex_coord).xyz;
	float occlusion = orm.x;
	float roughness = orm.y;
	float metallic = orm.z;



	vec3 light_color = vec3(0.9,0.9,0.9);	

	out_color= f_color* max(dot(normalize(light_dir), normalize(-f_normal)),0.0)* 0.8 + f_color * 0.2;
}
