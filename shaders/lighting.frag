#version 450

layout(set = 0, binding = 0) uniform sampler2D gPosition;
layout(set = 0, binding = 1) uniform sampler2D gNormal;
layout(set = 0, binding = 2) uniform sampler2D gAlbedo;

struct PointLight {
    vec3 pos;
    vec4 color;  // w is brightness
};

layout(std430, set = 1, binding = 0) readonly buffer SceneData {
    // uint count;
    PointLight lights[];
} scene_data;

layout(push_constant) uniform LightingPC {
    mat4 projection_view;
} pc;

// layout(std430, set = 1, binding = 0) buffer SceneData {
//     vec4 elems[];
// } scene;

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

const float ambient = 0.08;

void main() {
    vec3 pos = texture(gPosition, uv).xyz;
    vec3 normal = normalize(texture(gNormal, uv).xyz * 2.0 - 1.0);
    vec3 albedo = texture(gAlbedo, uv).rgb;

    // read first light: elems[0] = position (xyz + padding), elems[1] = color (rgba)

    
    // PointLight light = scene_data.lights[0];
    // vec3 lightPos = scene.elems[0].xyz;
    // vec3 lightColor = scene.elems[1].rgb;

    vec4 light_sum = vec4(0.);
    vec4 light_overlay = vec4(0.);
    for (int i = 0; i < 9; i++) { ///////!!!!!!!!!!!!!!!!!!!!!! hardcoded light amount todo
        vec3 light_pos = scene_data.lights[i].pos;
        vec3 point_light_dist_world = light_pos - pos;
        vec3 point_light_dir_world = normalize(point_light_dist_world);

        vec4 point_light_col_bight = vec4(scene_data.lights[i].color.xyz * scene_data.lights[i].color.w, 1.);
        float point_light_intensity = 1.0 / dot(point_light_dist_world, point_light_dist_world);

        float point_light = max(dot(point_light_dir_world, normal), 0.0) * point_light_intensity;
        vec4 light_col = point_light  * point_light_col_bight;
        light_sum += light_col;


        // drawing light overlay
        
        vec4 clip = pc.projection_view * vec4(light_pos, 1.0);
        vec4 clip_point = pc.projection_view * vec4(pos, 1.0);

        vec3 ndc = clip.xyz / clip.w;
        vec3 ndc_point = clip_point.xyz / clip_point.w;
        vec2 light_uv = ndc.xy * 0.5 + 0.5;

        if (clip.w > 0.0 && ndc.z < ndc_point.z) {
            light_overlay += pow(0.01 / length(uv - light_uv), 99.0) * point_light_col_bight;
        }
        // light_overlay += pow(0.1/ length(uv - (clip_space/2. + vec2(0.5))), 99.) * point_light_col_bight;
    }
    


    outColor = vec4(albedo, 1.0) * max(light_sum, ambient) + light_overlay;// vec4(n * 0.5 + 0.5, 1.0);
}
