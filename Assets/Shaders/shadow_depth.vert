#version 460 core

layout (location = 0) in vec3 a_Position;
layout (location = 2) in vec2 a_Uv;

uniform mat4 u_LightSpaceMatrix;
uniform mat4 u_Model;

out vec2 v_Uv;

void main() {
    v_Uv = a_Uv;
    gl_Position = u_LightSpaceMatrix * u_Model * vec4(a_Position, 1.0);
}