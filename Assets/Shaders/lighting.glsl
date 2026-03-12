#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

uniform sampler2DArray u_ShadowMap;
uniform mat4 u_LightSpaceMatrix[4];
uniform float u_CascadeSplits[4];
uniform float u_CascadeWorldUnits[4];
uniform int u_CascadeCount;
uniform samplerCubeArray u_PointShadowMap;

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

struct PointShadowMeta {
    uint  slot;
    float farPlane;
    float bias;
    float pad;
};

layout(std430, binding = 8) readonly buffer PointShadowBuffer {
    PointShadowMeta pointShadowMeta[];
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

int SelectCascade(float depthVS) {
    for (int i = 0; i < u_CascadeCount; ++i) {
        if (depthVS < u_CascadeSplits[i])
        return i;
    }
    return u_CascadeCount - 1;
}

// no clue how they got these random numbers
const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624,  -0.39906216),
    vec2( 0.94558609,  -0.76890725),
    vec2(-0.09418410,  -0.92938870),
    vec2( 0.34495938,   0.29387760),
    vec2(-0.91588581,   0.45771432),
    vec2(-0.81544232,  -0.87912464),
    vec2(-0.38277543,   0.27676845),
    vec2( 0.97484398,   0.75648379),
    vec2( 0.44323325,  -0.97511554),
    vec2( 0.53742981,  -0.47373420),
    vec2(-0.26496911,  -0.41893023),
    vec2( 0.79197514,   0.19090188),
    vec2(-0.24188840,   0.99706507),
    vec2(-0.81409955,   0.91437590),
    vec2( 0.19984126,   0.78641367),
    vec2( 0.14383161,  -0.14100790)
);

// Simple, fast hash: returns [0,1)
float Hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

mat2 Rotate2D(float a) {
    float c = cos(a), s = sin(a);
    return mat2(c, -s, s, c);
}

float PointShadow(vec3 fragPos, vec3 normal, uint lightIndex, PointLight light) {
    uint slot = pointShadowMeta[lightIndex].slot;
    if (slot == 0xFFFFFFFFu) return 0.0;

    float farPlane = pointShadowMeta[lightIndex].farPlane;
    float biasBase = pointShadowMeta[lightIndex].bias;

    vec3 toFrag = fragPos - light.position.xyz;
    float currentDist = length(toFrag);

    vec3 L = normalize(light.position.xyz - fragPos);
    float cosTheta = clamp(dot(normal, L), 0.0, 1.0);
    float bias = max(biasBase * (1.0 - cosTheta), biasBase * 0.05);

    vec3 dir = normalize(toFrag);
    vec3 up = abs(dir.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 right = normalize(cross(up, dir));
    vec3 tangent = normalize(cross(dir, right));

    float diskRadius = (currentDist / farPlane) * 0.04;
    float angle = Hash12(gl_FragCoord.xy) * 6.28318530718;
    mat2 R = Rotate2D(angle);

    float shadow = 0.0;
    for (int i = 0; i < 16; i++) {
        vec2 offset = (R * poissonDisk[i]) * diskRadius;
        vec3 sampleDir = normalize(toFrag + right * offset.x + tangent * offset.y);
        float closest = texture(u_PointShadowMap, vec4(sampleDir, float(slot))).r * farPlane;
        shadow += (currentDist - bias) > closest ? 1.0 : 0.0;
    }

    return shadow / 16.0;
}

float EdgeFade(float dist, float radius) {
    // fade out over last 20% of the radius
    float fadeStart = radius * 0.8;
    return 1.0 - smoothstep(fadeStart, radius, dist);
}

float ShadowCalculation(vec3 fragPos, int cascade, vec3 normal, vec3 lightDirWS) {
    float texelWU  = u_CascadeWorldUnits[cascade];
    vec3 offsetPos = fragPos + normal * texelWU * 0.1;

    vec4 fragPosLightSpace = u_LightSpaceMatrix[cascade] * vec4(offsetPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float currentDepth = clamp(projCoords.z, 0.0, 1.0);

    float cosTheta = clamp(dot(normal, lightDirWS), 0.0001, 1.0);
    float tanTheta = sqrt(1.0 - cosTheta * cosTheta) / cosTheta;

    float texel = 1.0 / float(textureSize(u_ShadowMap, 0).x);
    float bias = max(1.0 * texel * tanTheta, 0.25 * texel);

    float diskRadius = 2.5 * texel;
    float angle = Hash12(gl_FragCoord.xy) * 6.28318530718;
    mat2 R = Rotate2D(angle);

    float shadow = 0.0;
    for (int i = 0; i < 16; i++) {
        vec2 offset = (R * poissonDisk[i]) * diskRadius;
        float pcfDepth = texture(u_ShadowMap, vec3(projCoords.xy + offset, float(cascade))).r;
        shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
    }

    return shadow / 16.0;
}

float ShadowWithBlend(vec3 fragPos, float fragDepthVS, vec3 normal, vec3 lightDirWS) {
    int cascade = SelectCascade(fragDepthVS);
    float shadow = ShadowCalculation(fragPos, cascade, normal, lightDirWS);

    if (cascade + 1 < u_CascadeCount) {
        float splitDist   = u_CascadeSplits[cascade];
        float blendRange  = splitDist * 0.25;
        float blendFactor = smoothstep(splitDist - blendRange, splitDist, fragDepthVS);

        if (blendFactor > 0.0) {
            float nextShadow = ShadowCalculation(fragPos, cascade + 1, normal, lightDirWS);
            shadow = mix(shadow, nextShadow, blendFactor);
        }
    }
    return shadow;
}

vec3 CalculateLighting(vec3 norm, vec3 fragPos, float fragDepthVS, LightGrid grid) {
    vec3 totalLighting = vec3(0.0);

    for (int i = 0; i < u_LightCounts.x; i++) {
        vec3 L = normalize(-u_DirLights[i].direction.xyz);
        float diff = max(dot(norm, L), 0.0);

        float shadow = ShadowWithBlend(fragPos, fragDepthVS, norm, L);
        totalLighting += (1.0 - shadow) * diff * u_DirLights[i].color.rgb * u_DirLights[i].color.w;
    }

    for (uint i = 0; i < grid.pointCount; i++) {
        uint lightIndex = globalLightIndexList[grid.offset + i];
        PointLight light = pointLights[lightIndex];

        vec3  toLight  = light.position.xyz - fragPos;
        float distance = length(toLight);
        float radius   = light.position.w;

        if (distance > radius) continue;

        vec3 lightDir = toLight / max(distance, 1e-5);
        float diff = max(dot(norm, lightDir), 0.0);

        float attenuation = 1.0 / (light.attenuation.x + light.attenuation.y * distance + light.attenuation.z * (distance * distance));
        float edge = EdgeFade(distance, radius);
        float shadow = PointShadow(fragPos, norm, lightIndex, light);

        totalLighting += (1.0 - shadow) * diff * light.color.rgb * light.color.w * attenuation * edge;
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