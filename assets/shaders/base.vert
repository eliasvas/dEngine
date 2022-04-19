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
	mat4 obj1;
	mat4 obj2;
} ObjectData;


layout(location = 0) out vec3 f_color;

void main() {
    gl_Position = vec4(vertex_pos  * GlobalData.view[0][0], 1.0);
    gl_Position.z = 0.799;
    gl_Position.x += GlobalData.view[0][1];
    f_color.x = GlobalData.view[0][1];
}
