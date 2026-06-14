#version 450

layout (location = 0) in float in_world_light;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 in_pos;

// G-buffer attachments: position, normal (encoded), albedo
layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gAlbedo;

layout(binding = 0) uniform sampler2D tex_sampler;

void main() {
    vec4 tex_color = texture(tex_sampler, uv);

    // store world-space position (w = 1.0)
    gPosition = vec4(in_pos, 1.0);

    // encode normal from [-1,1] into [0,1] for storage
    vec3 encodedNormal = normalize(in_normal) * 0.5 + 0.5;
    gNormal = vec4(encodedNormal, 1.0);

    // store albedo (texture color)
    gAlbedo = tex_color;
}