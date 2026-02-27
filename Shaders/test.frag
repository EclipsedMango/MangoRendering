#version 460 core

in vec3 v_Normal;
in vec2 v_TexCoord;
in vec3 v_FragPos;

out vec4 FragColor;

uniform sampler2D u_Textures[16];

uniform float u_ZNear;
uniform float u_ZFar;
uniform vec2 u_ScreenSize;

layout (std140, binding = 0) uniform CameraData {
    mat4 u_View;
    mat4 u_Projection;
};

struct DirectionalLight {
    vec4 direction;
    vec4 color;
};

layout (std140, binding = 1) uniform GlobalLightData {
    ivec4 u_LightCounts; // x = dir, y = point, z = spot
    DirectionalLight u_DirLights[4];
};

struct PointLight {
    vec4 position;    // w = radius
    vec4 color;       // w = intensity
    vec4 attenuation; // x = constant, y = linear, z = quadratic
};

layout (std430, binding = 2) readonly buffer PointLightBuffer {
    PointLight pointLights[];
};

struct SpotLight {
    vec4 position;    // w = radius
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
    vec3 totalLighting = vec3(0.0);

    for (int i = 0; i < u_LightCounts.x; i++) {
        vec3 lightDir = normalize(-u_DirLights[i].direction.xyz);
        float diff = max(dot(norm, lightDir), 0.0);

        vec3 color = u_DirLights[i].color.rgb;
        float intensity = u_DirLights[i].color.w;

        totalLighting += diff * color * intensity;
    }

    for (uint i = 0; i < grid.pointCount; i++) {
        uint lightIndex = globalLightIndexList[grid.offset + i];
        PointLight light = pointLights[lightIndex];

        vec3 lightDir = normalize(light.position.xyz - v_FragPos);
        float distance = length(light.position.xyz - v_FragPos);

        if (distance > light.position.w) {
            continue;
        }

        float diff = max(dot(norm, lightDir), 0.0);

        float constant = light.attenuation.x;
        float linear = light.attenuation.y;
        float quadratic = light.attenuation.z;
        float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));

        vec3 color = light.color.rgb;
        float intensity = light.color.w;

        totalLighting += diff * color * intensity * attenuation;
    }

    for (uint i = 0; i < grid.spotCount; i++) {
        uint lightIndex = globalLightIndexList[grid.offset + grid.pointCount + i];
        SpotLight light = spotLights[lightIndex];

        vec3 lightDir = normalize(light.position.xyz - v_FragPos);
        float distance = length(light.position.xyz - v_FragPos);

        if (distance > light.position.w) {
            continue;
        }

        float diff = max(dot(norm, lightDir), 0.0);

        float linear = light.params.z;
        float quadratic = light.params.w;
        float attenuation = 1.0 / (1.0 + linear * distance + quadratic * (distance * distance));

        float theta = dot(lightDir, normalize(-light.direction.xyz));
        float cutOff = light.params.x;
        float outerCutOff = light.params.y;
        float epsilon = cutOff - outerCutOff;
        float intensityFactor = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);

        vec3 color = light.color.rgb;
        float intensity = light.color.w;

        totalLighting += diff * color * intensity * attenuation * intensityFactor;
    }

    vec3 ambient = vec3(0.05);
    totalLighting += ambient;

    totalLighting = totalLighting / (totalLighting + vec3(1.0));

    vec4 texColor = texture(u_Textures[0], v_TexCoord);
    FragColor = vec4(totalLighting * texColor.rgb, texColor.a);

    // debug to check, does tell much but i think its working
//    float heat = float(grid.pointCount + grid.spotCount);
//    FragColor = vec4(min(heat, 1.0), 0.0, 1.0 - min(heat, 1.0), 1.0);
}