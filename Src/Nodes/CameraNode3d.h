
#ifndef MANGORENDERING_CAMERANODE3D_H
#define MANGORENDERING_CAMERANODE3D_H
#include "Node3d.h"
#include "glm/glm.hpp"

class CameraNode3d : public Node3d {
public:
    CameraNode3d();
    CameraNode3d(glm::vec3 position, float fov, float aspectRatio);
    ~CameraNode3d() override = default;

    std::unique_ptr<Node3d> Clone() override;

    void SetNearPlane(const float nearPlane) { m_nearPlane = nearPlane; }
    void SetFarPlane(const float farPlane) { m_farPlane = farPlane; }
    void SetAspectRatio(const float aspectRatio) { m_aspectRatio = aspectRatio; }
    void SetYaw(const float yaw) { m_yaw = yaw; UpdateVectors(); }
    void SetPitch(const float pitch) { m_pitch = pitch; UpdateVectors(); }
    void SetAsGameCamera(const bool val) { m_isGameCamera = val; }
    void SetViewMatrixOverride(const glm::mat4& viewMatrix);
    void ClearViewMatrixOverride();

    void Process(float deltaTime) override {}

    [[nodiscard]] std::string GetNodeType() const override { return "CameraNode3d"; }
    [[nodiscard]] glm::mat4 GetViewMatrix() const;
    [[nodiscard]] glm::mat4 GetProjectionMatrix() const;

    [[nodiscard]] glm::vec3 GetFront() const { return m_front; }
    [[nodiscard]] glm::vec3 GetRight() const { return m_right; }
    [[nodiscard]] glm::vec3 GetUp() const { return m_up; }
    [[nodiscard]] float GetNearPlane() const { return m_nearPlane; }
    [[nodiscard]] float GetFarPlane() const { return m_farPlane; }
    [[nodiscard]] float GetAspectRatio() const { return m_aspectRatio; }
    [[nodiscard]] float GetYaw() const { return m_yaw; }
    [[nodiscard]] float GetPitch() const { return m_pitch; }
    [[nodiscard]] float GetFov() const { return glm::radians(m_fov); }
    [[nodiscard]] bool IsGameCamera() const { return m_isGameCamera; }

    void Rotate(float yawDelta, float pitchDelta);

private:
    void UpdateVectors();

    glm::vec3 m_front{};
    glm::vec3 m_right{};
    glm::vec3 m_up{};
    glm::vec3 m_worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 m_viewOverride{};
    bool m_hasViewOverride = false;
    bool m_isGameCamera = false;

    float m_yaw = -90.0f;
    float m_pitch = 0.0f;
    float m_fov;
    float m_aspectRatio;
    float m_nearPlane = 0.1;
    float m_farPlane = 200.0;
};


#endif //MANGORENDERING_CAMERANODE3D_H