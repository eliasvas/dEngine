#version 450


layout (location = 0) in vec3 f_color;
layout (location = 1) in vec2 f_tex_coord;

layout (location = 0) out vec4 frag_color;
//layout (location = 1) out vec4 frag_neg_color;

layout(set = 0, binding = 0) uniform  GlobalBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} GlobalData;

layout(set = 1, binding = 0) uniform  ObjectBuffer{
	vec4 color;
	float modifier;
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D tex_sampler1;
layout(set = 2, binding = 1) uniform sampler2D tex_sampler2;

void main() {
    frag_color = vec4(f_color,1.0);
    frag_color = (1.0 - ObjectData.modifier) * texture(tex_sampler1, f_tex_coord) + ObjectData.modifier * ObjectData.color;
}
