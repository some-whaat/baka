#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 frag_color;

layout(push_constant) uniform Push {
    mat4 trans;
    // float time;
    // vec3 color;
} push;


void main() {
    //gl_Position = vec4(position.x * cos(push.time), position.y * sin(push.time), position.z, 1.0);// * push.trans;
    gl_Position = push.trans * vec4(position, 1.0);

    // gl_Position = vec4(position * mat2(cos(push.rot), -sin(push.rot), sin(push.rot), cos(push.rot)) + push.offset, 0.0, 1.0);

    frag_color = color;
}