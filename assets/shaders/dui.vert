#version 450

layout(location = 0) in vec3 vertex_pos;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in vec2 tex_coord;

layout(set = 0, binding = 0) uniform  GlobalBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} GlobalData;

layout(set = 1, binding = 0) uniform  ObjectBuffer{
	float window_width;
	float window_height;
} ObjectData;

layout(set = 2, binding = 0) uniform usampler2D tex_sampler1;

layout(location = 0) out vec4 f_color;
layout(location = 1) out vec2 f_tex_coord;

void main() {
    gl_Position = vec4((vertex_pos.x / ObjectData.window_width) * 2 - 1, (vertex_pos.y / ObjectData.window_height) * 2 - 1, 0, 1);
    f_color= vec4(vertex_normal.x, vertex_normal.y, vertex_normal.z, vertex_pos.z);
    f_tex_coord = tex_coord;
}
