
const char DUI_VERT[1000] = 
"#version 450\n\
layout(location = 0) in vec3 vertex_pos;\n\
layout(location = 1) in vec3 vertex_normal;\n\
layout(location = 2) in vec2 tex_coord;\n\
layout(set = 0, binding = 0) uniform  GlobalBuffer{\n\
	mat4 view;\n\
	mat4 proj;\n\
	mat4 viewproj;\n\
} GlobalData;\n\
layout(set = 1, binding = 0) uniform  ObjectBuffer{\n\
	float window_width;\n\
	float window_height;\n\
} ObjectData;\n\
layout(set = 2, binding = 0) uniform usampler2D tex_sampler1;\n\
layout(location = 0) out vec4 f_color;\n\
layout(location = 1) out vec2 f_tex_coord;\n\
void main() {\n\
    gl_Position = GlobalData.proj * GlobalData.view * vec4(vertex_pos.x, vertex_pos.y, 1.0,1.0);\n\
    f_color= vec4(vertex_normal.x, vertex_normal.y, vertex_normal.z, vertex_pos.z);\n\
    f_tex_coord = tex_coord;\n\
    gl_Position.z = 0.0;\n\
}";

const char DUI_FRAG[1000] = 
"#version 450\n\
layout(location = 0) in vec4 f_color;\n\
layout(location = 1) in vec2 f_tex_coord;\n\
layout(location = 0) out vec4 out_color;\n\
layout(set = 0, binding = 0) uniform  GlobalBuffer{\n\
	mat4 view;\n\
	mat4 proj;\n\
	mat4 viewproj;\n\
} GlobalData;\n\
layout(set = 1, binding = 0) uniform  ObjectBuffer{\n\
	uint window_width;\n\
	uint window_height;\n\
} ObjectData;\n\
layout(set = 2, binding = 0) uniform usampler2D tex_sampler1;\n\
void main2() {\n\
	vec4 col = texture(tex_sampler1, f_tex_coord);\n\
	out_color = f_color;\n\
	out_color.a = col.x;\n\
	if (col.x < 0.8)discard;\n\
	//out_color = vec4(1,0,0,1);\n\
}\n\
void main() {\n\
	uvec4 col = texture(tex_sampler1, f_tex_coord);\n\
	float c = col.x / 255.0;\n\
	out_color = f_color;\n\
	if(c <0.1)discard;\n\
	out_color.a = c;\n\
}";