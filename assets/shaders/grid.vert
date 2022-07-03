#version 450

layout (location = 0) out vec3 near_point;
layout (location = 1) out vec3 far_point;

layout(set = 0, binding = 0) uniform  GlobalBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} GlobalData;

layout(set = 1, binding = 0) uniform  ObjectBuffer{
	float modifier;
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D tex_sampler1;

vec3 grid_plane[6] = vec3[](
    vec3(1, 1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
    vec3(-1, -1, 0), vec3(1, 1, 0), vec3(1, -1, 0)
);

vec3 unproject_point(float x, float y, float z, mat4 view, mat4 projection) {
    mat4 viewInv = inverse(view);
    mat4 projInv = inverse(projection);
    vec4 unprojectedPoint =  viewInv * projInv * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}



void main() {
    vec3 p = grid_plane[gl_VertexIndex].xyz;
    near_point = unproject_point(p.x,p.y, 0.0, GlobalData.view, GlobalData.proj).xyz;
    far_point = unproject_point(p.x,p.y, 1.0, GlobalData.view, GlobalData.proj).xyz;
    gl_Position = vec4(p,1.0);
    
    //gl_Position = GlobalData.proj * GlobalData.view * vec4(grid_plane[gl_VertexIndex].xyz, 1.0);
}
