#version 330 core
layout(location=0) in vec3 a_pos;
layout(location=2) in vec3 a_uv;
layout(location=4) in vec4 a_color;

uniform mat4 u_model, u_view, u_proj;

out vec2 v_uv;
out vec4 v_color;

void main() {
    gl_Position = u_proj * u_view * u_model * vec4(a_pos, 1.0);
    v_uv = a_uv.xy;
    v_color = a_color;
}
