#version 450

layout (location = 0) out vec3 f_color;
layout (location = 1) out vec2 f_tex_coord;
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
	mat4 lsm[4];
	vec4 fdist[4];
	vec4 light_dir;
	vec4 view_pos;
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D g_pos;
layout(set = 2, binding = 1) uniform sampler2D g_normal;
layout(set = 2, binding = 2) uniform sampler2D g_albedo_spec;
layout(set = 2, binding = 3) uniform sampler2DArray depth_map;
layout(set = 2, binding = 4) uniform sampler2DArray irradiance_map;
layout(set = 2, binding = 5) uniform samplerCube prefilter_map;
layout(set = 2, binding = 6) uniform sampler2D brdf_LUT;
layout(set = 2, binding = 7) uniform sampler2D ssao_tex;

void main() {
    f_tex_coord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    f_color = colors[gl_VertexIndex];
    gl_Position = vec4(f_tex_coord * 2.0 -1.0, 0.9999, 1.0);
}
