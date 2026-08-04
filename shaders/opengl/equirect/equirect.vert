#version 330 core
layout(location=0) in vec3 a_pos;
uniform mat4 u_model, u_view, u_proj;
out vec3 v_world_pos;
void main() {
    vec4 wp = u_model * vec4(a_pos, 1.0);
    v_world_pos = wp.xyz;
    gl_Position = u_proj * u_view * wp;
}
