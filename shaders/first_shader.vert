#version 450

struct PointLight {
    vec3 position;
    vec4 color;  // w is brightness
};

layout(set = 1, binding = 0) readonly buffer SceneData {
    uint count;
    PointLight lights[];
} scene_data;

layout(location = 0) in vec3 position;
layout(location = 3) in vec3 color;
layout(location = 2) in vec3 normal;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_uv;

layout(push_constant) uniform Push {
    mat4 trans;
    float time;
    // vec3 color;
} push;

vec3 light_dir = vec3(0.45, 0.5, 0.05);
const float ambient = 0.1;

void main() {
    //gl_Position = vec4(position.x * cos(push.time), position.y * sin(push.time), position.z, 1.0);// * push.trans;
    gl_Position = push.trans * vec4(position, 1.0);
    light_dir = normalize(vec3(sin(push.time), -cos(push.time), -sin(push.time)));
    vec3 trans_normal = normalize((push.trans * vec4(normal, 0.0)).xyz);

    // gl_Position = vec4(position * mat2(cos(push.rot), -sin(push.rot), sin(push.rot), cos(push.rot)) + push.offset, 0.0, 1.0);
    vec3 light_pos = vec3(sin(push.time), 0., cos(push.time)) * 3.; // scene_data.lights[0].position;
    vec3 light_dist_world = light_pos - (push.trans * vec4(position, 1.0)).xyz;
    vec3 light_dir_world = normalize(light_dist_world);

    // vec3 dir_to_pos_light = /*scene_data.lights[0].position*/ vec3(sin(push.time), 0., cos(push.time)) - position;

    // float light = max(dot(light_dir, trans_normal), 0.0) + ambient;
    frag_color = /*color * light  + */max(dot(light_dir_world, trans_normal), 0.0) * vec3(1., 0., 1.) * 0.9/length(light_dist_world); // * scene_data.lights[0].color.xyz * scene_data.lights[0].color.w;
    frag_uv = uv;
}