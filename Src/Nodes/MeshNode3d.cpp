
#include "MeshNode3d.h"

#include <chrono>
#include <cstdint>
#include <iostream>

#include "glad/gl.h"
#include "Core/RenderApi.h"
#include "Core/ResourceManager.h"
#include "Renderer/Animation/Animator.h"
#include "Renderer/Buffers/ShaderStorageBuffer.h"

namespace {
    float s_frameSkinUploadMs = 0.0f;

    struct GpuSkinnedVertex {
        glm::vec4 position;
        glm::vec4 normal;
        glm::vec4 tangent;
    };

    constexpr uint32_t kSkinnedOutputBinding = 11;
    constexpr uint32_t kSkinningComputeWorkGroupSize = 64;
}

REGISTER_NODE_TYPE(MeshNode3d)

MeshNode3d::MeshNode3d() : m_material(std::make_shared<Material>()) {
    MeshNode3d::Init();
}

MeshNode3d::MeshNode3d(std::shared_ptr<Mesh> mesh) : m_mesh(std::move(mesh)), m_material(std::make_shared<Material>()) {
    RefreshSkinningFlags();
    MeshNode3d::Init();
}

MeshNode3d::~MeshNode3d() = default;

std::unique_ptr<Node3d> MeshNode3d::Clone() {
    auto clone = std::make_unique<MeshNode3d>(m_mesh);

    clone->SetName(GetName());
    clone->SetVisible(IsVisible());
    clone->SetLocalTransform(GetLocalMatrix());

    if (m_material) {
        const auto materialCopy = std::make_shared<Material>(*m_material);
        clone->SetMaterial(materialCopy);
    }

    if (m_materialOverride) {
        const auto overrideCopy = std::make_shared<Material>(*m_materialOverride);
        clone->SetMaterialOverride(overrideCopy);
    }
    clone->SetAnimator(m_animator);
    clone->SetAnimatorAutoUpdate(m_animatorAutoUpdate);
    clone->SetIsUnique(m_isUnique);

    CopyBaseStateTo(*clone);

    for (Node3d* child : GetChildren()) {
        clone->AddChild(child->Clone());
    }

    return clone;
}

void MeshNode3d::Process(const float deltaTime) {
    Node3d::Process(deltaTime);
    if (m_animator && m_animatorAutoUpdate) {
        m_animator->AdvanceTime(deltaTime);
    }
}

void MeshNode3d::SetMesh(std::shared_ptr<Mesh> mesh) {
    m_mesh = std::move(mesh);
    RefreshSkinningFlags();
    m_uploadedPoseVersion = std::numeric_limits<uint64_t>::max();
    m_uploadedSkinCount = 0;
    m_dispatchedVertexCount = 0;
    m_skinnedVertexSsbo.reset();
}

void MeshNode3d::SetMeshByName(const std::string &name) {
    SetMesh(ResourceManager::Get().Load<Mesh>(name));
    if (m_meshSlot) {
        m_meshSlot->Set("mesh_type", name);
    }
}

void MeshNode3d::SetAnimator(std::shared_ptr<Animator> animator) {
    m_animator = std::move(animator);
    m_uploadedPoseVersion = std::numeric_limits<uint64_t>::max();
    m_uploadedSkinCount = 0;
    m_dispatchedVertexCount = 0;
}

void MeshNode3d::Init() {
    SetName("MeshNode3d");
    m_meshSlot = std::make_shared<PropertyHolder>();

    m_meshSlot->AddProperty("mesh_type",
        [this]() -> PropertyValue {
            return m_mesh ? "LoadedMesh" : "None";
        },
        [this](const PropertyValue& v) {
            const std::string name = std::get<std::string>(v);
            SetMesh(ResourceManager::Get().Load<Mesh>(name));
        }
    );

    m_meshSlot->AddProperty("mesh",
        [this]() -> PropertyValue {
            if (!m_mesh) return std::shared_ptr<PropertyHolder>{};
            return std::static_pointer_cast<PropertyHolder>(m_mesh);
        },
        [this](const PropertyValue& v) {
            const auto& holder = std::get<std::shared_ptr<PropertyHolder>>(v);
            if (!holder) {
                SetMesh(nullptr);
            }
        }
    );

    AddProperty("mesh",
        [this]() -> PropertyValue { return m_meshSlot; },
        [this](const PropertyValue& v) {
            const auto& holder = std::get<std::shared_ptr<PropertyHolder>>(v);
            if (!holder) {
                SetMesh(nullptr);
                return;
            }

            for (const auto& [key, prop] : holder->GetProperties()) {
                try {
                    m_meshSlot->Set(key, prop.getter());
                } catch (const std::exception& e) {
                    std::cerr << "[MeshNode3d] mesh property error: " << e.what() << "\n";
                }
            }
        }
    );

    AddProperty("material",
        [this]() -> PropertyValue { return std::static_pointer_cast<PropertyHolder>(m_material); },
        [this](const PropertyValue& v) {
            const auto& holder = std::get<std::shared_ptr<PropertyHolder>>(v);
            if (!holder) {
                m_material = nullptr;
                return;
            }

            if (const auto mat = std::dynamic_pointer_cast<Material>(holder)) {
                m_material = mat;
            }
        }
    );

    AddProperty("is_unique",
        [this]() -> PropertyValue { return m_isUnique; },
        [this](const PropertyValue& v) {
            m_isUnique = std::get<bool>(v);
        }
    );
}

bool MeshNode3d::HasSkinning() const {
    return m_meshHasSkinWeights && m_animator && m_animator->HasSkeleton() && m_animator->GetSkinMatrixCount() > 0;
}

void MeshNode3d::PrepareSkinningForRender() const {
    if (m_animator) {
        m_animator->EvaluateAt(m_animator->GetCurrentTime());
    }
    SyncSkinningBuffer();
}

void MeshNode3d::BindSkinning(const Shader& shader) const {
    if (!HasSkinning()) {
        shader.SetBool("u_UseSkinnedVertexBuffer", false);
        return;
    }

    if (!m_skinnedVertexSsbo) {
        shader.SetBool("u_UseSkinnedVertexBuffer", false);
        return;
    }

    shader.SetBool("u_UseSkinnedVertexBuffer", true);
    m_skinnedVertexSsbo->Bind();
}

float MeshNode3d::ConsumeFrameSkinUploadMs() {
    const float value = s_frameSkinUploadMs;
    s_frameSkinUploadMs = 0.0f;
    return value;
}

const glm::mat4 & MeshNode3d::GetNormalMatrix() const {
    if (m_normalMatrixDirty) {
        m_cachedNormalMatrix = glm::transpose(glm::inverse(GetWorldMatrix()));
        m_normalMatrixDirty = false;
    }

    

    return m_cachedNormalMatrix;
}

void MeshNode3d::SyncSkinningBuffer() const {
    if (!HasSkinning()) {
        return;
    }

    const int skinCount = m_animator->GetSkinMatrixCount();
    if (skinCount <= 0) {
        return;
    }

    const uint64_t poseVersion = m_animator->GetPoseVersion();
    const bool poseChanged = (m_uploadedPoseVersion != poseVersion) || (m_uploadedSkinCount != skinCount);
    if (poseChanged) {
        m_uploadedPoseVersion = poseVersion;
        m_uploadedSkinCount = skinCount;
    }

    if (!m_mesh) {
        return;
    }

    const size_t vertexCount = m_mesh->GetVertexCount();
    if (vertexCount == 0) {
        return;
    }

    const size_t skinnedBytes = vertexCount * sizeof(GpuSkinnedVertex);
    if (!m_skinnedVertexSsbo || m_skinnedVertexSsbo->GetSize() < skinnedBytes) {
        m_skinnedVertexSsbo = std::make_unique<ShaderStorageBuffer>(skinnedBytes, kSkinnedOutputBinding);
        m_dispatchedVertexCount = 0;
    }

    if (!m_skinningComputeShader) {
        m_skinningComputeShader = ResourceManager::Get().LoadComputeShader("SkinningCompute", "skinning.comp");
    }

    const ShaderStorageBuffer* sourceBuffer = m_mesh->GetSkinningSourceBuffer();
    const ShaderStorageBuffer* skinMatricesBuffer = m_animator->GetSkinMatricesSsbo();
    if (!m_skinningComputeShader || !sourceBuffer || !skinMatricesBuffer || !m_skinnedVertexSsbo) {
        return;
    }

    if (!poseChanged && m_dispatchedVertexCount == vertexCount) {
        return;
    }

    sourceBuffer->Bind();
    skinMatricesBuffer->Bind();
    m_skinnedVertexSsbo->Bind();

    const auto uploadStart = std::chrono::high_resolution_clock::now();
    GLint previousProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);

    m_skinningComputeShader->Bind();
    m_skinningComputeShader->SetUint("u_VertexCount", static_cast<unsigned int>(vertexCount));
    m_skinningComputeShader->SetInt("u_SkinMatrixCount", skinCount);
    const unsigned int groupCount = static_cast<unsigned int>((vertexCount + kSkinningComputeWorkGroupSize - 1) / kSkinningComputeWorkGroupSize);
    m_skinningComputeShader->Dispatch(groupCount, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    glUseProgram(static_cast<GLuint>(previousProgram));
    const auto uploadEnd = std::chrono::high_resolution_clock::now();
    s_frameSkinUploadMs += std::chrono::duration<float, std::milli>(uploadEnd - uploadStart).count();
    m_dispatchedVertexCount = vertexCount;
}

void MeshNode3d::RefreshSkinningFlags() {
    m_meshHasSkinWeights = m_mesh && m_mesh->HasSkinWeights();
}
