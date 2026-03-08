#version 460 core
#include "lighting.glsl"

in vec3 v_Normal;
in vec2 v_TexCoord;
in vec3 v_FragPos;

out vec4 FragColor;

// material
uniform vec4 u_AlbedoColor;
uniform float u_MetallicValue;
uniform float u_RoughnessValue;
uniform float u_AOStrength;
uniform float u_NormalStrength;
uniform float u_EmissionStrength;
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
uniform bool u_HasAmbientOcclusion;
uniform bool u_HasEmissive;
uniform bool u_HasDisplacement;

uniform float u_ZNear;
uniform float u_ZFar;
uniform vec2 u_ScreenSize;
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
    vec3 norm = normalize(v_Normal);
    float viewDepth = -(u_View * vec4(v_FragPos, 1.0)).z;

    uint x = uint(gl_FragCoord.x / (u_ScreenSize.x / float(GRID_SIZE_X)));
    uint y = uint(gl_FragCoord.y / (u_ScreenSize.y / float(GRID_SIZE_Y)));

    float zLog = log(viewDepth / u_ZNear);
    float zScale = float(GRID_SIZE_Z) / log(u_ZFar / u_ZNear);
    uint z = uint(floor(zLog * zScale));

    x = clamp(x, 0u, GRID_SIZE_X - 1u);
    y = clamp(y, 0u, GRID_SIZE_Y - 1u);
    z = clamp(z, 0u, GRID_SIZE_Z - 1u);

    uint clusterIndex = x + y * GRID_SIZE_X + z * GRID_SIZE_X * GRID_SIZE_Y;
    LightGrid grid = lightGrid[clusterIndex];

    if (u_DebugMode == 1) {
        FragColor = vec4(norm * 0.5 + 0.5, 1.0);
        return;
    }

    if (u_DebugMode == 2) {
        uint totalLights = grid.pointCount + grid.spotCount;
        FragColor = totalLights == 0 ? vec4(0.0, 0.0, 0.1, 1.0) : vec4(Heatmap(float(totalLights) / 8.0), 1.0);
        return;
    }

    if (u_DebugMode == 3) {
        FragColor = vec4(Heatmap(float(z) / float(GRID_SIZE_Z)), 1.0);
        return;
    }

    if (u_DebugMode == 4) {
        FragColor = vec4(float(x) / float(GRID_SIZE_X), float(y) / float(GRID_SIZE_Y), 0.5, 1.0);
        return;
    }

    if (u_DebugMode == 5) {
        vec2 uv = gl_FragCoord.xy / u_ScreenSize;
        float depth = texture(u_ShadowMap, vec3(uv, float(u_DebugCascade))).r;
        FragColor = vec4(vec3(depth), 1.0);
        return;
    }

    if (u_DebugMode == 6) {
        int cascade = SelectCascade(viewDepth);
        FragColor = vec4(Heatmap(float(cascade) / float(u_CascadeCount - 1)), 1.0);
        return;
    }

    if (u_DebugMode == 7) {
        int cascade = SelectCascade(viewDepth);
        vec4 lightSpace = u_LightSpaceMatrix[cascade] * vec4(v_FragPos, 1.0);
        vec3 projCoords = lightSpace.xyz / lightSpace.w;
        projCoords = projCoords * 0.5 + 0.5;
        FragColor = vec4(projCoords, 1.0);
        return;
    }

    if (u_DebugMode == 8) {
        float shadowFactor = ShadowWithBlend(v_FragPos, viewDepth, norm, normalize(-u_DirLights[0].direction.xyz));
        FragColor = vec4(vec3(shadowFactor), 1.0);
        return;
    }

    vec2 uv = v_TexCoord * u_UVScale + u_UVOffset;
    vec4 albedo = u_HasDiffuse ? texture(u_Diffuse, uv) * u_AlbedoColor : u_AlbedoColor;

    vec3 totalLighting = CalculateLighting(norm, v_FragPos, viewDepth, grid);

    vec3 ambient = vec3(0.05) * u_AOStrength;
    totalLighting += ambient;

    vec3 emission = u_HasEmissive ? texture(u_Emissive, uv).rgb * u_EmissionColor * u_EmissionStrength : u_EmissionColor * u_EmissionStrength;
    totalLighting += emission;

    float exposure = 2.0;
    totalLighting = ACESFilmic(totalLighting * exposure);

    FragColor = vec4(totalLighting * albedo.rgb, albedo.a);
}