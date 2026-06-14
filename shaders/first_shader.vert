#version 450

layout(set = 2, binding = 0) readonly buffer ObjectTransforms {
    mat4 models[];
} objects;

layout(push_constant) uniform Push {
    mat4 projection_view;
    uint objectIndex;
    float time;
    // padding to 16 byte boundary
    float _pad0;
} push;

layout(location = 0) in vec3 position;
layout(location = 3) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 1) in vec2 uv;

layout (location = 0) out float out_color;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec2 out_uv;
layout (location = 3) out vec3 out_pos;

vec3 light_dir = vec3(0.45, 0.5, 0.05);
const float ambient = 0.1;

void main() {
    mat4 model = objects.models[push.objectIndex];
    vec3 trans_normal = normalize((model * vec4(normal, 0.0)).xyz);

    // apply model transform before projection
    gl_Position = push.projection_view * model * vec4(position, 1.0);

    float world_light = dot(light_dir, trans_normal);
    out_color = world_light + ambient;
    out_normal = trans_normal;
    out_pos = (model * vec4(position, 1.0)).xyz;
    out_uv = uv;
}