#ifndef MANGORENDERING_ANIMATIONCONTROLLERNODE3D_H
#define MANGORENDERING_ANIMATIONCONTROLLERNODE3D_H

#include <string>
#include <vector>

#include "Nodes/Node3d.h"

class MeshNode3d;

class AnimationControllerNode3d : public Node3d {
public:
    enum class TargetMode {
        Subtree,
        Scene
    };

    AnimationControllerNode3d();
    ~AnimationControllerNode3d() override;

    [[nodiscard]] std::unique_ptr<Node3d> Clone() override;
    void Process(float deltaTime) override;

    void Play();
    void Pause();
    void Stop();
    void SetSpeed(float speed);
    void SetLoop(bool loop);
    void SetClipName(const std::string& clipName);
    void SetAutoPlay(bool autoPlay);
    void SetTargetMode(TargetMode mode);
    void SetTargetMeshName(const std::string& meshName);

    [[nodiscard]] bool IsPlaying() const { return m_playing; }
    [[nodiscard]] float GetSpeed() const { return m_speed; }
    [[nodiscard]] bool GetLoop() const { return m_loop; }
    [[nodiscard]] bool GetAutoPlay() const { return m_autoPlay; }
    [[nodiscard]] const std::string& GetClipName() const { return m_clipName; }
    [[nodiscard]] TargetMode GetTargetMode() const { return m_targetMode; }
    [[nodiscard]] const std::string& GetTargetMeshName() const { return m_targetMeshName; }
    [[nodiscard]] std::string GetNodeType() const override { return "AnimationControllerNode3d"; }

private:
    void RefreshTargets();
    void ReleaseTargets() const;
    void CollectMeshes(Node3d* root, std::vector<MeshNode3d*>& out) const;
    void ApplyClipIfNeeded() const;

    bool m_playing = true;
    bool m_autoPlay = true;
    bool m_autoPlayed = false;
    bool m_loop = true;
    float m_speed = 1.0f;
    std::string m_clipName;
    TargetMode m_targetMode = TargetMode::Subtree;
    std::string m_targetMeshName;

    mutable std::vector<MeshNode3d*> m_targets;
    mutable std::string m_lastAppliedClip;
};

#endif //MANGORENDERING_ANIMATIONCONTROLLERNODE3D_H
