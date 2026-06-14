#version 450

layout(set = 0, binding = 0) uniform sampler2D gPosition;
layout(set = 0, binding = 1) uniform sampler2D gNormal;
layout(set = 0, binding = 2) uniform sampler2D gAlbedo;

layout(std430, set = 1, binding = 0) buffer SceneData {
    vec4 elems[];
} scene;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 pos = texture(gPosition, uv).xyz;
    vec3 n = normalize(texture(gNormal, uv).xyz * 2.0 - 1.0);
    vec3 albedo = texture(gAlbedo, uv).rgb;

    // read first light: elems[0] = position (xyz + padding), elems[1] = color (rgba)
    vec3 lightPos = scene.elems[0].xyz;
    vec3 lightColor = scene.elems[1].rgb;

    // debug: visualize normal from G-buffer
    outColor = vec4(n * 0.5 + 0.5, 1.0);
}
