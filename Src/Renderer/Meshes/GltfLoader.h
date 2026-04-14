#ifndef MANGORENDERING_GLTFLOADER_H
#define MANGORENDERING_GLTFLOADER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include "tiny_gltf.h"
#include "Nodes/MeshNode3d.h"
#include "Renderer/Animation/Skeleton.h"
#include "Renderer/Shader.h"

class GltfLoader {
public:
    enum class AnimationPath {
        Translation,
        Rotation,
        Scale,
        Weights,
        Unknown
    };

    struct SkinData {
        std::string name;
        std::vector<int> joints;
        std::vector<glm::mat4> inverseBindMatrices;
        int skeletonRootNode = -1;
    };

    struct AnimationSamplerData {
        std::string interpolation = "LINEAR";
        std::vector<float> inputTimes;
        std::vector<glm::vec4> outputValues;
        int outputComponents = 0;
    };

    struct AnimationChannelData {
        int samplerIndex = -1;
        int targetNode = -1;
        AnimationPath path = AnimationPath::Unknown;
    };

    struct AnimationClipData {
        std::string name;
        std::vector<AnimationSamplerData> samplers;
        std::vector<AnimationChannelData> channels;
        float startTime = 0.0f;
        float endTime = 0.0f;
    };

    struct LoadResult {
        std::unique_ptr<Node3d> sceneRoot;
        std::vector<std::shared_ptr<Skeleton>> skeletons;
        std::vector<SkinData> skins;
        std::vector<AnimationClipData> animations;
        std::unordered_map<int, int> nodeToSkin;
    };

    // loads the full scene hierarchy, caller owns the returned tree
    static std::unique_ptr<Node3d> Load(const std::string& path, std::shared_ptr<Shader> shader);
    static LoadResult LoadWithSkeletonData(const std::string& path, std::shared_ptr<Shader> shader);

    // extracts a specific sub mesh
    static std::shared_ptr<Mesh> ExtractMesh(const std::string& path, int meshIndex, int primIndex);

private:
    static std::shared_ptr<tinygltf::Model> GetParsedModel(const std::string& path);
};

#endif //MANGORENDERING_GLTFLOADER_H
