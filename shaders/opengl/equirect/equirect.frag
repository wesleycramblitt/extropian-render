#version 330 core
uniform sampler2D u_equirect;
in vec3 v_world_pos;
out vec4 frag_color;

const float PI = 3.141592653589793;

void main() {
    vec3 d = normalize(v_world_pos);
    float u = 0.5 + atan(d.x, d.z) / (2.0 * PI);
    float v = 0.5 - asin(d.y) / PI;
    frag_color = texture(u_equirect, vec2(u, v));
}
