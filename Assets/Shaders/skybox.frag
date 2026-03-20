#version 460 core

in vec3 v_TexCoords;
out vec4 FragColor;

uniform samplerCube u_Skybox;

vec3 ACESFilmic(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

void main() {
    vec3 hdr = texture(u_Skybox, normalize(v_TexCoords)).rgb;

    hdr *= 0.3;
    vec3 ldr = ACESFilmic(hdr);

    ldr = pow(ldr, vec3(1.0 / 2.2));
    FragColor = vec4(ldr, 1.0);
}