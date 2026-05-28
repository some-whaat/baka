#version 450

struct PointLight {
    vec3 position;
    vec4 color;  // w is brightness
};

layout(set = 1, binding = 0) readonly buffer SceneData {
    // uint count;
    PointLight lights[];
} scene_data;

layout (location = 0) in float in_world_light;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 in_pos;


layout (location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D tex_sampler;

// layout(push_constant) uniform Push {
//     vec2 offset;
//     vec3 color;
// } push;

void main() {

    vec3 point_light_pos = scene_data.lights[0].position;
    vec3 point_light_dist_world = point_light_pos - in_pos;
    vec3 point_light_dir_world = normalize(point_light_dist_world);

    vec4 point_light_col_bight = vec4(scene_data.lights[0].color.xyz * scene_data.lights[0].color.w, 1.);
    float point_light_intensity = 1.0 / dot(point_light_dist_world, point_light_dist_world);
    
    float point_light = max(dot(point_light_dir_world, in_normal), 0.0) * point_light_intensity;
    vec4 tex_color = texture(tex_sampler, uv);
    vec4 col = tex_color * (point_light  * point_light_col_bight + vec4(in_world_light) * 0.6 + 0.3);
    out_color = col; // pow(col, vec4(2.2));
    // out_color = vec4(push.color, 1.0);
}