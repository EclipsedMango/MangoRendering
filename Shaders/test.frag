#version 460 core

in vec3 v_Normal;
in vec2 v_TexCoord;

out vec4 FragColor;

uniform sampler2D u_Textures[16];

layout (std140, binding = 0) uniform CameraData {
    mat4 u_View;
    mat4 u_Projection;
};

struct Light {
    vec4 direction;       // 16 bytes
    vec4 colorIntensity;  // 16 bytes
};

layout (std140, binding = 1) uniform LightData {
    ivec4 u_LightInfo;    // 16 bytes (x = count)
    Light u_Lights[4];    // Array of structs matches loop order
};
void main() {
    vec3 norm = normalize(v_Normal);
    vec3 ambient = vec3(0.1);
    vec3 lighting = ambient;

    for (int i = 0; i < u_LightInfo.x; i++) {
        vec3 lightDir = normalize(-u_Lights[i].direction.xyz);
        float diff = max(dot(norm, lightDir), 0.0);

        vec3 lightColor = u_Lights[i].colorIntensity.xyz;
        float intensity = u_Lights[i].colorIntensity.w;

        lighting += diff * lightColor * intensity;
    }

    lighting = clamp(lighting, 0.0, 1.0);
    vec4 texColor = texture(u_Textures[0], v_TexCoord);
    FragColor = vec4(lighting * texColor.rgb, texColor.a);
}