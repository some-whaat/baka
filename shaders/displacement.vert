#version 450

layout(set = 2, binding = 0) readonly buffer ObjectTransforms {
    mat4 models[];
} objects;

layout(push_constant) uniform Push {
    mat4 projection_view;
    uint objectIndex;
    float time;
    // padding to 16 byte boundary
    float _pad0;
} push;


layout(binding = 0) uniform sampler2D displacement_map;
layout(binding = 1) uniform sampler2D normal_map;

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
// layout(location = 2) in vec3 normal;
// layout(location = 3) in vec3 color;

layout (location = 0) out float out_world_light;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec2 out_uv;
layout (location = 3) out vec3 out_pos;

const float intensity = 1.;

vec3 light_dir = vec3(0.45, 0.5, 0.05);

void main() {
    mat4 model = objects.models[push.objectIndex];

    // const float delta = 0.0001;
    vec2 texel_size = 1.0 / textureSize(displacement_map, 0);
    
    // float hL = texture(displacement_map, uv - vec2(texel_size.x, 0.0)).r;
    // float hR = texture(displacement_map, uv + vec2(texel_size.x, 0.0)).r;
    // float hD = texture(displacement_map, uv - vec2(0.0, texel_size.y)).r;
    // float hU = texture(displacement_map, uv + vec2(0.0, texel_size.y)).r;
    
    // float du = (hR - hL) * heightScale;
    // float dv = (hU - hD) * heightScale;
    // vec3 tangent = vec3(du, 0., dv);


    vec3 tangent_normal = normalize(texture(normal_map, uv).xyz * 2.0 - 1.0);
    
    // 2. Transform vertex normal and tangent to world space
    // Use inverse transpose for normal transformation (or just mat3 if no non-uniform scale)
    mat3 normal_matrix = mat3(transpose(inverse(model)));
    vec3 world_normal = normalize(inverse(model) * normal);
    vec3 world_tangent = normalize(mat3(model) * tangent.xyz);
    
    // 3. Re-orthogonalize tangent (Gram-Schmidt)
    world_tangent = normalize(world_tangent - dot(world_tangent, world_normal) * world_normal);
    
    // 4. Calculate bitangent with handedness from tangent.w
    vec3 world_bitangent = cross(world_normal, world_tangent) * tangent.w;
    
    // 5. Build TBN matrix
    mat3 TBN = mat3(world_tangent, world_bitangent, world_normal);


    vec3 normal = ( inverse(model) * vec4(2.0 * (texture(normal_map, uv).xyz - vec3(0.5)), 0.)).xyz;

    vec3 displacement = vec3(0., texture(displacement_map, uv).x, 0.);
    vec3 new_pos = (position + displacement) * intensity;
    // normal = (normal + displacement) * intensity;
    vec4 pos = push.projection_view * model * vec4(new_pos, 1.0);
    gl_Position = pos;

    const vec3 tangent = vec3(1, 0, 0); ///// HARDCODING todo

    // vec3 T = normalize(mat3(model) * a_Tangent.xyz);
    // vec3 N = normalize(mat3(model) * a_Normal);
    // // Re-orthogonalize T
    // T = normalize(T - dot(T, N) * N);
    // // Calculate B with handedness correction
    // vec3 B = cross(N, T) * a_Tangent.w;  // tangent.w stores handedness (±1)
    // v_TBN = mat3(T, B, N);


    out_normal = normal;
    out_uv = uv;
    out_pos = pos.xyz;


    // float world_light = dot(light_dir, trans_normal);
    // out_color = world_light + ambient;
    // out_normal = trans_normal;
    // out_pos = (model * vec4(position, 1.0)).xyz;
    // out_uv = uv;
}