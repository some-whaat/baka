#version 450

layout (location = 0) out vec4 out_color;

void main() {
    out_color = vec4(gl_PointCoord.x, gl_PointCoord.y, 1.0, 1.0);
    // out_color = vec4(0., 0., 1.0, 1.0);
}