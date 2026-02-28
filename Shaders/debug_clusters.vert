#version 460 core
layout (location = 0) in vec3 a_Position;

struct ClusterAABB {
    vec4 min;
    vec4 max;
};

layout(std430, binding = 4) readonly buffer ClusterBuffer {
    ClusterAABB clusters[];
};

uniform mat4 u_View;
uniform mat4 u_Projection;

uniform uint u_DimX;
uniform uint u_DimY;
uniform uint u_DimZ;

out vec4 v_Color;

void main() {
    ClusterAABB aabb = clusters[gl_InstanceID];

    vec3 minPoint = aabb.min.xyz;
    vec3 maxPoint = aabb.max.xyz;
    vec3 size = maxPoint - minPoint;

    vec3 worldPos = minPoint + (a_Position * size);
    gl_Position = u_Projection * u_View * vec4(worldPos, 1.0);

    uint sliceStride = u_DimX * u_DimY;
    uint tileZ = gl_InstanceID / sliceStride;

    float t = float(tileZ) / float(u_DimZ);

    vec3 nearColor = vec3(0.0, 1.0, 1.0);
    vec3 farColor  = vec3(1.0, 0.0, 1.0);

    vec3 color = mix(nearColor, farColor, t);
    v_Color = vec4(color, 0.01);
}