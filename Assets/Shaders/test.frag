#version 460 core
#include "lighting.glsl"

in vec3 v_Normal;
in vec2 v_TexCoord;
in vec3 v_FragPos;
in mat3 v_TBN;

out vec4 FragColor;

// material
uniform vec4 u_AlbedoColor;
uniform float u_MetallicValue;
uniform float u_RoughnessValue;
uniform float u_AOStrength;
uniform float u_NormalStrength;
uniform float u_EmissionStrength;
uniform float u_AlphaScissorThreshold;
uniform vec3 u_EmissionColor;
uniform float u_DisplacementScale;
uniform vec2 u_UVScale;
uniform vec2 u_UVOffset;

uniform sampler2D u_Diffuse;
uniform sampler2D u_Normal;
uniform sampler2D u_Metallic;
uniform sampler2D u_Roughness;
uniform sampler2D u_AmbientOcclusion;
uniform sampler2D u_Emissive;
uniform sampler2D u_Displacement;

uniform bool u_HasDiffuse;
uniform bool u_HasNormal;
uniform bool u_HasMetallic;
uniform bool u_HasRoughness;
uniform bool u_HasMetallicRoughnessPacked;
uniform bool u_HasAmbientOcclusion;
uniform bool u_HasEmissive;
uniform bool u_HasDisplacement;
uniform bool u_AlphaScissor;
uniform bool u_DoubleSided;

uniform float u_ZNear;
uniform float u_ZFar;
uniform vec2 u_ScreenSize;
uniform vec3 u_CameraPos;
uniform int u_DebugMode;
uniform int u_DebugCascade;

layout (std140, binding = 0) uniform CameraData {
    mat4 u_View;
    mat4 u_Projection;
};

const uint GRID_SIZE_X = 16;
const uint GRID_SIZE_Y = 9;
const uint GRID_SIZE_Z = 24;

void main() {
    vec3 V = normalize(u_CameraPos - v_FragPos);
    vec2 uv = v_TexCoord * u_UVScale + u_UVOffset;

    vec3 norm;
    if (u_HasNormal) {
        vec3 normalSample = texture(u_Normal, uv).rgb * 2.0 - 1.0;
        normalSample.xy *= u_NormalStrength;
        norm = normalize(v_TBN * normalSample);
    } else {
        norm = normalize(v_Normal);
    }

    if (u_HasDisplacement) {
        vec3 viewDirTangent = normalize(transpose(v_TBN) * V);
        float height = texture(u_Displacement, uv).r;

        vec2 parallaxOffset = viewDirTangent.xy * (height * u_DisplacementScale);
        uv -= parallaxOffset;
    }

    // correct lighting for two sided materials (foliage/cards)
    if (u_DoubleSided && !gl_FrontFacing) {
        norm = -norm;
    }

    float viewDepth = -(u_View * vec4(v_FragPos, 1.0)).z;

    uint x = uint(gl_FragCoord.x * float(GRID_SIZE_X) / u_ScreenSize.x);
    uint y = uint(gl_FragCoord.y * float(GRID_SIZE_Y) / u_ScreenSize.y);

    float zLog = log(viewDepth / u_ZNear);
    float zScale = float(GRID_SIZE_Z) / log(u_ZFar / u_ZNear);
    int zInt = int(floor(zLog * zScale));
    zInt = clamp(zInt, 0, int(GRID_SIZE_Z) - 1);

    uint z = uint(zInt);
    x = clamp(x, 0u, GRID_SIZE_X - 1u);
    y = clamp(y, 0u, GRID_SIZE_Y - 1u);

    uint clusterIndex = x + y * GRID_SIZE_X + z * GRID_SIZE_X * GRID_SIZE_Y;
    LightGrid grid = lightGrid[clusterIndex];

    vec4  albedo = u_HasDiffuse ? texture(u_Diffuse, uv) * u_AlbedoColor : u_AlbedoColor;
    if (u_AlphaScissor && albedo.a < u_AlphaScissorThreshold) {
        discard;
    }

    float ao = u_HasAmbientOcclusion ? texture(u_AmbientOcclusion, uv).r * u_AOStrength : u_AOStrength;

    float metallic  = 0.0;
    float roughness = 0.0;

    if (u_HasMetallicRoughnessPacked) {
        vec3 mr = texture(u_Metallic, uv).rgb; // both textures are the same binding
        roughness = mr.g * u_RoughnessValue;
        metallic = mr.b * u_MetallicValue;
    } else {
        metallic = u_HasMetallic ? texture(u_Metallic, uv).r * u_MetallicValue : u_MetallicValue;
        roughness = u_HasRoughness ? texture(u_Roughness, uv).r * u_RoughnessValue : u_RoughnessValue;
    }

    // clamp roughness to avoid precision issues at zero
    roughness = max(roughness, 0.045);

    vec3 totalLighting = CalculateLighting(norm, v_FragPos, viewDepth, grid, V, albedo.rgb, metallic, roughness, ao);

    vec3 emission = u_HasEmissive ? texture(u_Emissive, uv).rgb * u_EmissionColor * u_EmissionStrength : u_EmissionColor * u_EmissionStrength;
    totalLighting += emission;

    float exposure = 0.5;
    totalLighting = ACESFilmic(totalLighting * exposure);

    FragColor = vec4(totalLighting, albedo.a);
}