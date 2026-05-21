#version 450

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 uv;

layout (location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D tex_sampler;

// layout(push_constant) uniform Push {
//     vec2 offset;
//     vec3 color;
// } push;

void main() {
    
    vec4 texColor = texture(tex_sampler, uv);
    out_color = texColor * vec4(frag_color, 1.);
    // out_color = vec4(push.color, 1.0);
}