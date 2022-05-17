#version 450

layout(triangles, invocations = 5) in;
layout(triangle_strip, max_vertices = 3) out;

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
	for (int i = 0; i < 3; ++i)
    {
        gl_Position = 
            ObjectData.lsm[gl_InvocationID] * gl_in[i].gl_Position;
        gl_Layer = gl_InvocationID;
        EmitVertex();
    }
    EndPrimitive();
}

