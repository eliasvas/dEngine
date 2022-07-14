#version 450

layout(location = 0) in vec2 tex_coord;
layout(location = 1) in vec3 vertex_pos;
layout(location = 2) in vec4 joint;
layout(location = 3) in vec4 weight;

layout(set = 0, binding = 0) uniform  GlobalBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} GlobalData;

layout(set = 1, binding = 0) uniform  ObjectBuffer{
	mat4 model;
	mat4 joint_mat[25];
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D base_color_tex;
layout(set = 2, binding = 1) uniform sampler2D orm_tex;
layout(set = 2, binding = 2) uniform sampler2D normal_tex;
layout(set = 2, binding = 3) uniform sampler2D emissive_tex;

layout(location = 0) out vec3 f_frag_pos;
layout(location = 1) out vec3 f_normal;
layout(location = 2) out vec2 f_tex_coord;

void main() {
	mat4 joint_matrix = weight.x * ObjectData.joint_mat[int(joint.x)]+
						weight.y * ObjectData.joint_mat[int(joint.y)]+ 
						weight.z * ObjectData.joint_mat[int(joint.z)]+
						weight.w * ObjectData.joint_mat[int(joint.w)];
    gl_Position = GlobalData.proj * GlobalData.view * joint_matrix * ObjectData.model * vec4(vertex_pos, 1.0);
    f_frag_pos = (ObjectData.model * vec4(vertex_pos,1.0f)).xyz;
    f_tex_coord = tex_coord;
    
}
