#version 330 core
in vec3 v_normal, v_world_pos;
uniform vec3 u_light_dir;
out vec4 frag_color;
void main() {
    vec3 n = normalize(v_normal);
    float ndotl = max(dot(n, normalize(-u_light_dir)), 0.1);
    frag_color = vec4(vec3(0.8) * ndotl, 1.0);
}
