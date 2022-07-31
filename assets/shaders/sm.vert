#version 450


#extension GL_EXT_multiview : enable

layout(location = 0) in vec3 vertex_pos;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in vec2 tex_coord;

layout(set = 0, binding = 0) uniform  GlobalBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} GlobalData;

layout(set = 1, binding = 0) uniform  ObjectBuffer{
	mat4 model;
	mat4 lsm[4];
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D tex_sampler1;


void main() {
    gl_Position = ObjectData.lsm[gl_ViewIndex] * ObjectData.model * vec4(vertex_pos, 1.0);
    
}

