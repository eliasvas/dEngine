#version 450

layout(location = 0) in vec4 f_color;

layout(location = 1) in vec2 f_tex_coord;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform  GlobalBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} GlobalData;

layout(set = 1, binding = 0) uniform  ObjectBuffer{
	uint window_width;
	uint window_height;
} ObjectData;

layout(set = 2, binding = 0) uniform usampler2D tex_sampler1;

void main() {
	vec4 col = texture(tex_sampler1, f_tex_coord);
	out_color = f_color;
	if (col.x < 0.1)discard;
	//out_color = vec4(1,0,0,1);
}
