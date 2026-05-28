#version 450

struct PointLight {
    vec3 position;
    vec4 color;  // w is brightness
};

layout(set = 1, binding = 0) readonly buffer SceneData {
    // uint count;
    PointLight lights[];
} scene_data;

layout(location = 0) in vec3 position;
layout(location = 3) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_uv;

layout(push_constant) uniform Push {
    mat4 model;
    mat4 trans;
    float time;
} push;

vec3 light_dir = vec3(0.45, 0.5, 0.05);
const float ambient = 0.1;

void main() {
    vec3 trans_normal = normalize((push.model * vec4(normal, 0.0)).xyz);

    gl_Position = push.trans * vec4(position, 1.0);

    vec3 point_light_pos = scene_data.lights[0].position;
    vec3 point_light_dist_world = point_light_pos - position;
    vec3 point_light_dir_world = normalize(point_light_dist_world);

    vec3 point_light_col_bight = scene_data.lights[0].color.xyz * scene_data.lights[0].color.w;
    float point_light_intensity = 1.0 / dot(point_light_dist_world, point_light_dist_world);

    float world_light = 0.;
    float point_light = max(dot(point_light_dir_world, trans_normal), 0.0) * point_light_intensity;
    frag_color = point_light * point_light_col_bight + world_light;
    frag_uv = uv;
}