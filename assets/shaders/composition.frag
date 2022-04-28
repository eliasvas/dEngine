#version 450

layout(location = 0) out vec4 frag_color;
  
layout (location = 0) in vec3 f_color;
layout (location = 1) in vec2 f_tex_coord;



layout(set = 0, binding = 0) uniform  GlobalBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} GlobalData;

layout(set = 1, binding = 0) uniform  ObjectBuffer{
	vec4 color;
	float modifier;
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D g_pos;
layout(set = 2, binding = 1) uniform sampler2D g_normal;
layout(set = 2, binding = 2) uniform sampler2D g_albedo_spec;

void main()
{             
    // retrieve data from G-buffer
    vec3 frag_pos = texture(g_pos, f_tex_coord).rgb;
    vec3 norm = texture(g_normal, f_tex_coord).rgb;
    vec3 albedo = texture(g_albedo_spec, f_tex_coord).rgb;
    float spec = texture(g_albedo_spec, f_tex_coord).a;
    
    
    vec3 lighting = albedo * 1.0;
    
    frag_color = vec4(lighting, 1.0);
}  
