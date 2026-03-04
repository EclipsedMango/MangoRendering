
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

#include "GltfLoader.h"
#include "Mesh.h"
#include <iostream>

std::vector<Object*> GltfLoader::Load(const std::string& path, Shader* shader) {
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err, warn;

    const bool success = path.ends_with(".glb") ? loader.LoadBinaryFromFile(&model, &err, &warn, path) : loader.LoadASCIIFromFile(&model, &err, &warn, path);

    if (!warn.empty()) std::cerr << "GLTF Warning: " << warn << std::endl;
    if (!err.empty())  std::cerr << "GLTF Error: "   << err  << std::endl;
    if (!success) return {};

    std::vector<Object*> objects;

    for (auto& gltfMesh : model.meshes) {
        for (auto& primitive : gltfMesh.primitives) {
            // positions
            const auto& posAccessor = model.accessors[primitive.attributes["POSITION"]];
            const auto& posView     = model.bufferViews[posAccessor.bufferView];
            const float* positions = reinterpret_cast<const float*>(
                model.buffers[posView.buffer].data.data() + posView.byteOffset + posAccessor.byteOffset
            );

            // normals
            const float* normals = nullptr;
            if (primitive.attributes.contains("NORMAL")) {
                const auto& normAccessor = model.accessors[primitive.attributes["NORMAL"]];
                const auto& normView     = model.bufferViews[normAccessor.bufferView];
                normals = reinterpret_cast<const float*>(
                    model.buffers[normView.buffer].data.data() + normView.byteOffset + normAccessor.byteOffset
                );
            }

            // uvs
            const float* uvs = nullptr;
            if (primitive.attributes.contains("TEXCOORD_0")) {
                const auto& uvAccessor = model.accessors[primitive.attributes["TEXCOORD_0"]];
                const auto& uvView     = model.bufferViews[uvAccessor.bufferView];
                uvs = reinterpret_cast<const float*>(
                    model.buffers[uvView.buffer].data.data() + uvView.byteOffset + uvAccessor.byteOffset
                );
            }

            if (primitive.material >= 0) {
                auto& mat = model.materials[primitive.material];
                if (mat.doubleSided) {
                    glDisable(GL_CULL_FACE);
                } else {
                    glEnable(GL_CULL_FACE);
                }
            }

            std::vector<Vertex> vertices;
            for (size_t i = 0; i < posAccessor.count; i++) {
                Vertex v{};
                v.position = { positions[i*3], positions[i*3+1], positions[i*3+2] };
                v.normal = normals ? glm::vec3(normals[i*3], normals[i*3+1], normals[i*3+2]) : glm::vec3(0, 1, 0);
                v.texCoord = uvs ? glm::vec2(uvs[i*2], uvs[i*2+1]) : glm::vec2(0, 0);
                vertices.push_back(v);
            }

            // indices
            std::vector<uint32_t> indices;
            const auto& idxAccessor = model.accessors[primitive.indices];
            const auto& idxView     = model.bufferViews[idxAccessor.bufferView];
            const uint8_t* idxData  = model.buffers[idxView.buffer].data.data() + idxView.byteOffset + idxAccessor.byteOffset;

            for (size_t i = 0; i < idxAccessor.count; i++) {
                if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    indices.push_back(reinterpret_cast<const uint16_t*>(idxData)[i]);
                }
                else if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    indices.push_back(reinterpret_cast<const uint32_t*>(idxData)[i]);
                }
            }

            Mesh* mesh = new Mesh(vertices, indices);
            Object* object = new Object(mesh, shader);
            objects.push_back(object);
        }
    }

    return objects;
}
