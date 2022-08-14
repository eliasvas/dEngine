

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

layout(set = 2, binding = 0) uniform samplerCube tex_sampler1;

const float PI = 3.14159265359;

void main() {
	//this is the code for the normal skybox
	vec3 env_color = texture(tex_sampler1, f_local_pos).rgb;
    //env_color = env_color / (env_color + vec3(1.0));
    //env_color = pow(env_color, vec3(1.0/2.2));

    color = vec4(env_color, 1.0);
}
