#version 460 core

in vec2 v_Uv;

uniform bool u_AlphaScissor;
uniform float u_AlphaScissorThreshold;
uniform bool u_AlphaDitherShadow;

uniform bool u_HasDiffuse;
uniform sampler2D u_Diffuse;

// Simple hash [0,1)
float Hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

void main() {
    float a = 1.0;
    if (u_HasDiffuse) {
        a = texture(u_Diffuse, v_Uv).a;
    }

    if (u_AlphaScissor) {
        if (a < u_AlphaScissorThreshold) {
            discard;
        }

    } else if (u_AlphaDitherShadow) {
        float r = Hash12(gl_FragCoord.xy);
        if (a < r) {
            discard;
        }
    }
}