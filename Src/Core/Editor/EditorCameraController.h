
#ifndef MANGORENDERING_EDITORCAMERACONTROLLER_H
#define MANGORENDERING_EDITORCAMERACONTROLLER_H


class Window;
class CameraNode3d;

class EditorCameraController {
public:
    EditorCameraController(CameraNode3d* camera, Window* window);

    void Update(float deltaTime, bool viewportHovered);
    void CancelLook();

    void SetMoveSpeedLastFrame(const float moveSpeed) { m_moveSpeedLastFrame = moveSpeed; };

    [[nodiscard]] bool IsLooking() const { return m_rmbLook; }
    [[nodiscard]] float GetMoveSpeed() const { return m_moveSpeed; }
    [[nodiscard]] float GetMoveSpeedLastFrame() const { return m_moveSpeedLastFrame; }

private:
    CameraNode3d* m_camera;
    Window* m_window;
    bool m_rmbLook = false;
    float m_moveSpeed = 10.0f;
    float m_moveSpeedLastFrame = 0.0f;
    float m_mouseSensitivity = 0.08f;
};


#endif //MANGORENDERING_EDITORCAMERACONTROLLER_H