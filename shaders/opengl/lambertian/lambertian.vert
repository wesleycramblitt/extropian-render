#version 330 core
layout(location=0) in vec3 a_pos;
layout(location=1) in vec3 a_norm;
layout(location=4) in vec4 a_color;
uniform mat4 u_model, u_view, u_proj;
out vec3 v_normal, v_world_pos;
out vec4 v_color;
void main() {
    vec4 wp = u_model * vec4(a_pos, 1.0);
    v_world_pos = wp.xyz;
    v_normal = mat3(u_model) * a_norm;
    v_color = a_color;
    gl_Position = u_proj * u_view * wp;
}
