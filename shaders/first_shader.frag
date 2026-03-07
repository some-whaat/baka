#version 450

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 uv;

layout (location = 0) out vec4 out_color;

// layout(push_constant) uniform Push {
//     vec2 offset;
//     vec3 color;
// } push;

void main() {
    
    out_color = vec4(uv * frag_color.x, 1., 1.);
    // out_color = vec4(push.color, 1.0);
}