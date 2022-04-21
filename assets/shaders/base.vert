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
	vec3 color;
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D tex_sampler1;


layout(location = 0) out vec3 f_color;
layout(location = 1) out vec2 f_tex_coord;
void main() {
    gl_Position = vec4(vertex_pos  *ObjectData.color.x, 1.0);
    gl_Position.z = 0.799;
    gl_Position.x += GlobalData.view[0][1];
    f_color = ObjectData.color;
    f_tex_coord = tex_coord;
}
