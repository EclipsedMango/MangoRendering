#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

struct DirectionalLight {
    vec4 direction;
    vec4 color;
};

layout (std140, binding = 1) uniform GlobalLightData {
    ivec4 u_LightCounts;
    DirectionalLight u_DirLights[4];
};

struct PointLight {
    vec4 position;
    vec4 color;
    vec4 attenuation;
};

layout (std430, binding = 2) readonly buffer PointLightBuffer {
    PointLight pointLights[];
};

struct SpotLight {
    vec4 position;
    vec4 direction;
    vec4 color;
    vec4 params;
};

layout (std430, binding = 3) readonly buffer SpotLightBuffer {
    SpotLight spotLights[];
};

layout (std430, binding = 5) readonly buffer LightIndexBuffer {
    uint globalLightIndexList[];
};

struct LightGrid {
    uint offset;
    uint pointCount;
    uint spotCount;
    uint pad;
};

layout (std430, binding = 6) readonly buffer LightGridBuffer {
    LightGrid lightGrid[];
};

vec3 Heatmap(float t) {
    t = clamp(t, 0.0, 1.0);
    vec3 cold = vec3(0.0, 0.0, 1.0);
    vec3 mid  = vec3(0.0, 1.0, 0.0);
    vec3 hot  = vec3(1.0, 0.0, 0.0);
    if (t < 0.5) return mix(cold, mid, t * 2.0);
    return mix(mid, hot, (t - 0.5) * 2.0);
}

vec3 ACESFilmic(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

vec3 CalculateLighting(vec3 norm, vec3 fragPos, LightGrid grid) {
    vec3 totalLighting = vec3(0.0);

    for (int i = 0; i < u_LightCounts.x; i++) {
        vec3 lightDir = normalize(-u_DirLights[i].direction.xyz);
        float diff = max(dot(norm, lightDir), 0.0);
        totalLighting += diff * u_DirLights[i].color.rgb * u_DirLights[i].color.w;
    }

    for (uint i = 0; i < grid.pointCount; i++) {
        uint lightIndex = globalLightIndexList[grid.offset + i];
        PointLight light = pointLights[lightIndex];

        vec3 lightDir = normalize(light.position.xyz - fragPos);
        float distance = length(light.position.xyz - fragPos);

        if (distance > light.position.w) continue;

        float diff = max(dot(norm, lightDir), 0.0);
        float attenuation = 1.0 / (light.attenuation.x + light.attenuation.y * distance + light.attenuation.z * (distance * distance));
        totalLighting += diff * light.color.rgb * light.color.w * attenuation;
    }

    for (uint i = 0; i < grid.spotCount; i++) {
        uint lightIndex = globalLightIndexList[grid.offset + grid.pointCount + i];
        SpotLight light = spotLights[lightIndex];

        vec3 lightDir = normalize(light.position.xyz - fragPos);
        float distance = length(light.position.xyz - fragPos);

        if (distance > light.position.w) continue;

        float diff = max(dot(norm, lightDir), 0.0);
        float attenuation = 1.0 / (1.0 + light.params.z * distance + light.params.w * (distance * distance));

        float theta = dot(lightDir, normalize(-light.direction.xyz));
        float epsilon = light.params.x - light.params.y;
        float intensityFactor = clamp((theta - light.params.y) / epsilon, 0.0, 1.0);

        totalLighting += diff * light.color.rgb * light.color.w * attenuation * intensityFactor;
    }

    return totalLighting;
}

#endif