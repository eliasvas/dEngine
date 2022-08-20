#version 450

#extension GL_EXT_multiview : enable

layout(location = 0) in vec3 vertex_pos;

layout(set = 0, binding = 0) uniform  GlobalBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} GlobalData;

layout(set = 1, binding = 0) uniform  ObjectBuffer{
	mat4 p;
    mat4 v[6];
	vec4 rough;//we just need the .r
} ObjectData;

layout(set = 2, binding = 0) uniform samplerCube tex_sampler1;


layout(location = 0) out vec3 f_local_pos;

void main() {
    f_local_pos = vertex_pos;
    gl_Position = ObjectData.p * ObjectData.v[gl_ViewIndex] * vec4(f_local_pos, 1.0);
    
}
