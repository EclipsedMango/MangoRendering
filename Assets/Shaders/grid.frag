#version 460 core

layout(std140, binding = 0) uniform CameraData {
    mat4 u_View;
    mat4 u_Projection;
};

uniform vec3 u_CameraPos;
uniform vec2 u_ScreenSize;
uniform float u_GridSpacing = 1.0;
uniform float u_FadeDistance = 80.0;

out vec4 FragColor;

float GridLine(float coord, float spacing) {
    float f = abs(fract(coord / spacing - 0.5) - 0.5) / fwidth(coord / spacing);
    return 1.0 - clamp(f, 0.0, 1.0);
}

void main() {
    vec2 ndc = (gl_FragCoord.xy / u_ScreenSize) * 2.0 - 1.0;

    mat4 invViewProj = inverse(u_Projection * u_View);

    vec4 worldNear = invViewProj * vec4(ndc, -1.0, 1.0);
    vec4 worldFar = invViewProj * vec4(ndc,  1.0, 1.0);
    worldNear /= worldNear.w;
    worldFar /= worldFar.w;

    float t = -worldNear.y / (worldFar.y - worldNear.y);
    if (t <= 0.0) discard;

    vec3 worldPos = worldNear.xyz + t * (worldFar.xyz - worldNear.xyz);

    float dist = length(worldPos.xz - u_CameraPos.xz);
    float fade = 1.0 - clamp(dist / u_FadeDistance, 0.0, 1.0);
    fade = fade * fade;
    if (fade < 0.001) discard;

    float minor = max(GridLine(worldPos.x, u_GridSpacing), GridLine(worldPos.z, u_GridSpacing));
    float major = max(GridLine(worldPos.x, u_GridSpacing * 10.0), GridLine(worldPos.z, u_GridSpacing * 10.0));

    float xAxis = GridLine(worldPos.z, u_GridSpacing * 0.5) * (1.0 - clamp(abs(worldPos.x) - 0.02, 0.0, 1.0));
    float zAxis = GridLine(worldPos.x, u_GridSpacing * 0.5) * (1.0 - clamp(abs(worldPos.z) - 0.02, 0.0, 1.0));

    vec3 color = vec3(0.25);
    color = mix(color, vec3(0.45), major);
    color = mix(color, vec3(0.2, 0.2, 1.0), clamp(zAxis, 0.0, 1.0));
    color = mix(color, vec3(1.0, 0.2, 0.2), clamp(xAxis, 0.0, 1.0));

    float alpha = max(minor * 0.55, major) * fade;

    vec4 clipPos = u_Projection * u_View * vec4(worldPos, 1.0);
    gl_FragDepth = (clipPos.z / clipPos.w) * 0.5 + 0.5;

    FragColor = vec4(color, alpha);
}