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
	mat4 model;
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D tex_sampler1;


layout(location = 0) out vec3 f_frag_pos;
layout(location = 1) out vec3 f_normal;
layout(location = 2) out vec2 f_tex_coord;

void main() {
    gl_Position = GlobalData.proj * GlobalData.view * ObjectData.model * vec4(vertex_pos, 1.0);
    vec3 N = mat3(transpose(inverse(ObjectData.model))) * vertex_normal;  
    f_normal = N;
    f_frag_pos = (ObjectData.model * vec4(vertex_pos,1.0f)).xyz;
    f_tex_coord = tex_coord;
    
}
