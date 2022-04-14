#version 450

layout(location = 0) in vec3 f_color;

layout(location = 0) out vec4 out_color;

layout( push_constant ) uniform constants
{
	vec4 data;
	mat4 render_matrix;
} PushConstants;

vec3 dir_light = vec3(-1,1,0.2);

void main() {
	out_color = vec4(f_color.x, f_color.y, f_color.z, 1.0);
}
