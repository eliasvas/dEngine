#version 450
#extension GL_EXT_debug_printf : enable

layout (location = 0) in vec2 f_tex_coord;

layout (location = 0) out vec4 frag_color;

layout(set = 0, binding = 0) uniform  GlobalBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} GlobalData;

layout(set = 1, binding = 0) uniform  ObjectBuffer{
	vec4 blur_factor;
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D tex;


int blur_size = 4;

void main() {
	vec2 texel_size = 1.0 / vec2(textureSize(tex,0));
	float res = 0.0;
	for (int i  =-blur_size/2; i < blur_size/2; ++i){
		for (int j = -blur_size/2; j < blur_size/2; ++j){
			vec2 offset = (vec2(float(i), float(j))) * texel_size;
			res += texture(tex, f_tex_coord + offset).a;
		}
	}
	frag_color = vec4(texture(tex, f_tex_coord).xyz, res / float(blur_size * blur_size));
	//debugPrintfEXT("%f %f %f\n", res, res, res);
}
