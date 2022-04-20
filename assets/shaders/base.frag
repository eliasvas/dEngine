#version 450

layout(location = 0) in vec3 f_color;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform  GlobalBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} GlobalData;

layout(set = 1, binding = 0) uniform  ObjectBuffer{
	float color;
} ObjectData;
vec3 dir_light = vec3(-1,1,0.2);

void main() {
	out_color = vec4(f_color.x, f_color.y, f_color.z, 1.0);
}
