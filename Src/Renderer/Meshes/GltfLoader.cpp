
// ReSharper disable CppDFAUnusedValue
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "GltfLoader.h"
#include "Mesh.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <functional>
#include <memory>

#include "Core/ResourceManager.h"

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

struct AccessorData {
    const uint8_t* base = nullptr;
    size_t stride = 0; // effective byte stride between elements
    int compType = 0;
    size_t count = 0;
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

    // use explicit stride if provided, otherwise fall back to tight packing
    if (bufferView.byteStride > 0) {
        d.stride = bufferView.byteStride;
    } else {
        d.stride = tinygltf::GetNumComponentsInType(accessor.type) * tinygltf::GetComponentSizeInBytes(accessor.componentType);
    }

    d.valid = true;
    return d;
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

    ResourceManager::Get().LoadTexture(outPath);
    return outPath;
}

std::shared_ptr<Material> BuildMaterial(const tinygltf::Model& model, const int materialIndex, const std::string& gltfPath) {
    const std::string materialID = gltfPath + "#mat" + std::to_string(materialIndex);
    if (materialIndex >= 0) {
        if (auto existing = ResourceManager::Get().GetMaterial(materialID)) {
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

    ResourceManager::Get().RegisterMaterial(materialID, mat);
    return mat;
}

std::unique_ptr<Node3d> GltfLoader::Load(const std::string& path, std::shared_ptr<Shader> shader) {
    auto modelPtr = GetParsedModel(path);
    if (!modelPtr) return nullptr;
    const auto& model = *modelPtr;

    auto root = std::make_unique<Node3d>();

    std::unordered_map<int, std::vector<std::shared_ptr<Mesh>>> meshCache;

    // process node builds the subtree and adds it to parent, parent is a raw observer
    std::function<void(int, Node3d*)> ProcessNode = [&](int nodeIndex, Node3d* parent) {
        const auto& gltfNode = model.nodes[nodeIndex];
        auto sceneNode = std::make_unique<Node3d>();
        sceneNode->SetName(gltfNode.name);

        // Build local transform
        glm::mat4 localTransform(1.0f);
        if (!gltfNode.matrix.empty()) {
            localTransform = glm::make_mat4(gltfNode.matrix.data());
        } else {
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

            localTransform = glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
        }

        sceneNode->SetLocalTransform(localTransform);

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
                    AccessorData posData = GetAccessorData(model, primitive.attributes.at("POSITION"));

                    AccessorData normData;
                    if (primitive.attributes.contains("NORMAL")) {
                        normData = GetAccessorData(model, primitive.attributes.at("NORMAL"));
                    }

                    AccessorData tanData;
                    if (primitive.attributes.contains("TANGENT")) {
                        tanData = GetAccessorData(model, primitive.attributes.at("TANGENT"));
                    }

                    // TEXCOORD_0, fall back to TEXCOORD_1 if absent
                    AccessorData uvData;
                    if (primitive.attributes.contains("TEXCOORD_0"))
                        uvData = GetAccessorData(model, primitive.attributes.at("TEXCOORD_0"));
                    else if (primitive.attributes.contains("TEXCOORD_1")) {
                        std::cerr << "GLTF: TEXCOORD_0 missing, falling back to TEXCOORD_1 for mesh '" << gltfMesh.name << "'" << std::endl;
                        uvData = GetAccessorData(model, primitive.attributes.at("TEXCOORD_1"));
                    }

                    std::vector<Vertex> vertices;
                    vertices.reserve(posData.count);
                    const size_t floatSize = sizeof(float);

                    for (size_t i = 0; i < posData.count; i++) {
                        Vertex v{};

                        v.position = {
                            ReadFloat(posData.base, TINYGLTF_COMPONENT_TYPE_FLOAT, i, posData.stride, 0),
                            ReadFloat(posData.base, TINYGLTF_COMPONENT_TYPE_FLOAT, i, posData.stride, floatSize),
                            ReadFloat(posData.base, TINYGLTF_COMPONENT_TYPE_FLOAT, i, posData.stride, floatSize * 2)
                        };

                        if (normData.valid) {
                            v.normal = {
                                ReadFloat(normData.base, normData.compType, i, normData.stride, 0),
                                ReadFloat(normData.base, normData.compType, i, normData.stride, tinygltf::GetComponentSizeInBytes(normData.compType)),
                                ReadFloat(normData.base, normData.compType, i, normData.stride, tinygltf::GetComponentSizeInBytes(normData.compType) * 2)
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

                        vertices.push_back(v);
                    }

                    // indices
                    std::vector<uint32_t> indices;
                    if (primitive.indices >= 0) {
                        const auto& idxAccessor = model.accessors[primitive.indices];
                        const auto& idxView = model.bufferViews[idxAccessor.bufferView];
                        const uint8_t* idxData = model.buffers[idxView.buffer].data.data() + idxView.byteOffset + idxAccessor.byteOffset;
                        size_t idxStride = idxView.byteStride > 0 ? idxView.byteStride : tinygltf::GetComponentSizeInBytes(idxAccessor.componentType);

                        indices.reserve(idxAccessor.count);
                        for (size_t i = 0; i < idxAccessor.count; i++) {
                            const uint8_t* p = idxData + i * idxStride;
                            switch (idxAccessor.componentType) {
                                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: indices.push_back(*p); break;
                                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: indices.push_back(*reinterpret_cast<const uint16_t*>(p)); break;
                                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: indices.push_back(*reinterpret_cast<const uint32_t*>(p)); break;
                                default: std::cerr << "GLTF: unsupported index component type" << std::endl; break;
                            }
                        }
                    }

                    sharedMesh = std::make_shared<Mesh>(vertices, indices);
                    meshCache[gltfNode.mesh].push_back(sharedMesh);
                }

                ResourceManager::Get().RegisterMesh(meshID, sharedMesh);

                auto meshNode = std::make_unique<MeshNode3d>(sharedMesh, shader);
                meshNode->SetMeshByName(meshID);
                meshNode->SetMaterial(BuildMaterial(model, primitive.material, path));
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

    return root;
}

std::shared_ptr<Mesh> GltfLoader::ExtractMesh(const std::string &path, int meshIndex, int primIndex) {
    auto modelPtr = GetParsedModel(path);
    if (!modelPtr) return nullptr;
    const auto& model = *modelPtr;

    // safety bounds checking
    if (meshIndex < 0 || meshIndex >= model.meshes.size()) return nullptr;
    if (primIndex < 0 || primIndex >= model.meshes[meshIndex].primitives.size()) return nullptr;

    const auto& primitive = model.meshes[meshIndex].primitives[primIndex];
    if (!primitive.attributes.contains("POSITION")) return nullptr;

    AccessorData posData = GetAccessorData(model, primitive.attributes.at("POSITION"));

    AccessorData normData;
    if (primitive.attributes.contains("NORMAL")) normData = GetAccessorData(model, primitive.attributes.at("NORMAL"));

    AccessorData uvData;
    if (primitive.attributes.contains("TEXCOORD_0")) uvData = GetAccessorData(model, primitive.attributes.at("TEXCOORD_0"));

    std::vector<Vertex> vertices;
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
            v.normal = {
                ReadFloat(normData.base, normData.compType, i, normData.stride, 0),
                ReadFloat(normData.base, normData.compType, i, normData.stride, tinygltf::GetComponentSizeInBytes(normData.compType)),
                ReadFloat(normData.base, normData.compType, i, normData.stride, tinygltf::GetComponentSizeInBytes(normData.compType) * 2)
            };
        } else v.normal = glm::vec3(0, 1, 0);

        if (uvData.valid) {
            const size_t cs = tinygltf::GetComponentSizeInBytes(uvData.compType);
            v.texCoord = {
                ReadFloat(uvData.base, uvData.compType, i, uvData.stride, 0),
                ReadFloat(uvData.base, uvData.compType, i, uvData.stride, cs)
            };
        } else v.texCoord = glm::vec2(0.0f);

        vertices.push_back(v);
    }

    std::vector<uint32_t> indices;
    if (primitive.indices >= 0) {
        const auto& idxAccessor = model.accessors[primitive.indices];
        const auto& idxView     = model.bufferViews[idxAccessor.bufferView];
        const uint8_t* idxData  = model.buffers[idxView.buffer].data.data() + idxView.byteOffset + idxAccessor.byteOffset;
        size_t idxStride = idxView.byteStride > 0 ? idxView.byteStride : tinygltf::GetComponentSizeInBytes(idxAccessor.componentType);

        indices.reserve(idxAccessor.count);
        for (size_t i = 0; i < idxAccessor.count; i++) {
            const uint8_t* p = idxData + i * idxStride;
            switch (idxAccessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: indices.push_back(*p); break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: indices.push_back(*reinterpret_cast<const uint16_t*>(p)); break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: indices.push_back(*reinterpret_cast<const uint32_t*>(p)); break;
                default: break;
            }
        }
    }

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
