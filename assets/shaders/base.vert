#version 450

layout(location = 0) in vec3 vertex_pos;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in vec2 tex_coord;

layout( push_constant ) uniform constants
{
	vec4 data;
	mat4 render_matrix;
} PushConstants;

layout(location = 0) out vec3 f_color;

void main() {
    gl_Position = vec4(vertex_pos, 1.0);
    gl_Position.z = 0.799;
    f_color = PushConstants.data.xyz;
}
