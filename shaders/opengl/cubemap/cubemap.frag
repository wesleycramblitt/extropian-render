#version 330 core
uniform samplerCube u_skybox;
in vec3 v_world_pos;
out vec4 frag_color;
void main() { frag_color = texture(u_skybox, v_world_pos); }
