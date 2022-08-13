

#version 450

layout(location = 0) in vec3 f_local_pos;

layout(location = 0) out vec4 color;

layout(set = 0, binding = 0) uniform  GlobalBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} GlobalData;

layout(set = 1, binding = 0) uniform  ObjectBuffer{
	mat4 p;
    mat4 v[6];
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D tex_sampler1;


const vec2 inv_atan = vec2(0.1591, 0.3183);
vec2 sample_spherical_map(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= inv_atan;
    uv += 0.5;
    return uv;
}

void main() {
	vec2 uv = sample_spherical_map(normalize(f_local_pos)); // make sure to normalize localPos
    vec3 c = texture(tex_sampler1, uv).rgb;
    
    color = vec4(c, 1.0);
}
