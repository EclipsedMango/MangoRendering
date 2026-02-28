#version 460 core

invariant gl_Position;

layout (location = 0) in vec3 aPos;
// add texture coordinates for doing alpha testing (leaves, etc)

layout(std140, binding = 0) uniform CameraData {
    mat4 view;
    mat4 proj;
};

uniform mat4 u_Model;

void main() {
    vec4 worldPosition = u_Model * vec4(aPos, 1.0);
    gl_Position = proj * view * worldPosition;
}