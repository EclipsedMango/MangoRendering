
#ifndef MANGORENDERING_GPULIGHTS_H
#define MANGORENDERING_GPULIGHTS_H

#include <glm/glm.hpp>

// align with std140 layout in shaders
struct GPUDirectionalLight {
    glm::vec4 direction;    // w = unused/padding
    glm::vec4 color;        // w = intensity
};

// align with std430 layout in shaders
struct GPUPointLight {
    glm::vec4 position;     // w = unused
    glm::vec4 color;        // w = intensity
    glm::vec4 attenuation;  // x=constant, y=linear, z=quadratic
};

// align with std430 layout in shaders
struct GPUSpotLight {
    glm::vec4 position;     // w = unused
    glm::vec4 direction;    // w = unused
    glm::vec4 color;        // w = intensity
    glm::vec4 params;       // x=cutOff, y=outerCutOff, z=linear, w=quadratic
};

#endif //MANGORENDERING_GPULIGHTS_H