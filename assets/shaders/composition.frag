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


const float PI = 3.141592653539;

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
    float shadow = current_depth > closest_depth + 0.005 ? 1.0 : 0.0;
    if(proj_coords.z > 1.0)
        shadow = 1.0;
    if(proj_coords.z < 0.0)
        shadow = 1.0;
    return shadow;
}  

//F0 is ~0.04 for dielectrics and for metallics the albedo value
vec3 fresnel_schlick(float cos_theta, vec3 F0){
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

float geometry_schlick_ggx(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float distribution_ggx(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = geometry_schlick_ggx(NdotV, roughness);
    float ggx1  = geometry_schlick_ggx(NdotL, roughness);
	
    return ggx1 * ggx2;
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
    
    
    vec3 dir_light_color = vec3(9,9,9);
    vec3 V = normalize(ObjectData.view_pos.xyz - frag_pos);
    vec3 H = normalize(ObjectData.light_dir.xyz + V);
    vec3 N = normalize(norm);
    vec3 R = reflect(-ObjectData.light_dir.xyz, N);
    vec3 L = ObjectData.light_dir.xyz;
    vec3 radiance = vec3(1.0 - shadow) + 0.1; 
    vec3 Lo = vec3(0.0);

    float metallic = texture(g_pos, f_tex_coord).w;
    float roughness = texture(g_normal, f_tex_coord).w;
    float ao = texture(g_albedo_spec, f_tex_coord).w;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    vec3 F = fresnel_schlick(max(dot(H, V),0.0), F0);
    float NDF = distribution_ggx(N, H, roughness);
    float G = geometry_smith(N,V,L,roughness);
    vec3 numerator = NDF * F * G;
    float denom = 4.0 * max(dot(N,V), 0.0) * max(dot(N,L), 0.0) + 0.0001;
    vec3 specular = numerator/denom;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    float NdotL = max(dot(N,L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;

    


    
    vec3 lighting = vec3(0.0);
    //calc point light's contrib
    for (int i = 1; i < 1; ++i) //for each light
    {
        radiance = vec3(3);
        V  = normalize(ObjectData.view_pos.xyz - frag_pos);
        vec3 ld = normalize(point_light_pos - frag_pos);
        H = normalize(ld + V);
        R = reflect(-ld, N);
        L = ld;



        vec3 F0 = vec3(0.04);
        F0 = mix(F0, albedo, metallic);
        vec3 F = fresnel_schlick(max(dot(H, V),0.0), F0);
        float NDF = distribution_ggx(N, H, roughness);
        float G = geometry_smith(N,V,L,roughness);
        vec3 numerator = NDF * F * G;
        float denom = 4.0 * max(dot(N,V), 0.0) * max(dot(N,L), 0.0) + 0.0001;
        vec3 specular = numerator/denom;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        float NdotL = max(dot(N,L), 0.0);
        lighting += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    Lo += lighting;


    vec3 ambient = vec3(0.03) * albedo;
    vec3 color = ambient + Lo;

    //apply tone mapping
    const float exposure = 0.8;
    color = vec3(1.0) - exp(-color* exposure);
    //gamma done automatically!
    /*
    float gamma = 2.2;
    result = pow(result, vec3(gamma));
    */
    
    frag_color = vec4(color, 1.0);
    //frag_color[layer] += 0.05;
}  
