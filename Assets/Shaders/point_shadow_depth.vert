#version 460 core

layout (location = 0) in vec3 a_Position;
layout (location = 2) in vec2 a_Uv;

uniform mat4 u_Model;

out vec3 v_WorldPos;
out vec2 v_Uv;

void main() {
    v_Uv = a_Uv;
    vec4 world = u_Model * vec4(a_Position, 1.0);
    v_WorldPos = world.xyz;
    gl_Position = world; // raw world pos, GS will project
}