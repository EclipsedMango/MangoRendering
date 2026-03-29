
#include "CameraNode3d.h"

REGISTER_NODE_TYPE(CameraNode3d)

CameraNode3d::CameraNode3d() : CameraNode3d(glm::vec3(0.0f), 60.0f, 16.0f / 9.0f) {}

CameraNode3d::CameraNode3d(const glm::vec3 position, const float fov, const float aspectRatio) : m_fov(fov), m_aspectRatio(aspectRatio) {
    SetPosition(position);
    UpdateVectors();
    SetName("CameraNode3d");

    AddProperty("fov",
        [this]() -> PropertyValue { return m_fov; },
        [this](const PropertyValue& v) { m_fov = std::get<float>(v); }
    );
    AddProperty("near_plane",
        [this]() -> PropertyValue { return m_nearPlane; },
        [this](const PropertyValue& v) { SetNearPlane(std::get<float>(v)); }
    );
    AddProperty("far_plane",
        [this]() -> PropertyValue { return m_farPlane; },
        [this](const PropertyValue& v) { SetFarPlane(std::get<float>(v)); }
    );
    AddProperty("yaw",
        [this]() -> PropertyValue { return m_yaw; },
        [this](const PropertyValue& v) { SetYaw(std::get<float>(v)); }
    );
    AddProperty("pitch",
        [this]() -> PropertyValue { return m_pitch; },
        [this](const PropertyValue& v) { SetPitch(std::get<float>(v)); }
    );
}

// TODO: finish this
std::unique_ptr<Node3d> CameraNode3d::Clone() {
    return Node3d::Clone();
}

void CameraNode3d::SetViewMatrixOverride(const glm::mat4 &viewMatrix) {
    m_viewOverride = viewMatrix;
    m_hasViewOverride = true;
}

void CameraNode3d::ClearViewMatrixOverride() {
    m_hasViewOverride = false;
}

glm::mat4 CameraNode3d::GetViewMatrix() const {
    if (m_hasViewOverride) return m_viewOverride;
    return glm::lookAt(GetPosition(), GetPosition() + m_front, m_up);
}

glm::mat4 CameraNode3d::GetProjectionMatrix() const {
    return glm::perspective(glm::radians(m_fov), m_aspectRatio, m_nearPlane, m_farPlane);
}

void CameraNode3d::Rotate(const float yawDelta, const float pitchDelta) {
    m_yaw += yawDelta;
    m_pitch = glm::clamp(m_pitch + pitchDelta, -89.0f, 89.0f);
    UpdateVectors();
}

void CameraNode3d::UpdateVectors() {
    const glm::vec3 front = {
        cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch)),
        sin(glm::radians(m_pitch)),
        sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch))
    };

    m_front = glm::normalize(front);
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up    = glm::normalize(glm::cross(m_right, m_front));
}