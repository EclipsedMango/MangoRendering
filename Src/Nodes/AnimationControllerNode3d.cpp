#include "AnimationControllerNode3d.h"

#include <algorithm>

#include "Nodes/MeshNode3d.h"
#include "Renderer/Animation/Animator.h"

REGISTER_NODE_TYPE(AnimationControllerNode3d)

AnimationControllerNode3d::AnimationControllerNode3d() {
    SetName("AnimationControllerNode3d");

    AddProperty("playing",
        [this]() -> PropertyValue { return m_playing; },
        [this](const PropertyValue& v) { m_playing = std::get<bool>(v); }
    );

    AddProperty("autoplay",
        [this]() -> PropertyValue { return m_autoPlay; },
        [this](const PropertyValue& v) { SetAutoPlay(std::get<bool>(v)); }
    );

    AddProperty("loop",
        [this]() -> PropertyValue { return m_loop; },
        [this](const PropertyValue& v) { SetLoop(std::get<bool>(v)); }
    );

    AddProperty("speed",
        [this]() -> PropertyValue { return m_speed; },
        [this](const PropertyValue& v) { SetSpeed(std::get<float>(v)); }
    );

    AddProperty("clip_name",
        [this]() -> PropertyValue { return m_clipName; },
        [this](const PropertyValue& v) { SetClipName(std::get<std::string>(v)); }
    );

    AddProperty("target_mode",
        [this]() -> PropertyValue {
            return m_targetMode == TargetMode::Scene ? std::string("scene") : std::string("subtree");
        },
        [this](const PropertyValue& v) {
            const std::string mode = std::get<std::string>(v);
            SetTargetMode(mode == "scene" ? TargetMode::Scene : TargetMode::Subtree);
        }
    );

    AddProperty("target_mesh_name",
        [this]() -> PropertyValue { return m_targetMeshName; },
        [this](const PropertyValue& v) { SetTargetMeshName(std::get<std::string>(v)); }
    );
}

AnimationControllerNode3d::~AnimationControllerNode3d() {
    ReleaseTargets();
}

std::unique_ptr<Node3d> AnimationControllerNode3d::Clone() {
    auto clone = std::make_unique<AnimationControllerNode3d>();
    clone->SetName(GetName());
    clone->SetVisible(IsVisible());
    clone->SetLocalTransform(GetLocalMatrix());
    clone->SetAutoPlay(m_autoPlay);
    clone->SetLoop(m_loop);
    clone->SetSpeed(m_speed);
    clone->SetClipName(m_clipName);
    clone->SetTargetMode(m_targetMode);
    clone->SetTargetMeshName(m_targetMeshName);
    if (m_playing) clone->Play();
    else clone->Pause();

    CopyBaseStateTo(*clone);
    for (Node3d* child : GetChildren()) {
        clone->AddChild(child->Clone());
    }
    return clone;
}

void AnimationControllerNode3d::Process(const float deltaTime) {
    Node3d::Process(deltaTime);

    RefreshTargets();
    ApplyClipIfNeeded();

    if (m_autoPlay && !m_autoPlayed) {
        Play();
        m_autoPlayed = true;
    }

    if (!m_playing || m_speed == 0.0f) {
        return;
    }

    const float scaledDelta = deltaTime * m_speed;
    for (MeshNode3d* meshNode : m_targets) {
        if (!meshNode) continue;
        const auto animator = meshNode->GetAnimator();
        if (animator) {
            animator->Update(scaledDelta);
        }
    }
}

void AnimationControllerNode3d::Play() {
    m_playing = true;
    RefreshTargets();
    for (const MeshNode3d* meshNode : m_targets) {
        if (!meshNode) continue;
        const auto animator = meshNode->GetAnimator();
        if (animator) {
            animator->Play(m_loop);
        }
    }
}

void AnimationControllerNode3d::Pause() {
    m_playing = false;
    RefreshTargets();
    for (const MeshNode3d* meshNode : m_targets) {
        if (!meshNode) continue;
        const auto animator = meshNode->GetAnimator();
        if (animator) {
            animator->Pause();
        }
    }
}

void AnimationControllerNode3d::Stop() {
    m_playing = false;
    RefreshTargets();
    for (const MeshNode3d* meshNode : m_targets) {
        if (!meshNode) continue;
        const auto animator = meshNode->GetAnimator();
        if (animator) {
            animator->Stop();
        }
    }
}

void AnimationControllerNode3d::SetSpeed(const float speed) {
    m_speed = speed;
}

void AnimationControllerNode3d::SetLoop(const bool loop) {
    m_loop = loop;
    if (m_playing) {
        Play();
    }
}

void AnimationControllerNode3d::SetClipName(const std::string& clipName) {
    m_clipName = clipName;
    m_lastAppliedClip.clear();
}

void AnimationControllerNode3d::SetAutoPlay(const bool autoPlay) {
    m_autoPlay = autoPlay;
    if (!m_autoPlay) {
        m_autoPlayed = false;
    }
}

void AnimationControllerNode3d::SetTargetMode(const TargetMode mode) {
    if (m_targetMode != mode) {
        ReleaseTargets();
    }
    m_targetMode = mode;
}

void AnimationControllerNode3d::SetTargetMeshName(const std::string& meshName) {
    if (m_targetMeshName != meshName) {
        ReleaseTargets();
    }
    m_targetMeshName = meshName;
}

void AnimationControllerNode3d::RefreshTargets() {
    ReleaseTargets();

    Node3d* root = this;
    if (m_targetMode == TargetMode::Scene) {
        while (root && root->GetParent()) {
            root = root->GetParent();
        }
    }

    if (!root) return;
    CollectMeshes(root, m_targets);

    for (MeshNode3d* meshNode : m_targets) {
        if (meshNode) {
            meshNode->SetAnimatorAutoUpdate(false);
        }
    }
}

void AnimationControllerNode3d::ReleaseTargets() const {
    for (MeshNode3d* meshNode : m_targets) {
        if (meshNode) {
            meshNode->SetAnimatorAutoUpdate(true);
        }
    }
    m_targets.clear();
}

void AnimationControllerNode3d::CollectMeshes(Node3d* root, std::vector<MeshNode3d*>& out) const {
    if (!root) return;

    if (auto* meshNode = dynamic_cast<MeshNode3d*>(root)) {
        if (m_targetMeshName.empty() || meshNode->GetName() == m_targetMeshName) {
            out.push_back(meshNode);
        }
    }

    for (Node3d* child : root->GetChildren()) {
        CollectMeshes(child, out);
    }
}

void AnimationControllerNode3d::ApplyClipIfNeeded() const {
    if (m_clipName.empty() || m_clipName == m_lastAppliedClip) {
        return;
    }

    bool applied = false;
    for (const MeshNode3d* meshNode : m_targets) {
        if (!meshNode) continue;
        const auto animator = meshNode->GetAnimator();
        if (!animator) continue;
        if (animator->SetClipByName(m_clipName)) {
            applied = true;
            if (m_playing) {
                animator->Play(m_loop);
            } else {
                animator->Pause();
            }
        }
    }

    if (applied) {
        m_lastAppliedClip = m_clipName;
    }
}
