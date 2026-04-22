#ifndef MANGORENDERING_ANIMATOR_H
#define MANGORENDERING_ANIMATOR_H

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <limits>

#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include "Renderer/Animation/Skeleton.h"
#include "Renderer/Meshes/GltfLoader.h"

class Shader;
class ShaderStorageBuffer;

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
    void SetAvailableClips(std::vector<GltfLoader::AnimationClipData> clips);
    [[nodiscard]] bool SetClipByName(const std::string& clipName);

    void Play(bool loop = true);
    void Pause();
    void Stop();
    static void BeginFrame();
    void AdvanceTime(float deltaTime);
    void Update(float deltaTime);
    void EvaluateAt(float timeSeconds);

    [[nodiscard]] bool HasSkeleton() const { return static_cast<bool>(m_skeleton); }
    [[nodiscard]] bool HasClip() const { return m_hasClip; }
    [[nodiscard]] bool IsPlaying() const { return m_isPlaying; }
    [[nodiscard]] float GetCurrentTime() const { return m_currentTime; }
    [[nodiscard]] float GetClipStartTime() const { return m_clip.startTime; }
    [[nodiscard]] float GetClipEndTime() const { return m_clip.endTime; }
    [[nodiscard]] uint64_t GetPoseVersion() const { return m_poseVersion; }
    [[nodiscard]] int GetSkinMatrixCount() const;
    [[nodiscard]] ShaderStorageBuffer* GetSkinMatricesSsbo() const;
    [[nodiscard]] static float ConsumeFrameUpdateMs();

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

    void RebuildSkeletonCaches();
    void ResetPoseFromSkeleton();
    void RebuildChannelJointMap();
    void EvaluateGlobalAndSkinMatrices();
    void EnsureSkinningComputeBuffers();
    void UploadLocalJointMatricesToGpu();
    void DispatchSkinningHierarchyCompute() const;

    std::shared_ptr<Skeleton> m_skeleton;
    GltfLoader::AnimationClipData m_clip;
    std::vector<GltfLoader::AnimationClipData> m_availableClips;
    bool m_hasClip = false;
    bool m_isPlaying = false;
    bool m_loop = true;
    float m_currentTime = 0.0f;
    uint64_t m_poseVersion = 0;
    uint64_t m_lastUpdatedFrame = std::numeric_limits<uint64_t>::max();
    bool m_poseDirty = true;

    std::vector<JointPose> m_restPose;
    std::vector<JointPose> m_localPose;
    std::vector<glm::mat4> m_localJointMatrices;
    std::vector<glm::mat4> m_globalJointMatrices;
    std::vector<glm::mat4> m_skinMatrices;
    std::vector<int> m_channelJointIndices;
    std::vector<int> m_parentJointIndices;
    std::vector<char> m_jointComputed;
    std::vector<int> m_animatedJointIndices;

    std::unique_ptr<ShaderStorageBuffer> m_localJointSsbo;
    std::unique_ptr<ShaderStorageBuffer> m_inverseBindSsbo;
    std::unique_ptr<ShaderStorageBuffer> m_parentIndexSsbo;
    std::unique_ptr<ShaderStorageBuffer> m_skinMatricesSsbo;
    std::shared_ptr<Shader> m_skinningHierarchyComputeShader;
    bool m_gpuSkeletonStaticUploaded = false;
};

#endif //MANGORENDERING_ANIMATOR_H
