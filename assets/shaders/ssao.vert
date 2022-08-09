#version 450

layout (location = 0) out vec2 f_tex_coord;

vec3 colors[3] = vec3[](
    vec3(0.7, 0.3, 0.1),
    vec3(0.4, 0.2, 0.7),
    vec3(0.4, 0.3, 0.6)
);


vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);



layout(set = 0, binding = 0) uniform  GlobalBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} GlobalData;

layout(set = 1, binding = 0) uniform  ObjectBuffer{
	vec4 kernels[32];
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D g_pos;
layout(set = 2, binding = 1) uniform sampler2D g_normal;
layout(set = 2, binding = 2) uniform sampler2D random_tex;
layout(set = 2, binding = 3) uniform sampler2D depth_map;


void main() {
    f_tex_coord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(f_tex_coord * 2.0 -1.0, 0.1, 1.0);
}
