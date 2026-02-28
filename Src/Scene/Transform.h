
#ifndef MANGORENDERING_TRANSFORM_H
#define MANGORENDERING_TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Transform {
public:
    glm::vec3 Position = {0, 0, 0};
    glm::vec3 Rotation = {0, 0, 0}; // euler angles in degrees
    glm::vec3 Scale    = {1, 1, 1};

    [[nodiscard]] glm::mat4 GetModelMatrix() const {
        glm::mat4 mat = glm::translate(glm::mat4(1.0f), Position);
        mat = glm::rotate(mat, glm::radians(Rotation.x), {1, 0, 0});
        mat = glm::rotate(mat, glm::radians(Rotation.y), {0, 1, 0});
        mat = glm::rotate(mat, glm::radians(Rotation.z), {0, 0, 1});
        mat = glm::scale(mat, Scale);
        return mat;
    }
};


#endif //MANGORENDERING_TRANSFORM_H