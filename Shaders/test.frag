#version 460 core

in vec3 v_Normal;
in vec2 v_TexCoord;
in vec3 v_FragPos;

out vec4 FragColor;

uniform sampler2D u_Textures[16];

// these must match cpp padding
struct DirectionalLight {
    vec4 direction;      // w = unused
    vec4 color;          // w = intensity
};

struct PointLight {
    vec4 position;       // w = unused
    vec4 color;          // w = intensity
    vec4 attenuation;    // x=constant, y=linear, z=quadratic
};

struct SpotLight {
    vec4 position;       // w = unused
    vec4 direction;      // w = unused
    vec4 color;          // w = intensity
    vec4 params;         // x=cutOff, y=outerCutOff, z=linear, w=quadratic
};

layout (std140, binding = 1) uniform GlobalLightData {
    ivec4 u_LightCounts; // x=dir, y=point, z=spot
    DirectionalLight u_DirLights[4];
};

layout (std430, binding = 2) readonly buffer PointLightBuffer {
    PointLight pointLights[];
};

layout (std430, binding = 3) readonly buffer SpotLightBuffer {
    SpotLight spotLights[];
};

void main() {
    vec3 norm = normalize(v_Normal);
    vec3 viewDir = normalize(-v_FragPos);

    vec3 totalLighting = vec3(0.0);

    // directional lights
    for (int i = 0; i < u_LightCounts.x; i++) {
        vec3 lightDir = normalize(-u_DirLights[i].direction.xyz);

        // diffuse
        float diff = max(dot(norm, lightDir), 0.0);

        vec3 color = u_DirLights[i].color.rgb;
        float intensity = u_DirLights[i].color.w;

        totalLighting += diff * color * intensity;
    }

    // point lights
    for (int i = 0; i < u_LightCounts.y; i++) {
        PointLight light = pointLights[i];

        vec3 lightDir = normalize(light.position.xyz - v_FragPos);
        float diff = max(dot(norm, lightDir), 0.0);

        // attenuation
        float distance = length(light.position.xyz - v_FragPos);
        float constant = light.attenuation.x;
        float linear = light.attenuation.y;
        float quadratic = light.attenuation.z;
        float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));

        vec3 color = light.color.rgb;
        float intensity = light.color.w;

        totalLighting += diff * color * intensity * attenuation;
    }

    // spot lights
    for (int i = 0; i < u_LightCounts.z; i++) {
        SpotLight light = spotLights[i];

        vec3 lightDir = normalize(light.position.xyz - v_FragPos);
        float diff = max(dot(norm, lightDir), 0.0);

        // attenuation
        float distance = length(light.position.xyz - v_FragPos);
        float linear = light.params.z;
        float quadratic = light.params.w;
        float attenuation = 1.0 / (1.0f + linear * distance + quadratic * (distance * distance));

        // spotlight intensity with soft edges
        float theta = dot(lightDir, normalize(-light.direction.xyz));
        float cutOff = light.params.x;
        float outerCutOff = light.params.y;
        float epsilon = cutOff - outerCutOff;
        float intensityFactor = clamp((theta - outerCutOff) / epsilon, 0.0, 1.0);

        vec3 color = light.color.rgb;
        float intensity = light.color.w;

        totalLighting += diff * color * intensity * attenuation * intensityFactor;
    }

    vec3 ambient = vec3(0.2);
    totalLighting += ambient;

    // simple tone mapping prevents burning out to white too fast
    totalLighting = totalLighting / (totalLighting + vec3(1.0));

    vec4 texColor = texture(u_Textures[0], v_TexCoord);
    FragColor = vec4(totalLighting * texColor.rgb, texColor.a);
}