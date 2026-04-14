#include "Animator.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace {
    float s_frameAnimatorUpdateMs = 0.0f;
}

Animator::Animator(std::shared_ptr<Skeleton> skeleton) {
    SetSkeleton(std::move(skeleton));
}

void Animator::SetSkeleton(std::shared_ptr<Skeleton> skeleton) {
    m_skeleton = std::move(skeleton);
    m_poseDirty = true;
    ResetPoseFromSkeleton();
    EvaluateGlobalAndSkinMatrices();
    m_poseVersion++;
    m_poseDirty = false;
}

void Animator::SetClip(const GltfLoader::AnimationClipData& clip) {
    m_clip = clip;
    m_hasClip = true;
    m_currentTime = m_clip.startTime;
    m_poseDirty = true;
    EvaluateAt(m_currentTime);
}

void Animator::SetAvailableClips(std::vector<GltfLoader::AnimationClipData> clips) {
    m_availableClips = std::move(clips);
}

bool Animator::SetClipByName(const std::string& clipName) {
    const auto it = std::ranges::find_if(m_availableClips, [&](const GltfLoader::AnimationClipData& clip) {
        return clip.name == clipName;
    });

    if (it == m_availableClips.end()) {
        return false;
    }

    SetClip(*it);
    return true;
}

void Animator::Play(const bool loop) {
    if (!m_hasClip || !m_skeleton) {
        return;
    }

    m_loop = loop;
    m_isPlaying = true;
}

void Animator::Pause() {
    m_isPlaying = false;
}

void Animator::Stop() {
    m_isPlaying = false;
    if (m_hasClip) {
        m_currentTime = m_clip.startTime;
    } else {
        m_currentTime = 0.0f;
    }
    m_poseDirty = true;
    EvaluateAt(m_currentTime);
}

void Animator::Update(const float deltaTime) {
    if (!m_isPlaying || !m_hasClip || !m_skeleton) {
        return;
    }

    const float clipStart = m_clip.startTime;
    const float clipEnd = m_clip.endTime;
    const float duration = std::max(clipEnd - clipStart, 0.0f);

    if (duration <= 0.0f) {
        EvaluateAt(clipStart);
        return;
    }

    float nextTime = m_currentTime + deltaTime;
    if (m_loop) {
        const float offset = std::fmod(nextTime - clipStart, duration);
        nextTime = clipStart + (offset < 0.0f ? offset + duration : offset);
    } else if (nextTime >= clipEnd) {
        nextTime = clipEnd;
        m_isPlaying = false;
    }

    const auto t0 = std::chrono::high_resolution_clock::now();
    const uint64_t prevVersion = m_poseVersion;
    EvaluateAt(nextTime);
    const auto t1 = std::chrono::high_resolution_clock::now();
    if (m_poseVersion != prevVersion) {
        s_frameAnimatorUpdateMs += std::chrono::duration<float, std::milli>(t1 - t0).count();
    }
}

void Animator::EvaluateAt(const float timeSeconds) {
    if (!m_skeleton) {
        return;
    }

    constexpr float timeEps = 1e-6f;
    if (!m_poseDirty && std::abs(timeSeconds - m_currentTime) <= timeEps) {
        return;
    }

    m_currentTime = timeSeconds;
    ResetPoseFromSkeleton();

    if (!m_hasClip) {
        EvaluateGlobalAndSkinMatrices();
        return;
    }

    for (const auto& channel : m_clip.channels) {
        if (channel.samplerIndex < 0 || channel.samplerIndex >= static_cast<int>(m_clip.samplers.size())) {
            continue;
        }

        const int jointIndex = m_skeleton->FindJointByNodeIndex(channel.targetNode);
        if (jointIndex < 0 || jointIndex >= static_cast<int>(m_localPose.size())) {
            continue;
        }

        const auto sample = SampleChannel(m_clip.samplers[channel.samplerIndex], timeSeconds, channel.path);
        if (!sample.valid) {
            continue;
        }

        JointPose& pose = m_localPose[static_cast<size_t>(jointIndex)];
        switch (channel.path) {
            case GltfLoader::AnimationPath::Translation:
                pose.translation = glm::vec3(sample.value);
                break;
            case GltfLoader::AnimationPath::Rotation:
                pose.rotation = glm::normalize(glm::quat(sample.value.w, sample.value.x, sample.value.y, sample.value.z));
                break;
            case GltfLoader::AnimationPath::Scale:
                pose.scale = glm::vec3(sample.value);
                break;
            default:
                break;
        }
    }

    EvaluateGlobalAndSkinMatrices();
    m_poseVersion++;
    m_poseDirty = false;
}

Animator::JointPose Animator::PoseFromMatrix(const glm::mat4& matrix) {
    JointPose pose;
    pose.translation = glm::vec3(matrix[3]);

    pose.scale = {
        glm::length(glm::vec3(matrix[0])),
        glm::length(glm::vec3(matrix[1])),
        glm::length(glm::vec3(matrix[2]))
    };

    constexpr float eps = 1e-4f;
    const glm::mat3 rotationMatrix(
        pose.scale.x > eps ? glm::vec3(matrix[0]) / pose.scale.x : glm::vec3(1, 0, 0),
        pose.scale.y > eps ? glm::vec3(matrix[1]) / pose.scale.y : glm::vec3(0, 1, 0),
        pose.scale.z > eps ? glm::vec3(matrix[2]) / pose.scale.z : glm::vec3(0, 0, 1)
    );
    pose.rotation = glm::normalize(glm::quat_cast(rotationMatrix));
    return pose;
}

glm::mat4 Animator::PoseToMatrix(const JointPose& pose) {
    return glm::translate(glm::mat4(1.0f), pose.translation) * glm::mat4_cast(pose.rotation) * glm::scale(glm::mat4(1.0f), pose.scale);
}

Animator::SampleResult Animator::SampleChannel(const GltfLoader::AnimationSamplerData& sampler, const float timeSeconds, const GltfLoader::AnimationPath path) {
    SampleResult out;

    if (sampler.inputTimes.empty() || sampler.outputValues.empty() || sampler.outputComponents <= 0) {
        return out;
    }

    const auto firstTime = sampler.inputTimes.front();
    const auto lastTime = sampler.inputTimes.back();

    auto valueAtIndex = [&](const size_t keyIndex) -> glm::vec4 {
        if (sampler.interpolation == "CUBICSPLINE") {
            const size_t base = keyIndex * 3 + 1;
            if (base < sampler.outputValues.size()) {
                return sampler.outputValues[base];
            }
        }

        if (keyIndex < sampler.outputValues.size()) {
            return sampler.outputValues[keyIndex];
        }

        return sampler.outputValues.back();
    };

    if (timeSeconds <= firstTime) {
        out.value = valueAtIndex(0);
        out.valid = true;
        return out;
    }

    if (timeSeconds >= lastTime) {
        out.value = valueAtIndex(sampler.inputTimes.size() - 1);
        out.valid = true;
        return out;
    }

    const auto upperIt = std::upper_bound(sampler.inputTimes.begin(), sampler.inputTimes.end(), timeSeconds);
    if (upperIt == sampler.inputTimes.begin() || upperIt == sampler.inputTimes.end()) {
        out.value = sampler.outputValues.back();
        out.valid = true;
        return out;
    }

    const size_t rightIndex = static_cast<size_t>(upperIt - sampler.inputTimes.begin());
    const size_t leftIndex = rightIndex - 1;
    const float t0 = sampler.inputTimes[leftIndex];
    const float t1 = sampler.inputTimes[rightIndex];
    const float alpha = (t1 > t0) ? (timeSeconds - t0) / (t1 - t0) : 0.0f;

    const glm::vec4 v0 = valueAtIndex(leftIndex);
    const glm::vec4 v1 = valueAtIndex(rightIndex);

    if (sampler.interpolation == "STEP") {
        out.value = v0;
        out.valid = true;
        return out;
    }

    if (path == GltfLoader::AnimationPath::Rotation) {
        const glm::quat q0 = glm::normalize(glm::quat(v0.w, v0.x, v0.y, v0.z));
        const glm::quat q1 = glm::normalize(glm::quat(v1.w, v1.x, v1.y, v1.z));
        const glm::quat qr = glm::normalize(glm::slerp(q0, q1, alpha));
        out.value = glm::vec4(qr.x, qr.y, qr.z, qr.w);
    } else {
        out.value = glm::mix(v0, v1, alpha);
    }

    out.valid = true;
    return out;
}

void Animator::ResetPoseFromSkeleton() {
    m_localPose.clear();
    m_globalJointMatrices.clear();
    m_skinMatrices.clear();

    if (!m_skeleton) {
        return;
    }

    const auto& joints = m_skeleton->GetJoints();
    m_localPose.resize(joints.size());
    m_globalJointMatrices.resize(joints.size(), glm::mat4(1.0f));
    m_skinMatrices.resize(joints.size(), glm::mat4(1.0f));

    for (size_t i = 0; i < joints.size(); ++i) {
        m_localPose[i] = PoseFromMatrix(joints[i].restLocalTransform);
    }
}

void Animator::EvaluateGlobalAndSkinMatrices() {
    if (!m_skeleton) {
        return;
    }

    const auto& joints = m_skeleton->GetJoints();
    if (joints.empty()) {
        return;
    }

    std::vector<char> computed(joints.size(), 0);
    for (size_t i = 0; i < joints.size(); ++i) {
        ComputeJointGlobalMatrix(static_cast<int>(i), computed);
    }

    for (size_t i = 0; i < joints.size(); ++i) {
        m_skinMatrices[i] = m_globalJointMatrices[i] * joints[i].inverseBindMatrix;
    }
}

glm::mat4 Animator::ComputeJointGlobalMatrix(const int jointIndex, std::vector<char>& computed) {
    if (computed[jointIndex]) {
        return m_globalJointMatrices[static_cast<size_t>(jointIndex)];
    }

    const auto& joints = m_skeleton->GetJoints();
    const auto& joint = joints[static_cast<size_t>(jointIndex)];
    const glm::mat4 local = PoseToMatrix(m_localPose[static_cast<size_t>(jointIndex)]);

    if (joint.parentJointIndex >= 0) {
        m_globalJointMatrices[static_cast<size_t>(jointIndex)] = ComputeJointGlobalMatrix(joint.parentJointIndex, computed) * local;
    } else {
        m_globalJointMatrices[static_cast<size_t>(jointIndex)] = local;
    }

    computed[jointIndex] = 1;
    return m_globalJointMatrices[static_cast<size_t>(jointIndex)];
}

float Animator::ConsumeFrameUpdateMs() {
    const float value = s_frameAnimatorUpdateMs;
    s_frameAnimatorUpdateMs = 0.0f;
    return value;
}
