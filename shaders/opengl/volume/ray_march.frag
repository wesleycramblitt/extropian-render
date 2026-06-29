#version 330 core
uniform sampler3D u_volume; uniform ivec3 u_grid_dims; uniform float u_absorption;
out vec4 frag_color;
void main() { frag_color = vec4(0.2, 0.4, 0.8, 0.5); }
