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
	vec4 kernels[32];
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D g_pos;
layout(set = 2, binding = 1) uniform sampler2D g_normal;
layout(set = 2, binding = 2) uniform sampler2D random_tex;
layout(set = 2, binding = 3) uniform sampler2D depth_map;


vec2 noise_scale = vec2(800.0/4.0,600.0/4.0);
float radius = 0.05;
float bias = 0.025;

float random (vec2 st) {
    return fract(sin(dot(st.xy,
                         vec2(12.9898,78.233)))*
        43758.5453123);
}

void main() {
	vec3 frag_pos   = texture(g_pos, f_tex_coord).xyz;
	vec3 normal     = normalize(texture(g_normal, f_tex_coord).rgb);
	vec3 random_vec = normalize(texture(random_tex, f_tex_coord * noise_scale).xyz);
	random_vec = random_vec * 2.0 - 1.0;

	vec3 tangent = normalize(random_vec - normal * dot(random_vec, normal));
	vec3 bitangent = cross(tangent, normal);
	mat3 TBN = mat3(tangent, bitangent, normal);

	float occlusion = 0.0;
	for (int i = 0; i < 32; ++i){
		vec3 sample1 = TBN * ObjectData.kernels[i].xyz;
		sample1 = frag_pos;// + sample1 * radius;

		
		//then get sample pos in screen space
		vec4 offset = vec4(sample1, 1.0); 
		
		offset = GlobalData.proj * offset; //transfer to clip space
		offset.xyz /= offset.w; //persp divide
		
		offset.xyz = offset.xyz * 0.5 + 0.5; //transform to 0.0 - 1.0 ??? vulkaaan
		


		float sample_depth = texture(depth_map, offset.xy).w;

		float range_check = smoothstep(0.0, 1.0, 32.0 / abs(frag_pos.z - sample_depth));
		occlusion += (sample_depth >= sample1.z + bias ? 1.0 : 0.0) * range_check;
		
	}
	//debugPrintfEXT("Clip-Space offset is %f %f %f", GlobalData.proj[0][0], random_vec.y, random_vec.z);
    //debugPrintfEXT("%f %f %f\n", random_vec.x, random_vec.y, random_vec.z);
	frag_color = vec4(1.0 - (occlusion / 32.0));
	debugPrintfEXT("%f %f %f\n", frag_color.x, frag_color.y, frag_color.z);
	//frag_color.a = 1.0;
}
