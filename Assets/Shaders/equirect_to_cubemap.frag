#version 460 core

in  vec3 v_LocalPos;
out vec4 FragColor;

uniform sampler2D u_EquirectMap;

const vec2 INV_ATAN = vec2(0.1591, 0.3183); // 1/(2pi), 1/pi

vec2 SampleSphericalMap(vec3 dir) {
    vec2 uv = vec2(atan(dir.z, dir.x), asin(dir.y));
    uv *= INV_ATAN;
    uv += 0.5;
    return uv;
}

void main() {
    vec2 uv = SampleSphericalMap(normalize(v_LocalPos));
    FragColor = vec4(texture(u_EquirectMap, uv).rgb, 1.0);
}