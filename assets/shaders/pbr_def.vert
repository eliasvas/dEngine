#version 450

layout(location = 0) in vec2 tex_coord;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in vec3 vertex_pos;
layout(location = 3) in vec4 tangent;


layout(set = 0, binding = 0) uniform  GlobalBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} GlobalData;

layout(set = 1, binding = 0) uniform  ObjectBuffer{
	mat4 model;
	vec3 color[2];
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D base_color_tex;
layout(set = 2, binding = 1) uniform sampler2D orm_tex;
layout(set = 2, binding = 2) uniform sampler2D normal_tex;
layout(set = 2, binding = 3) uniform sampler2D emissive_tex;

layout(location = 0) out vec3 f_frag_pos;
layout(location = 1) out vec3 f_normal;
layout(location = 2) out vec2 f_tex_coord;
layout(location = 3) out mat3 tbn;
void main() {
    gl_Position = GlobalData.proj * GlobalData.view * ObjectData.model * vec4(vertex_pos, 1.0);
    f_normal = mat3(transpose(inverse(ObjectData.model))) * vertex_normal;
    f_frag_pos = (ObjectData.model * vec4(vertex_pos,1.0f)).xyz;
    f_tex_coord = tex_coord;

	//TODO: fix normal mapping
	//https://www.gamedeveloper.com/programming/three-normal-mapping-techniques-explained-for-the-mathematically-uninclined
	vec3 N = vertex_normal;
	vec3 n = normalize( ( ObjectData.model * vec4( N, 0.0 ) ).xyz );
	vec3 t = normalize( ( ObjectData.model * vec4( tangent.xyz, 0.0 ) ).xyz );
	vec3 b = normalize( ( ObjectData.model * vec4( ( cross( N, tangent.xyz ) * tangent.w ), 0.0 ) ).xyz );
	tbn = mat3( t, b, n );
    
}