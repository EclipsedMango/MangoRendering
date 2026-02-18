#version 460 core

in vec2 v_TexCoord;

out vec4 FragColor;

void main() {
    FragColor = vec4(v_TexCoord, 0.0, 1.0);
}