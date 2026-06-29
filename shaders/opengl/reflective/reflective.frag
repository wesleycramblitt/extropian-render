#version 330 core
in vec3 v_normal, v_world_pos;
uniform samplerCube u_skybox;
uniform vec3 u_camPos;
out vec4 frag_color;
void main() {
    vec3 I = normalize(v_world_pos - u_camPos);
    vec3 R = reflect(I, normalize(v_normal));
    frag_color = texture(u_skybox, R);
}
