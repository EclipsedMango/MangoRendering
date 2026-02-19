
#include "Camera.h"

Camera::Camera(glm::vec3 position, float fov, float aspectRatio, float nearPlane, float farPlane)
    : m_position(position), m_fov(fov), m_aspectRatio(aspectRatio), m_nearPlane(nearPlane), m_farPlane(farPlane) {
    UpdateVectors();
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::GetProjectionMatrix() const {
    return glm::perspective(glm::radians(m_fov), m_aspectRatio, m_nearPlane, m_farPlane);
}

void Camera::Rotate(const float yawDelta, const float pitchDelta) {
    m_yaw += yawDelta;
    m_pitch = glm::clamp(m_pitch + pitchDelta, -89.0f, 89.0f);
    UpdateVectors();
}

void Camera::Move(glm::vec3 delta) {
    m_position += delta;
}

void Camera::UpdateVectors() {
    const glm::vec3 front = {
        cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch)),
        sin(glm::radians(m_pitch)),
        sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch))
    };

    m_front = glm::normalize(front);
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up    = glm::normalize(glm::cross(m_right, m_front));
}