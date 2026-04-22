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
uniform bool u_UseSkinnedVertexBuffer;

struct SkinnedVertex {
    vec4 position;
    vec4 normal;
    vec4 tangent;
};

layout (std430, binding = 11) readonly buffer SkinnedVertexBuffer {
    SkinnedVertex u_SkinnedVertices[];
};

out vec3 v_Normal;
out vec2 v_TexCoord;
out vec3 v_FragPos;
out mat3 v_TBN;

void main() {
    vec4 localPosition = vec4(a_Position, 1.0);
    vec3 localNormal = a_Normal;
    vec3 localTangent = a_Tangent.xyz;
    float tangentSign = a_Tangent.w;

    if (u_UseSkinnedVertexBuffer) {
        SkinnedVertex skinned = u_SkinnedVertices[gl_VertexID];
        localPosition = vec4(skinned.position.xyz, 1.0);
        localNormal = normalize(skinned.normal.xyz);
        localTangent = normalize(skinned.tangent.xyz);
        tangentSign = skinned.tangent.w;
    }

    vec4 worldPosition = u_Model * localPosition;
    v_FragPos = worldPosition.xyz;

    gl_Position = u_Projection * u_View * worldPosition;

    vec3 N = normalize(mat3(u_NormalMatrix) * localNormal);
    vec3 T = normalize(mat3(u_NormalMatrix) * localTangent);
    T = normalize(T - dot(T, N) * N); // re-orthogonalize against N
    vec3 B = cross(N, T) * tangentSign; // w handles mirrored UVs

    v_TBN = mat3(T, B, N);
    v_Normal = N;
    v_TexCoord = a_TexCoord;
}
