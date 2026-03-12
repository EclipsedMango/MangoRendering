#version 460 core

in vec3 g_WorldPos;

uniform vec3  u_LightPos;
uniform float u_FarPlane;

void main() {
    gl_FragDepth = length(g_WorldPos - u_LightPos) / u_FarPlane;
}