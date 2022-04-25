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
void main() {
    f_tex_coord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	f_color = colors[gl_VertexIndex];
    gl_Position = vec4(f_tex_coord * 2.0 -1.0, 0.8999, 1.0f);
}
