#version 460 core

layout (location = 0) in vec3 a_Position;
layout (location = 2) in vec2 a_Uv;
layout (location = 4) in uvec4 a_Joints;
layout (location = 5) in vec4 a_Weights;

uniform mat4 u_Model;
uniform bool u_Skinned;
uniform int u_SkinMatrixCount;
uniform mat4 u_SkinMatrices[128];

out vec3 v_WorldPos;
out vec2 v_Uv;

void main() {
    v_Uv = a_Uv;
    mat4 skinMatrix = mat4(1.0);
    if (u_Skinned) {
        int skinCount = max(u_SkinMatrixCount, 1);
        skinMatrix =
            a_Weights.x * u_SkinMatrices[min(int(a_Joints.x), skinCount - 1)] +
            a_Weights.y * u_SkinMatrices[min(int(a_Joints.y), skinCount - 1)] +
            a_Weights.z * u_SkinMatrices[min(int(a_Joints.z), skinCount - 1)] +
            a_Weights.w * u_SkinMatrices[min(int(a_Joints.w), skinCount - 1)];
    }

    vec4 world = u_Model * (skinMatrix * vec4(a_Position, 1.0));
    v_WorldPos = world.xyz;
    gl_Position = world; // raw world pos, GS will project
}
