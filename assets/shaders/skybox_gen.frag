

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
	vec3 normal = normalize(f_local_pos);
	vec3 irradiance = vec3(0);

	vec3 up = vec3(0,1,0);
	vec3 right = normalize(cross(up, normal));
	up = normalize(cross(normal, right));

	float sample_delta = 0.025;
	float nr_samples = 0.0;
	for (float phi = 0.0; phi < 2.0 * PI; phi += sample_delta){
		for (float theta = 0.0; theta < 0.5 * PI; theta += sample_delta){
			//spherical to cartesian (in tangent space)
			vec3 tangent_sample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			//tangent space to world
			vec3 sample_vec = tangent_sample.x * right + tangent_sample.y * up + tangent_sample.z * normal;
			irradiance += texture(tex_sampler1, sample_vec).rgb * cos(theta) * sin(theta);
			nr_samples++;
		}
	}
	irradiance = PI * irradiance * (1.0 / float(nr_samples));

	//this is the code for the normal skybox
	//vec3 env_color = texture(tex_sampler1, f_local_pos).rgb;
    //env_color = env_color / (env_color + vec3(1.0));
    //env_color = pow(env_color, vec3(1.0/2.2));

    color = vec4(irradiance, 1.0);
}
