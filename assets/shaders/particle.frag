#version 450

layout(location = 0) in vec2 f_tex_coord;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform  GlobalBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} GlobalData;

layout(set = 1, binding = 0) uniform  ObjectBuffer{
	mat4 model;
	vec4 color;
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D base_color_tex;

void main() {
	vec4 color = ObjectData.color;
	out_color = color;
}
