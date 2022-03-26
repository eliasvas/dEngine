#version 450

layout(location = 0) in vec3 f_normal;
layout(location = 1) in vec2 f_tex_coord;

layout(location = 0) out vec4 out_color;


layout(set = 0, binding = 1) uniform sampler2D tex_sampler;

layout(set  = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
	float modifier;
};

vec3 dir_light = vec3(-1,1,0.2);

void main() {
	vec3 lightdir = normalize(-dir_light);
	float NdotL = max(dot(f_normal, lightdir),0.0);
    vec4 diffuse = texture(tex_sampler, f_tex_coord);
	out_color =  modifier * diffuse * 0.5 + diffuse * 0.5 * NdotL;
	//out_color = diffuse;
}
