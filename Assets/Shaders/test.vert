#version 460 core

invariant gl_Position;

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoord;
layout (location = 3) in vec4 a_Tangent;

layout (std140, binding = 0) uniform CameraData {
    mat4 u_View;
    mat4 u_Projection;
};

uniform mat4 u_NormalMatrix;
uniform mat4 u_Model;

out vec3 v_Normal;
out vec2 v_TexCoord;
out vec3 v_FragPos;
out mat3 v_TBN;

void main() {
    vec4 worldPosition = u_Model * vec4(a_Position, 1.0);
    v_FragPos = worldPosition.xyz;

    gl_Position = u_Projection * u_View * worldPosition;

    vec3 N = normalize(mat3(u_NormalMatrix) * a_Normal);
    vec3 T = normalize(mat3(u_NormalMatrix) * a_Tangent.xyz);
    T = normalize(T - dot(T, N) * N); // re-orthogonalize against N
    vec3 B = cross(N, T) * a_Tangent.w; // w handles mirrored UVs

    v_TBN = mat3(T, B, N);
    v_Normal = N;
    v_TexCoord = a_TexCoord;
}