#version 450

//#extension GL_EXT_multiview : enable

layout(set = 0, binding = 0) uniform  GlobalBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} GlobalData;

layout(set = 1, binding = 0) uniform  ObjectBuffer{
	mat4 p;
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D tex_sampler1;


layout(location = 0) out vec2 f_tex_coord;

void main() {
    f_tex_coord = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	vec2 ftc = vec2(f_tex_coord.x, 1.0 - f_tex_coord.y);
    gl_Position = vec4(ftc * 2.0 -1.0, 0.90, 1.0f);
}
