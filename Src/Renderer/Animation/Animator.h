#ifndef MANGORENDERING_ANIMATOR_H
#define MANGORENDERING_ANIMATOR_H

#include <memory>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include "Renderer/Animation/Skeleton.h"
#include "Renderer/Meshes/GltfLoader.h"

class Animator {
public:
    struct JointPose {
        glm::vec3 translation = glm::vec3(0.0f);
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
    };

    Animator() = default;
    explicit Animator(std::shared_ptr<Skeleton> skeleton);

    void SetSkeleton(std::shared_ptr<Skeleton> skeleton);
    void SetClip(const GltfLoader::AnimationClipData& clip);
    [[nodiscard]] bool SetClipByName(const std::vector<GltfLoader::AnimationClipData>& clips, const std::string& clipName);

    void Play(bool loop = true);
    void Pause();
    void Stop();
    void Update(float deltaTime);
    void EvaluateAt(float timeSeconds);

    [[nodiscard]] bool HasSkeleton() const { return static_cast<bool>(m_skeleton); }
    [[nodiscard]] bool HasClip() const { return m_hasClip; }
    [[nodiscard]] bool IsPlaying() const { return m_isPlaying; }
    [[nodiscard]] float GetCurrentTime() const { return m_currentTime; }
    [[nodiscard]] float GetClipStartTime() const { return m_clip.startTime; }
    [[nodiscard]] float GetClipEndTime() const { return m_clip.endTime; }

    [[nodiscard]] const std::vector<JointPose>& GetLocalPoses() const { return m_localPose; }
    [[nodiscard]] const std::vector<glm::mat4>& GetGlobalJointMatrices() const { return m_globalJointMatrices; }
    [[nodiscard]] const std::vector<glm::mat4>& GetSkinMatrices() const { return m_skinMatrices; }

private:
    struct SampleResult {
        glm::vec4 value = glm::vec4(0.0f);
        bool valid = false;
    };

    static JointPose PoseFromMatrix(const glm::mat4& matrix);
    static glm::mat4 PoseToMatrix(const JointPose& pose);
    static SampleResult SampleChannel(const GltfLoader::AnimationSamplerData& sampler, float timeSeconds, GltfLoader::AnimationPath path);

    void ResetPoseFromSkeleton();
    void EvaluateGlobalAndSkinMatrices();

    glm::mat4 ComputeJointGlobalMatrix(int jointIndex, std::vector<char>& computed);

    std::shared_ptr<Skeleton> m_skeleton;
    GltfLoader::AnimationClipData m_clip;
    bool m_hasClip = false;
    bool m_isPlaying = false;
    bool m_loop = true;
    float m_currentTime = 0.0f;

    std::vector<JointPose> m_localPose;
    std::vector<glm::mat4> m_globalJointMatrices;
    std::vector<glm::mat4> m_skinMatrices;
};

#endif //MANGORENDERING_ANIMATOR_H
