#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(location = 0) out  vec3 frag_color;

layout(push_constant) uniform Push {
    vec3 offset;
    float a;
} push;


void main() {
    mat3 rot_mat_x = mat3(
        1, 0, 0,
        0, cos(push.a), -sin(push.a),
        0, sin(push.a), cos(push.a)
    );

    vec3 rot_pos = position * rot_mat_x;
    gl_Position = vec4(rot_pos, 1.0);

    frag_color = color;
}