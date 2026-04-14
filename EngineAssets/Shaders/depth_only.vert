#version 460 core

invariant gl_Position;

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoord;
layout (location = 4) in uvec4 a_Joints;
layout (location = 5) in vec4 a_Weights;

layout(std140, binding = 0) uniform CameraData {
    mat4 view;
    mat4 proj;
};

uniform mat4 u_Model;
uniform bool u_Skinned;
uniform int u_SkinMatrixCount;
layout(std140, binding = 9) uniform SkinMatrices {
    mat4 u_SkinMatrices[128];
};

out vec2 v_TexCoord;

void main() {
    v_TexCoord = aTexCoord;
    mat4 skinMatrix = mat4(1.0);
    if (u_Skinned) {
        int skinCount = max(u_SkinMatrixCount, 1);
        skinMatrix =
            a_Weights.x * u_SkinMatrices[min(int(a_Joints.x), skinCount - 1)] +
            a_Weights.y * u_SkinMatrices[min(int(a_Joints.y), skinCount - 1)] +
            a_Weights.z * u_SkinMatrices[min(int(a_Joints.z), skinCount - 1)] +
            a_Weights.w * u_SkinMatrices[min(int(a_Joints.w), skinCount - 1)];
    }

    vec4 worldPosition = u_Model * (skinMatrix * vec4(aPos, 1.0));
    gl_Position = proj * view * worldPosition;
}
