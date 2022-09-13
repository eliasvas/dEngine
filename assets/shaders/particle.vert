#version 450


layout(location = 0) in vec3 vertex_pos;
layout(location = 1) in vec2 tex_coord;

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

layout(location = 0) out vec2 f_tex_coord;

void main() {
    gl_Position = GlobalData.proj * GlobalData.view * ObjectData.model * vec4(vertex_pos, 1.0);
    f_tex_coord = tex_coord;
    
}
