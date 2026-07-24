#version 330 core
in vec3 v_normal, v_world_pos;
in vec4 v_color;
uniform vec3 u_light_dir;
uniform vec3 u_ambient;
uniform vec3 u_sun_color;
uniform vec3 u_fog_color;
uniform float u_fog_density;
uniform vec4 u_baseColor;
uniform vec3 u_cam_pos;      // camera world position for distance fog
out vec4 frag_color;

void main() {
    vec3 n = normalize(v_normal);
    float ndotl = max(dot(n, normalize(-u_light_dir)), 0.0);

    // ambient + diffuse sun, modulated by vertex colour × base colour
    vec3 lit = u_ambient + u_sun_color * ndotl;
    vec3 color = v_color.rgb * u_baseColor.rgb * lit;

    // exponential distance fog
    float dist = length(v_world_pos - u_cam_pos);
    float fog_factor = exp(-u_fog_density * dist);
    color = mix(u_fog_color, color, clamp(fog_factor, 0.0, 1.0));

    frag_color = vec4(color, v_color.a * u_baseColor.a);
}
