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
float radius = 0.3;
float bias = 0.025;

float random (vec2 st) {
    return fract(sin(dot(st.xy,
                         vec2(12.9898,78.233)))*
        43758.5453123);
}

const int samples = 16;
vec3 sample_sphere[16] = {
      vec3( 0.5381, 0.1856,-0.4319), vec3( 0.1379, 0.2486, 0.4430),
      vec3( 0.3371, 0.5679,-0.0057), vec3(-0.6999,-0.0451,-0.0019),
      vec3( 0.0689,-0.1598,-0.8547), vec3( 0.0560, 0.0069,-0.1843),
      vec3(-0.0146, 0.1402, 0.0762), vec3( 0.0100,-0.1924,-0.0344),
      vec3(-0.3577,-0.5301,-0.4358), vec3(-0.3169, 0.1063, 0.0158),
      vec3( 0.0103,-0.5869, 0.0046), vec3(-0.0897,-0.4940, 0.3287),
      vec3( 0.7119,-0.0154,-0.0918), vec3(-0.0533, 0.0596,-0.5411),
      vec3( 0.0352,-0.0631, 0.5460), vec3(-0.4776, 0.2847,-0.0271)
};

float near = 0.1;
float far = 100;
float lin_depth(float d) {
    //vec4 clip_space_pos = GlobalData.proj * GlobalData.view * vec4(pos.xyz, 1.0);
    //float clip_space_depth = (clip_space_pos.z / clip_space_pos.w) * 2.0 - 1.0; // put back between -1 and 1
    d = d * 2.0 - 1.0;
	float linear_depth = (2.0 * near * far) / (far + near - d * (far - near)); // get linear value between 0.01 and 100
    return linear_depth / far; // normalize
}

void main() {
	vec3 frag_pos   = texture(g_pos, f_tex_coord).xyz;
	vec3 normal     = normalize(texture(g_normal, f_tex_coord).rgb);
	vec3 random_vec = normalize(texture(random_tex, f_tex_coord * noise_scale ).xyz);
	random_vec.xy = random_vec.xy *2.0 - 1.0;
	//normal = normal * 2.0 - 1.0;
	normal =  vec3(GlobalData.view * vec4(normal,1.0));
	normal = normalize(normal);

	vec3 tangent = cross(normal, normalize(random_vec - normal * dot(random_vec, normal)));
	vec3 bitangent = cross(tangent, normal);
	mat3 TBN = mat3(tangent, bitangent, normal);
	//debugPrintfEXT("%f %f %f\n", frag_pos.x, frag_pos.y, frag_pos.z);
	float occlusion = 0.0;
	for (int i = 0; i < 16; ++i){
		vec3 sample1 = TBN * sample_sphere[i];
		sample1 = vec4(GlobalData.view * vec4(frag_pos,1)).xyz + sample1 * radius;
		
		//then get sample pos in screen space
		vec4 offset = vec4(sample1, 1.0); 
		
		offset = GlobalData.proj * offset; //transfer to clip space
		offset.xyz /= offset.w; //persp divide
		
		offset.xyz = offset.xyz * 0.5 + 0.5; //transform to 0.0 - 1.0 ??? vulkaaan
		
		float sample_depth = -texture(g_normal, offset.xy).w;
		//if (i == 0)debugPrintfEXT("%.2f ", sample_depth- sample1.z);
		occlusion += (sample_depth >= sample1.z - 0.00024 ? 1.0 : 0.0);		
	}
	//frag_color = vec4(texture(g_normal, f_tex_coord).rgb,1.0 - (occlusion / 32.0));
	frag_color = vec4(vec3(1.0 - (occlusion / 16.0)),1);

	//frag_color = vec4(texture(random_tex, f_tex_coord).xyz,1);
}
