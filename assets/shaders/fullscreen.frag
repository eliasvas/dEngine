#version 450


layout (location = 0) in vec3 f_color;
layout (location = 1) in vec2 f_tex_coord;

layout (location = 0) out vec4 frag_color;
//layout (location = 1) out vec4 frag_neg_color;


void main() {
    frag_color = vec4(f_color,1.0);
    
    //frag_neg_color = vec4(vec3(1,1,1) - f_color,1.0);
}
