#version 450

vec2 positions[3] = vec2[](
    vec2(-0.6, -0.7),
    vec2(0.6, -0.3),
    vec2(0, 0.8)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}