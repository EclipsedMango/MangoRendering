
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

    void SetYaw(float yaw);
    void SetPitch(float pitch);

    void SetAsGameCamera(const bool val) { m_isGameCamera = val; }

    void SetViewMatrixOverride(const glm::mat4& viewMatrix);
    void SetProjectionMatrixOverride(const glm::mat4& projMatrix);
    void ClearViewMatrixOverride();
    void ClearProjectionMatrixOverride();

    [[nodiscard]] std::string GetNodeType() const override { return "CameraNode3d"; }

    [[nodiscard]] glm::mat4 GetViewMatrix() const;
    [[nodiscard]] glm::mat4 GetProjectionMatrix() const;

    [[nodiscard]] glm::vec3 GetFront() const;
    [[nodiscard]] glm::vec3 GetRight() const;
    [[nodiscard]] glm::vec3 GetUp() const;

    [[nodiscard]] float GetNearPlane() const { return m_nearPlane; }
    [[nodiscard]] float GetFarPlane() const { return m_farPlane; }
    [[nodiscard]] float GetAspectRatio() const { return m_aspectRatio; }

    [[nodiscard]] float GetYaw() const;
    [[nodiscard]] float GetPitch() const;

    [[nodiscard]] float GetFov() const { return glm::radians(m_fov); }
    [[nodiscard]] bool IsGameCamera() const { return m_isGameCamera; }

    void Rotate(float yawDelta, float pitchDelta);

private:
    void SetYawPitch(float yaw, float pitch);

    [[nodiscard]] glm::vec3 GetLocalFront() const;
    [[nodiscard]] glm::vec3 TransformLocalDirection(const glm::vec3& localDir) const;

    static glm::quat RotationFromYawPitch(float yaw, float pitch);

    glm::vec3 m_worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 m_projOverride{};
    bool m_hasProjectionOverride = false;

    glm::mat4 m_viewOverride{};
    bool m_hasViewOverride = false;

    bool m_isGameCamera = false;

    float m_fov = 60.0f;
    float m_aspectRatio = 16.0f / 9.0f;
    float m_nearPlane = 0.01f;
    float m_farPlane = 300.0f;
};


#endif //MANGORENDERING_CAMERANODE3D_H