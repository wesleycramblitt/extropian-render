#version 330 core
layout(location=0) in vec3 a_pos; layout(location=1) in vec3 a_col;
uniform mat4 u_model, u_view, u_proj;
out vec3 v_color;
void main() { gl_Position = u_proj * u_view * u_model * vec4(a_pos,1); v_color=a_col; gl_PointSize=3.0; }
