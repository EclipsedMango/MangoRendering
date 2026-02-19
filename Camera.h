#ifndef MANGORENDERING_CAMERA_H
#define MANGORENDERING_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    Camera(glm::vec3 position, float fov, float aspectRatio, float nearPlane, float farPlane);

    [[nodiscard]] glm::mat4 GetViewMatrix() const;
    [[nodiscard]] glm::mat4 GetProjectionMatrix() const;

    void SetPosition(const glm::vec3 position) { m_position = position; }
    void SetAspectRatio(const float aspectRatio) { m_aspectRatio = aspectRatio; }

    [[nodiscard]] glm::vec3 GetPosition() const { return m_position; }
    [[nodiscard]] glm::vec3 GetFront() const { return m_front; }
    [[nodiscard]] glm::vec3 GetRight() const { return m_right; }
    [[nodiscard]] glm::vec3 GetUp() const { return m_up; }
    [[nodiscard]] float GetFov() const { return m_fov; }

    void SetYaw(const float yaw) { m_yaw = yaw; UpdateVectors(); }
    void SetPitch(const float pitch) { m_pitch = pitch; UpdateVectors(); }
    [[nodiscard]] float GetYaw() const { return m_yaw; }
    [[nodiscard]] float GetPitch() const { return m_pitch; }

    void Rotate(float yawDelta, float pitchDelta);
    void Move(glm::vec3 delta);

private:
    void UpdateVectors();

    glm::vec3 m_position;
    glm::vec3 m_front{};
    glm::vec3 m_right{};
    glm::vec3 m_up{};
    glm::vec3 m_worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float m_yaw = -90.0f;
    float m_pitch = 0.0f;
    float m_fov;
    float m_aspectRatio;
    float m_nearPlane;
    float m_farPlane;
};


#endif //MANGORENDERING_CAMERA_H