#version 460 core
out vec4 FragColor;

void main() {
    // this color is ignored because of glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE)
    FragColor = vec4(1.0);
}