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
	mat4 lsm[4];
	float fdist[4];
	vec4 light_dir;
	vec4 view_pos;
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D g_pos;
layout(set = 2, binding = 1) uniform sampler2D g_normal;
layout(set = 2, binding = 2) uniform sampler2D g_albedo_spec;
layout(set = 2, binding = 3) uniform sampler2DArray depth_map;



float shadow_calc(vec4 frag_pos_light_space,int cascade_index)
{
    // perform perspective divide
    vec3 proj_coords = frag_pos_light_space.xyz / frag_pos_light_space.w;
    //negate y coord (because vulkan????)
    // transform to [0,1] range
    proj_coords.xy = proj_coords.xy * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closest_depth = texture(depth_map, vec3(proj_coords.xy,cascade_index)).r; 
    // get depth of current fragment from light's perspective
    float current_depth = proj_coords.z;
    // check whether current frag pos is in shadow
    float shadow = current_depth > closest_depth + 0.001 ? 1.0 : 0.0;
    if(proj_coords.z > 1.0)
        shadow = 1.0;
    if(proj_coords.z < 0.0)
        shadow = 1.0;
    return shadow;
}  

vec3 point_light_color = vec3(50.0,50.0,50.0);
vec3 point_light_pos = vec3(2.0,6.0,0.0);

void main()
{             
    // retrieve data from G-buffer
    vec3 frag_pos = texture(g_pos, f_tex_coord).xyz;
    vec3 norm = texture(g_normal, f_tex_coord).xyz;
    vec3 albedo = texture(g_albedo_spec, f_tex_coord).xyz;
    //float spec = texture(g_albedo_spec, f_tex_coord).a;
    
    vec4 frag_pos_view_space = GlobalData.view * vec4(frag_pos,1.0);
    float depth_value = abs(frag_pos_view_space.z);
    int layer = -1;
    for (int i = 0; i < 4;++i)
    {
    	if (depth_value <ObjectData.fdist[i])
    	{
    		layer = i;
    		break;
    	}
    }
    
    vec4 frag_pos_light_space = ObjectData.lsm[layer] * vec4(frag_pos,1.0);
    float shadow = shadow_calc(frag_pos_light_space, layer);
    
    
    float spec_str = 0.4;
    vec3 dir_light_color = vec3(0.9,0.9,0.9);
    vec3 view_dir = normalize(ObjectData.view_pos.xyz - frag_pos);
    vec3 halfway_dir = normalize(-ObjectData.light_dir.xyz + view_dir);
    vec3 reflect_dir = reflect(-ObjectData.light_dir.xyz, norm);
    
    float spec = pow(max(dot(view_dir, halfway_dir), 0.0), 32);
    vec3 specular = spec_str * spec * dir_light_color;
    
    float diff = max(dot(norm, ObjectData.light_dir.xyz), 0.0);
    vec3 diffuse = diff * dir_light_color;
    
    vec3 ambient = vec3(0.1,0.1,0.1);
    
    vec3 result= (ambient) * albedo + (diffuse+specular) * albedo * (1-shadow); 

    vec3 lighting = vec3(0.0);
    //calc point light's contrib
    {
        vec3 light_dir  = normalize(point_light_pos - frag_pos);
        float diff = max(dot(light_dir, norm), 0.0);
        vec3 diffuse = point_light_color * diff * albedo;
        float distance = length(frag_pos - point_light_pos);
        vec3 res = diffuse;
        res *= 1.0 / (distance * distance);
        lighting+=res;
    }
    result +=lighting;

    //apply tone mapping
    const float exposure = 0.1;
    result = vec3(1.0) - exp(-result * exposure);

    /*
    float gamma = 2.2;
    result = pow(result, vec3(gamma));
    */
    
    frag_color = vec4(result, 1.0);
}  
