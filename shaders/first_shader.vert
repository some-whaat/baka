#version 450

layout(location = 0) in vec3 position;
layout(location = 3) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_uv;

layout(push_constant) uniform Push {
    mat4 trans;
    // float time;
    // vec3 color;
} push;

vec3 light_dir = vec3(0.45, 0.5, 0.05);
const float ambient = 0.6;

void main() {
    //gl_Position = vec4(position.x * cos(push.time), position.y * sin(push.time), position.z, 1.0);// * push.trans;
    gl_Position = push.trans * vec4(position, 1.0);
    vec3 trans_normal = (push.trans * vec4(normal, 1.0)).xyz;

    // gl_Position = vec4(position * mat2(cos(push.rot), -sin(push.rot), sin(push.rot), cos(push.rot)) + push.offset, 0.0, 1.0);

    float light = dot(light_dir, trans_normal) + ambient;
    frag_color = color * light;
    frag_uv = uv;
}