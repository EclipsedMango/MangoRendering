#version 460 core

invariant gl_Position;

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoord;
layout (location = 3) in vec4 a_Tangent;
layout (location = 4) in uvec4 a_Joints;
layout (location = 5) in vec4 a_Weights;

layout (std140, binding = 0) uniform CameraData {
    mat4 u_View;
    mat4 u_Projection;
};

uniform mat4 u_NormalMatrix;
uniform mat4 u_Model;
uniform bool u_Skinned;
uniform int u_SkinMatrixCount;
uniform mat4 u_SkinMatrices[128];

out vec3 v_Normal;
out vec2 v_TexCoord;
out vec3 v_FragPos;
out mat3 v_TBN;

void main() {
    mat4 skinMatrix = mat4(1.0);
    if (u_Skinned) {
        int skinCount = max(u_SkinMatrixCount, 1);
        skinMatrix =
            a_Weights.x * u_SkinMatrices[min(int(a_Joints.x), skinCount - 1)] +
            a_Weights.y * u_SkinMatrices[min(int(a_Joints.y), skinCount - 1)] +
            a_Weights.z * u_SkinMatrices[min(int(a_Joints.z), skinCount - 1)] +
            a_Weights.w * u_SkinMatrices[min(int(a_Joints.w), skinCount - 1)];
    }

    vec4 localPosition = skinMatrix * vec4(a_Position, 1.0);
    vec3 localNormal = normalize(mat3(skinMatrix) * a_Normal);
    vec3 localTangent = normalize(mat3(skinMatrix) * a_Tangent.xyz);

    vec4 worldPosition = u_Model * localPosition;
    v_FragPos = worldPosition.xyz;

    gl_Position = u_Projection * u_View * worldPosition;

    vec3 N = normalize(mat3(u_NormalMatrix) * localNormal);
    vec3 T = normalize(mat3(u_NormalMatrix) * localTangent);
    T = normalize(T - dot(T, N) * N); // re-orthogonalize against N
    vec3 B = cross(N, T) * a_Tangent.w; // w handles mirrored UVs

    v_TBN = mat3(T, B, N);
    v_Normal = N;
    v_TexCoord = a_TexCoord;
}
