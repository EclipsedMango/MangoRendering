
// ReSharper disable CppDFAUnusedValue
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "GltfLoader.h"
#include "Mesh.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <functional>
#include <limits>
#include <memory>

#include "Core/ResourceManager.h"
#include "Renderer/Animation/Animator.h"

static std::unordered_map<std::string, std::shared_ptr<tinygltf::Model>> s_parsedModels;

// read a float from a buffer respecting componentType, normalization, and byte stride
static float ReadFloat(const uint8_t* base, const int componentType, const size_t index, const size_t byteStride, const size_t componentOffset) {
    const uint8_t* ptr = base + index * byteStride + componentOffset;
    switch (componentType) {
        case TINYGLTF_COMPONENT_TYPE_FLOAT: return *reinterpret_cast<const float*>(ptr);
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: return *ptr / 255.0f;
        case TINYGLTF_COMPONENT_TYPE_BYTE: return std::max(*reinterpret_cast<const int8_t*>(ptr) / 127.0f, -1.0f);
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return *reinterpret_cast<const uint16_t*>(ptr) / 65535.0f;
        case TINYGLTF_COMPONENT_TYPE_SHORT: return std::max(*reinterpret_cast<const int16_t*>(ptr) / 32767.0f, -1.0f);
        default: return 0.0f;
    }
}

static uint32_t ReadUInt(const uint8_t* base, const int componentType, const size_t index, const size_t byteStride, const size_t componentOffset) {
    const uint8_t* ptr = base + index * byteStride + componentOffset;
    switch (componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: return *ptr;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return *reinterpret_cast<const uint16_t*>(ptr);
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: return *reinterpret_cast<const uint32_t*>(ptr);
        default: return 0;
    }
}

struct AccessorData {
    const uint8_t* base = nullptr;
    size_t stride = 0; // effective byte stride between elements
    int compType = 0;
    size_t count = 0;
    int type = 0;
    bool valid = false;
};

static AccessorData GetAccessorData(const tinygltf::Model& model, const int accessorIndex) {
    if (accessorIndex < 0) return {};

    const auto& accessor = model.accessors[accessorIndex];
    const auto& bufferView = model.bufferViews[accessor.bufferView];
    const auto& buffer = model.buffers[bufferView.buffer];

    AccessorData d;
    d.base = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
    d.compType = accessor.componentType;
    d.count = accessor.count;
    d.type = accessor.type;

    // use explicit stride if provided, otherwise fall back to tight packing
    if (bufferView.byteStride > 0) {
        d.stride = bufferView.byteStride;
    } else {
        d.stride = tinygltf::GetNumComponentsInType(accessor.type) * tinygltf::GetComponentSizeInBytes(accessor.componentType);
    }

    d.valid = true;
    return d;
}

static std::vector<uint32_t> ExtractIndices(const tinygltf::Model& model, const tinygltf::Primitive& primitive) {
    std::vector<uint32_t> indices;
    if (primitive.indices < 0) {
        return indices;
    }

    const auto& idxAccessor = model.accessors[primitive.indices];
    const auto& idxView = model.bufferViews[idxAccessor.bufferView];
    const uint8_t* idxData = model.buffers[idxView.buffer].data.data() + idxView.byteOffset + idxAccessor.byteOffset;
    const size_t idxStride = idxView.byteStride > 0 ? idxView.byteStride : tinygltf::GetComponentSizeInBytes(idxAccessor.componentType);

    indices.reserve(idxAccessor.count);
    for (size_t i = 0; i < idxAccessor.count; i++) {
        const uint8_t* p = idxData + i * idxStride;
        switch (idxAccessor.componentType) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: indices.push_back(*p); break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: indices.push_back(*reinterpret_cast<const uint16_t*>(p)); break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: indices.push_back(*reinterpret_cast<const uint32_t*>(p)); break;
            default: {
                std::cerr << "GLTF: unsupported index component type" << std::endl;
                break;
            }
        }
    }

    return indices;
}

static bool ExtractVerticesFromPrimitive(const tinygltf::Model& model, const tinygltf::Primitive& primitive, const std::string& meshName, std::vector<Vertex>& vertices) {
    if (!primitive.attributes.contains("POSITION")) {
        return false;
    }

    AccessorData posData = GetAccessorData(model, primitive.attributes.at("POSITION"));

    AccessorData normData;
    if (primitive.attributes.contains("NORMAL")) {
        normData = GetAccessorData(model, primitive.attributes.at("NORMAL"));
    }

    AccessorData tanData;
    if (primitive.attributes.contains("TANGENT")) {
        tanData = GetAccessorData(model, primitive.attributes.at("TANGENT"));
    }

    AccessorData uvData;
    if (primitive.attributes.contains("TEXCOORD_0")) {
        uvData = GetAccessorData(model, primitive.attributes.at("TEXCOORD_0"));
    } else if (primitive.attributes.contains("TEXCOORD_1")) {
        std::cerr << "GLTF: TEXCOORD_0 missing, falling back to TEXCOORD_1 for mesh '" << meshName << "'" << std::endl;
        uvData = GetAccessorData(model, primitive.attributes.at("TEXCOORD_1"));
    }

    AccessorData jointsData;
    if (primitive.attributes.contains("JOINTS_0")) {
        jointsData = GetAccessorData(model, primitive.attributes.at("JOINTS_0"));
    }

    AccessorData weightsData;
    if (primitive.attributes.contains("WEIGHTS_0")) {
        weightsData = GetAccessorData(model, primitive.attributes.at("WEIGHTS_0"));
    }

    vertices.clear();
    vertices.reserve(posData.count);
    constexpr size_t floatSize = sizeof(float);

    for (size_t i = 0; i < posData.count; i++) {
        Vertex v{};

        v.position = {
            ReadFloat(posData.base, TINYGLTF_COMPONENT_TYPE_FLOAT, i, posData.stride, 0),
            ReadFloat(posData.base, TINYGLTF_COMPONENT_TYPE_FLOAT, i, posData.stride, floatSize),
            ReadFloat(posData.base, TINYGLTF_COMPONENT_TYPE_FLOAT, i, posData.stride, floatSize * 2)
        };

        if (normData.valid) {
            const size_t cs = tinygltf::GetComponentSizeInBytes(normData.compType);
            v.normal = {
                ReadFloat(normData.base, normData.compType, i, normData.stride, 0),
                ReadFloat(normData.base, normData.compType, i, normData.stride, cs),
                ReadFloat(normData.base, normData.compType, i, normData.stride, cs * 2)
            };
        } else {
            v.normal = glm::vec3(0, 1, 0);
        }

        if (tanData.valid) {
            const size_t cs = tinygltf::GetComponentSizeInBytes(tanData.compType);
            v.tangent = {
                ReadFloat(tanData.base, tanData.compType, i, tanData.stride, 0),
                ReadFloat(tanData.base, tanData.compType, i, tanData.stride, cs),
                ReadFloat(tanData.base, tanData.compType, i, tanData.stride, cs * 2),
                ReadFloat(tanData.base, tanData.compType, i, tanData.stride, cs * 3)
            };
        } else {
            v.tangent = glm::vec4(0.0f);
        }

        if (uvData.valid) {
            const size_t cs = tinygltf::GetComponentSizeInBytes(uvData.compType);
            v.texCoord = {
                ReadFloat(uvData.base, uvData.compType, i, uvData.stride, 0),
                ReadFloat(uvData.base, uvData.compType, i, uvData.stride, cs)
            };
        } else {
            v.texCoord = glm::vec2(0.0f);
        }

        if (jointsData.valid) {
            const size_t cs = tinygltf::GetComponentSizeInBytes(jointsData.compType);
            v.joints = glm::uvec4(
                ReadUInt(jointsData.base, jointsData.compType, i, jointsData.stride, 0),
                ReadUInt(jointsData.base, jointsData.compType, i, jointsData.stride, cs),
                ReadUInt(jointsData.base, jointsData.compType, i, jointsData.stride, cs * 2),
                ReadUInt(jointsData.base, jointsData.compType, i, jointsData.stride, cs * 3)
            );
        } else {
            v.joints = glm::uvec4(0);
        }

        if (weightsData.valid) {
            const size_t cs = tinygltf::GetComponentSizeInBytes(weightsData.compType);
            v.weights = glm::vec4(
                ReadFloat(weightsData.base, weightsData.compType, i, weightsData.stride, 0),
                ReadFloat(weightsData.base, weightsData.compType, i, weightsData.stride, cs),
                ReadFloat(weightsData.base, weightsData.compType, i, weightsData.stride, cs * 2),
                ReadFloat(weightsData.base, weightsData.compType, i, weightsData.stride, cs * 3)
            );

            const float sum = v.weights.x + v.weights.y + v.weights.z + v.weights.w;
            if (sum > 0.0f) {
                v.weights /= sum;
            }
        } else {
            v.weights = glm::vec4(0.0f);
        }

        vertices.push_back(v);
    }

    return true;
}

static std::vector<float> ReadFloatAccessor(const tinygltf::Model& model, const int accessorIndex) {
    std::vector<float> values;
    const AccessorData data = GetAccessorData(model, accessorIndex);
    if (!data.valid) {
        return values;
    }

    const auto& accessor = model.accessors[accessorIndex];
    const int componentCount = tinygltf::GetNumComponentsInType(accessor.type);
    const size_t cs = tinygltf::GetComponentSizeInBytes(data.compType);

    values.reserve(data.count * static_cast<size_t>(componentCount));
    for (size_t i = 0; i < data.count; ++i) {
        for (int c = 0; c < componentCount; ++c) {
            values.push_back(ReadFloat(data.base, data.compType, i, data.stride, cs * static_cast<size_t>(c)));
        }
    }

    return values;
}

static std::vector<glm::mat4> ReadMat4Accessor(const tinygltf::Model& model, const int accessorIndex) {
    std::vector<glm::mat4> matrices;
    const AccessorData data = GetAccessorData(model, accessorIndex);
    if (!data.valid) {
        return matrices;
    }

    const auto& accessor = model.accessors[accessorIndex];
    if (accessor.type != TINYGLTF_TYPE_MAT4) {
        return matrices;
    }

    const size_t cs = tinygltf::GetComponentSizeInBytes(data.compType);
    matrices.reserve(data.count);
    for (size_t i = 0; i < data.count; ++i) {
        glm::mat4 mat(1.0f);
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                const size_t offset = (static_cast<size_t>(col) * 4 + static_cast<size_t>(row)) * cs;
                mat[col][row] = ReadFloat(data.base, data.compType, i, data.stride, offset);
            }
        }
        matrices.push_back(mat);
    }

    return matrices;
}

static GltfLoader::AnimationPath ParseAnimationPath(const std::string& path) {
    if (path == "translation") return GltfLoader::AnimationPath::Translation;
    if (path == "rotation") return GltfLoader::AnimationPath::Rotation;
    if (path == "scale") return GltfLoader::AnimationPath::Scale;
    if (path == "weights") return GltfLoader::AnimationPath::Weights;
    return GltfLoader::AnimationPath::Unknown;
}

static glm::mat4 BuildNodeLocalTransform(const tinygltf::Node& gltfNode) {
    if (!gltfNode.matrix.empty()) {
        return glm::make_mat4(gltfNode.matrix.data());
    }

    glm::vec3 translation(0.0f);
    glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale(1.0f);

    if (!gltfNode.translation.empty()) {
        translation = glm::vec3(gltfNode.translation[0], gltfNode.translation[1], gltfNode.translation[2]);
    }

    if (!gltfNode.rotation.empty()) {
        rotation = glm::quat(gltfNode.rotation[3], gltfNode.rotation[0], gltfNode.rotation[1], gltfNode.rotation[2]);
    }

    if (!gltfNode.scale.empty()) {
        scale = glm::vec3(gltfNode.scale[0], gltfNode.scale[1], gltfNode.scale[2]);
    }

    return glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
}

static std::vector<GltfLoader::SkinData> ParseSkins(const tinygltf::Model& model) {
    std::vector<GltfLoader::SkinData> skins;
    skins.reserve(model.skins.size());

    for (const auto& gltfSkin : model.skins) {
        GltfLoader::SkinData skin;
        skin.name = gltfSkin.name;
        skin.skeletonRootNode = gltfSkin.skeleton;
        skin.joints = gltfSkin.joints;

        if (gltfSkin.inverseBindMatrices >= 0) {
            skin.inverseBindMatrices = ReadMat4Accessor(model, gltfSkin.inverseBindMatrices);
        }

        if (skin.inverseBindMatrices.size() < skin.joints.size()) {
            skin.inverseBindMatrices.resize(skin.joints.size(), glm::mat4(1.0f));
        }

        skins.push_back(std::move(skin));
    }

    return skins;
}

static std::vector<GltfLoader::AnimationClipData> ParseAnimations(const tinygltf::Model& model) {
    std::vector<GltfLoader::AnimationClipData> animations;
    animations.reserve(model.animations.size());

    for (const auto& gltfAnim : model.animations) {
        GltfLoader::AnimationClipData clip;
        clip.name = gltfAnim.name;
        clip.samplers.resize(gltfAnim.samplers.size());
        clip.channels.reserve(gltfAnim.channels.size());
        clip.startTime = std::numeric_limits<float>::max();
        clip.endTime = std::numeric_limits<float>::lowest();

        for (size_t samplerIdx = 0; samplerIdx < gltfAnim.samplers.size(); ++samplerIdx) {
            const auto& gltfSampler = gltfAnim.samplers[samplerIdx];
            auto& sampler = clip.samplers[samplerIdx];
            sampler.interpolation = gltfSampler.interpolation.empty() ? "LINEAR" : gltfSampler.interpolation;
            sampler.inputTimes = ReadFloatAccessor(model, gltfSampler.input);

            const auto outputData = GetAccessorData(model, gltfSampler.output);
            if (outputData.valid) {
                const auto& outputAccessor = model.accessors[gltfSampler.output];
                sampler.outputComponents = tinygltf::GetNumComponentsInType(outputAccessor.type);
                const size_t cs = tinygltf::GetComponentSizeInBytes(outputData.compType);
                sampler.outputValues.reserve(outputData.count);

                for (size_t i = 0; i < outputData.count; ++i) {
                    glm::vec4 output(0.0f);
                    for (int c = 0; c < sampler.outputComponents && c < 4; ++c) {
                        output[c] = ReadFloat(outputData.base, outputData.compType, i, outputData.stride, cs * static_cast<size_t>(c));
                    }
                    sampler.outputValues.push_back(output);
                }
            }

            if (!sampler.inputTimes.empty()) {
                clip.startTime = std::min(clip.startTime, sampler.inputTimes.front());
                clip.endTime = std::max(clip.endTime, sampler.inputTimes.back());
            }
        }

        if (clip.startTime == std::numeric_limits<float>::max()) {
            clip.startTime = 0.0f;
            clip.endTime = 0.0f;
        }

        for (const auto& gltfChannel : gltfAnim.channels) {
            GltfLoader::AnimationChannelData channel;
            channel.samplerIndex = gltfChannel.sampler;
            channel.targetNode = gltfChannel.target_node;
            channel.path = ParseAnimationPath(gltfChannel.target_path);
            clip.channels.push_back(channel);
        }

        animations.push_back(std::move(clip));
    }

    return animations;
}

static std::vector<std::shared_ptr<Skeleton>> BuildSkeletonResources(const tinygltf::Model& model, const std::vector<GltfLoader::SkinData>& skins) {
    std::vector<std::shared_ptr<Skeleton>> skeletons;
    skeletons.reserve(skins.size());

    std::vector<int> nodeParents(model.nodes.size(), -1);
    for (size_t parentIndex = 0; parentIndex < model.nodes.size(); ++parentIndex) {
        for (const int childNode : model.nodes[parentIndex].children) {
            if (childNode >= 0 && childNode < static_cast<int>(model.nodes.size())) {
                nodeParents[childNode] = static_cast<int>(parentIndex);
            }
        }
    }

    for (size_t skinIndex = 0; skinIndex < skins.size(); ++skinIndex) {
        const auto& skinData = skins[skinIndex];
        auto skeleton = std::make_shared<Skeleton>();
        skeleton->SetName(skinData.name.empty() ? "Skeleton_" + std::to_string(skinIndex) : skinData.name);
        skeleton->SetRootNodeIndex(skinData.skeletonRootNode);

        std::unordered_map<int, int> jointNodeToIndex;
        jointNodeToIndex.reserve(skinData.joints.size());
        for (size_t jointIdx = 0; jointIdx < skinData.joints.size(); ++jointIdx) {
            jointNodeToIndex[skinData.joints[jointIdx]] = static_cast<int>(jointIdx);
        }

        std::vector<Skeleton::Joint> joints;
        joints.resize(skinData.joints.size());
        for (size_t jointIdx = 0; jointIdx < skinData.joints.size(); ++jointIdx) {
            Skeleton::Joint joint;
            joint.nodeIndex = skinData.joints[jointIdx];

            if (joint.nodeIndex >= 0 && joint.nodeIndex < static_cast<int>(model.nodes.size())) {
                const auto& node = model.nodes[joint.nodeIndex];
                joint.name = node.name.empty() ? "Joint_" + std::to_string(jointIdx) : node.name;
                joint.restLocalTransform = BuildNodeLocalTransform(node);

                const int parentNode = nodeParents[joint.nodeIndex];
                if (const auto parentIt = jointNodeToIndex.find(parentNode); parentIt != jointNodeToIndex.end()) {
                    joint.parentJointIndex = parentIt->second;
                }
            } else {
                joint.name = "Joint_" + std::to_string(jointIdx);
            }

            if (jointIdx < skinData.inverseBindMatrices.size()) {
                joint.inverseBindMatrix = skinData.inverseBindMatrices[jointIdx];
            }

            joints[jointIdx] = joint;
        }

        skeleton->SetJoints(std::move(joints));
        skeletons.push_back(std::move(skeleton));
    }

    return skeletons;
}

static const GltfLoader::AnimationClipData* FindFirstClipForSkeleton(
    const std::vector<GltfLoader::AnimationClipData>& clips,
    const std::shared_ptr<Skeleton>& skeleton) {
    if (!skeleton) {
        return nullptr;
    }

    for (const auto& clip : clips) {
        for (const auto& channel : clip.channels) {
            if (skeleton->FindJointByNodeIndex(channel.targetNode) >= 0) {
                return &clip;
            }
        }
    }

    return nullptr;
}

static std::string ExtractTexture(const tinygltf::Model& model, const int textureIndex, const std::string& gltfPath) {
    if (textureIndex < 0 || textureIndex >= static_cast<int>(model.textures.size())) return "";

    const auto& gltfTexture = model.textures[textureIndex];
    if (gltfTexture.source < 0 || gltfTexture.source >= static_cast<int>(model.images.size())) return "";

    const auto& gltfImage = model.images[gltfTexture.source];
    const std::string outPath = gltfPath + "_tex" + std::to_string(textureIndex) + ".png";

    if (!std::filesystem::exists(outPath)) {
        if (!gltfImage.image.empty() && gltfImage.width > 0 && gltfImage.height > 0) {
            stbi_write_png(outPath.c_str(), gltfImage.width, gltfImage.height, gltfImage.component, gltfImage.image.data(), gltfImage.width * gltfImage.component);
        } else {
            return "";
        }
    }
    
    return outPath;
}

std::shared_ptr<Material> BuildMaterial(const tinygltf::Model& model, const int materialIndex, const std::string& gltfPath) {
    const std::string materialID = gltfPath + "#mat" + std::to_string(materialIndex);
    if (materialIndex >= 0) {
        if (auto existing = ResourceManager::Get().Get<Material>(materialID)) {
            return existing;
        }
    }

    auto mat = std::make_shared<Material>();
    if (materialIndex < 0) {
        return mat;
    }

    const auto& gltfMat = model.materials[materialIndex];
    mat->SetName(gltfMat.name.empty() ? materialID : gltfMat.name);
    mat->SetDoubleSided(gltfMat.doubleSided);

    const auto& pbr = gltfMat.pbrMetallicRoughness;
    if (pbr.baseColorFactor.size() == 4) {
        mat->SetAlbedoColor(glm::vec4(
            pbr.baseColorFactor[0], pbr.baseColorFactor[1],
            pbr.baseColorFactor[2], pbr.baseColorFactor[3]
        ));
    }

    mat->SetMetallicValue(static_cast<float>(pbr.metallicFactor));
    mat->SetRoughnessValue(static_cast<float>(pbr.roughnessFactor));

    std::string path = ExtractTexture(model, gltfMat.pbrMetallicRoughness.baseColorTexture.index, gltfPath);
    if (!path.empty()) mat->SetDiffuse(path);

    path = ExtractTexture(model, gltfMat.pbrMetallicRoughness.metallicRoughnessTexture.index, gltfPath);
    if (!path.empty()) {
        mat->SetMetallic(path);
        mat->SetRoughness(path);
    }

    path = ExtractTexture(model, gltfMat.normalTexture.index, gltfPath);
    if (!path.empty()) mat->SetNormal(path);

    path = ExtractTexture(model, gltfMat.occlusionTexture.index, gltfPath);
    if (!path.empty()) mat->SetAmbientOcclusion(path);

    path = ExtractTexture(model, gltfMat.emissiveTexture.index, gltfPath);
    if (!path.empty()) mat->SetEmissive(path);

    if (gltfMat.emissiveFactor.size() == 3) {
        mat->SetEmissionColor(glm::vec3(
            gltfMat.emissiveFactor[0],
            gltfMat.emissiveFactor[1],
            gltfMat.emissiveFactor[2]
        ));
    }

    if (gltfMat.alphaMode == "BLEND") {
        mat->SetBlendMode(BlendMode::AlphaBlend);
    } else if (gltfMat.alphaMode == "MASK") {
        mat->SetBlendMode(BlendMode::AlphaScissor);
        mat->SetAlphaScissorThreshold(static_cast<float>(gltfMat.alphaCutoff));
    }

    mat->SetNormalStrength(static_cast<float>(gltfMat.normalTexture.scale));
    mat->SetAOStrength(static_cast<float>(gltfMat.occlusionTexture.strength));

    ResourceManager::Get().Register<Material>(materialID, mat);
    return mat;
}

std::unique_ptr<Node3d> GltfLoader::Load(const std::string& path, std::shared_ptr<Shader> shader) {
    return LoadWithSkeletonData(path, std::move(shader)).sceneRoot;
}

GltfLoader::LoadResult GltfLoader::LoadWithSkeletonData(const std::string& path, std::shared_ptr<Shader> shader) {
    LoadResult result;

    auto modelPtr = GetParsedModel(path);
    if (!modelPtr) return result;
    const auto& model = *modelPtr;

    result.nodeToSkin.reserve(model.nodes.size());
    for (size_t nodeIdx = 0; nodeIdx < model.nodes.size(); ++nodeIdx) {
        const auto& node = model.nodes[nodeIdx];
        if (node.skin >= 0) {
            result.nodeToSkin[static_cast<int>(nodeIdx)] = node.skin;
        }
    }

    result.skins = ParseSkins(model);
    result.skeletons = BuildSkeletonResources(model, result.skins);
    result.animations = ParseAnimations(model);

    auto root = std::make_unique<Node3d>();

    std::unordered_map<int, std::vector<std::shared_ptr<Mesh>>> meshCache;

    // process node builds the subtree and adds it to parent, parent is a raw observer
    std::function<void(int, Node3d*)> ProcessNode = [&](int nodeIndex, Node3d* parent) {
        const auto& gltfNode = model.nodes[nodeIndex];
        auto sceneNode = std::make_unique<Node3d>();
        sceneNode->SetName(gltfNode.name);

        sceneNode->SetLocalTransform(BuildNodeLocalTransform(gltfNode));

        if (gltfNode.mesh >= 0) {
            const auto& gltfMesh = model.meshes[gltfNode.mesh];

            for (size_t primIndex = 0; primIndex < gltfMesh.primitives.size(); ++primIndex) {
                const auto& primitive = gltfMesh.primitives[primIndex];
                if (!primitive.attributes.contains("POSITION")) continue;

                // register mesh for resource manager
                std::string meshID = path + "#" + std::to_string(gltfNode.mesh) + "_" + std::to_string(primIndex);

                std::shared_ptr<Mesh> sharedMesh;
                auto cacheIt = meshCache.find(gltfNode.mesh);
                if (cacheIt != meshCache.end() && primIndex < cacheIt->second.size()) {
                    sharedMesh = cacheIt->second[primIndex];
                } else {
                    std::vector<Vertex> vertices;
                    if (!ExtractVerticesFromPrimitive(model, primitive, gltfMesh.name, vertices)) {
                        continue;
                    }
                    std::vector<uint32_t> indices = ExtractIndices(model, primitive);

                    sharedMesh = std::make_shared<Mesh>(vertices, indices);
                    meshCache[gltfNode.mesh].push_back(sharedMesh);
                }

                ResourceManager::Get().Register<Mesh>(meshID, sharedMesh);

                auto meshNode = std::make_unique<MeshNode3d>(sharedMesh);
                meshNode->GetActiveMaterial()->SetShader(shader);
                meshNode->SetMeshByName(meshID);
                meshNode->SetMaterial(BuildMaterial(model, primitive.material, path));

                if (gltfNode.skin >= 0 && gltfNode.skin < static_cast<int>(result.skeletons.size())) {
                    auto animator = std::make_shared<Animator>(result.skeletons[static_cast<size_t>(gltfNode.skin)]);
                    if (const auto* clip = FindFirstClipForSkeleton(result.animations, result.skeletons[static_cast<size_t>(gltfNode.skin)])) {
                        animator->SetClip(*clip);
                        animator->Play(true);
                    }
                    meshNode->SetAnimator(std::move(animator));
                }
                sceneNode->AddChild(std::move(meshNode));
            }
        }

        Node3d* sceneNodeRaw = sceneNode.get();
        parent->AddChild(std::move(sceneNode));

        for (int childIndex : gltfNode.children) {
            ProcessNode(childIndex, sceneNodeRaw);
        }
    };

    const auto& scene = model.scenes[model.defaultScene >= 0 ? model.defaultScene : 0];
    for (const int nodeIndex : scene.nodes) {
        ProcessNode(nodeIndex, root.get());
    }

    // compute world-space AABB over all mesh nodes
    glm::vec3 minBounds( std::numeric_limits<float>::max());
    glm::vec3 maxBounds(-std::numeric_limits<float>::max());

    std::function<void(Node3d*, const glm::mat4&)> ComputeAABB = [&](Node3d* node, const glm::mat4& parentWorld) {
        const glm::mat4 world = parentWorld * node->GetLocalMatrix();

        if (const auto* meshNode = dynamic_cast<MeshNode3d*>(node)) {
            const Mesh* mesh = meshNode->GetMesh();
            for (const Vertex& v : mesh->GetVertices()) {
                const glm::vec3 wp = glm::vec3(world * glm::vec4(v.position, 1.0f));
                minBounds = glm::min(minBounds, wp);
                maxBounds = glm::max(maxBounds, wp);
            }
        }

        for (Node3d* child : node->GetChildren()) {
            ComputeAABB(child, world);
        }
    };

    ComputeAABB(root.get(), glm::mat4(1.0f));

    const glm::vec3 size = maxBounds - minBounds;
    const glm::vec3 center = (minBounds + maxBounds) * 0.5f;
    const float extent = std::max({ size.x, size.y, size.z });

    if (extent > 0.0f) {
        const float scale = 1.0f / extent;
        const glm::mat4 normalize = glm::scale(glm::mat4(1.0f), glm::vec3(scale)) * glm::translate(glm::mat4(1.0f), -center);
        root->SetLocalTransform(normalize);
    }

    result.sceneRoot = std::move(root);
    return result;
}

std::shared_ptr<Mesh> GltfLoader::ExtractMesh(const std::string &path, int meshIndex, int primIndex) {
    auto modelPtr = GetParsedModel(path);
    if (!modelPtr) return nullptr;
    const auto& model = *modelPtr;

    // safety bounds checking
    if (meshIndex < 0 || meshIndex >= model.meshes.size()) return nullptr;
    if (primIndex < 0 || primIndex >= model.meshes[meshIndex].primitives.size()) return nullptr;

    const auto& primitive = model.meshes[meshIndex].primitives[primIndex];

    std::vector<Vertex> vertices;
    if (!ExtractVerticesFromPrimitive(model, primitive, model.meshes[meshIndex].name, vertices)) {
        return nullptr;
    }

    std::vector<uint32_t> indices = ExtractIndices(model, primitive);

    return std::make_shared<Mesh>(vertices, indices);
}

std::shared_ptr<tinygltf::Model> GltfLoader::GetParsedModel(const std::string &path) {
    if (s_parsedModels.contains(path)) {
        return s_parsedModels[path];
    }

    auto model = std::make_shared<tinygltf::Model>();
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    const bool success = path.ends_with(".glb") ? loader.LoadBinaryFromFile(model.get(), &err, &warn, path) : loader.LoadASCIIFromFile(model.get(), &err, &warn, path);

    if (!warn.empty()) std::cerr << "GLTF Warning: " << warn << std::endl;
    if (!err.empty())  std::cerr << "GLTF Error: "   << err  << std::endl;

    if (success) {
        s_parsedModels[path] = model;
        return model;
    }

    return nullptr;
}
