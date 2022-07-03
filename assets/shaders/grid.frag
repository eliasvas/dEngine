#version 450

float near = 0.01;
float far = 100;

layout (location = 0) in vec3 near_point;
layout (location = 1) in vec3 far_point;

layout (location = 0) out vec4 frag_color;

layout(set = 0, binding = 0) uniform  GlobalBuffer{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
} GlobalData;

layout(set = 1, binding = 0) uniform  ObjectBuffer{
	float modifier;
} ObjectData;

layout(set = 2, binding = 0) uniform sampler2D tex_sampler1;


vec4 grid(vec3 fragPos3D, float scale) {
    vec2 coord = fragPos3D.xz * scale; // use the scale variable to set the distance between the lines
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1);
    float minimumx = min(derivative.x, 1);
    vec4 color = vec4(0.2, 0.2, 0.2, 1.0 - min(line, 1.0));
    // z axis
    if(fragPos3D.x > -0.4 * minimumx && fragPos3D.x < 0.4 * minimumx)
        color.z = 1.0;
    // x axis
    if(fragPos3D.z > -0.4 * minimumz && fragPos3D.z < 0.4 * minimumz)
        color.x = 1.0;
    return color;
}

float compute_depth(vec3 pos) {
    vec4 clip_space_pos = GlobalData.proj * GlobalData.view * vec4(pos.xyz, 1.0);
    return (clip_space_pos.z / clip_space_pos.w);
}

float compute_linear_depth(vec3 pos) {
    vec4 clip_space_pos = GlobalData.proj * GlobalData.view * vec4(pos.xyz, 1.0);
    float clip_space_depth = (clip_space_pos.z / clip_space_pos.w) * 2.0 - 1.0; // put back between -1 and 1
    float linear_depth = (2.0 * near * far) / (far + near - clip_space_depth * (far - near)); // get linear value between 0.01 and 100
    return linear_depth / far; // normalize
}

void main() {
    float t = -near_point.y/ (far_point.y - near_point.y);
    vec3 frag_pos3d = near_point + t * (far_point - near_point);
    gl_FragDepth = compute_depth(frag_pos3d);
    
    float linear_depth = compute_linear_depth(frag_pos3d);
    float fading = max(0, (0.5 -linear_depth));
    
    frag_color = grid(frag_pos3d, ObjectData.modifier)* float(t > 0);
    frag_color.a *=fading;
}
