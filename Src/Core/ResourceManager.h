
#ifndef MANGORENDERING_RESOURCEMANAGER_H
#define MANGORENDERING_RESOURCEMANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <iostream>

#include "Renderer/Materials/Texture.h"
#include "Renderer/Materials/Material.h"
#include "Renderer/Meshes/Mesh.h"
#include "Renderer/Meshes/PrimitiveMesh.h"
#include "Renderer/Shader.h"
#include "Renderer/Meshes/GltfLoader.h"

class ResourceManager {
public:
    enum class CacheMode {
        Reuse,    // return cached version if it exists (default)
        Replace,  // force a fresh load, replacing the cache entry
    };

    static ResourceManager& Get() {
        static ResourceManager instance;
        return instance;
    }

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    // ----- textures -----
    std::shared_ptr<Texture> LoadTexture(const std::string& path, const CacheMode mode = CacheMode::Reuse) {
        if (path.empty()) return nullptr;
        return Load<Texture>(m_textures, path, mode, [&] { return std::make_shared<Texture>(path); });
    }

    std::shared_ptr<Texture> LoadTexture(const std::string& key, const unsigned char* data, int width, int height, int channels, const CacheMode mode = CacheMode::Reuse) {
        if (key.empty()) return nullptr;
        return Load<Texture>(m_textures, key, mode, [&] { return std::make_shared<Texture>(data, width, height, channels, key); });
    }

    // ----- materials -----
    std::shared_ptr<Material> LoadMaterial(const std::string& name, const CacheMode mode = CacheMode::Reuse) {
        return Load<Material>(m_materials, name, mode, [&] { return std::make_shared<Material>(); });
    }

    std::shared_ptr<Material> DuplicateMaterial(const std::string& sourceName) {
        const auto source = LoadMaterial(sourceName);
        return std::make_shared<Material>(*source);
    }

    std::shared_ptr<Mesh> LoadMesh(const std::string& nameOrPath, const CacheMode mode = CacheMode::Reuse) {
        if (nameOrPath.empty() || nameOrPath == "None") return nullptr;

        return Load<Mesh>(m_meshes, nameOrPath, mode, [&]() -> std::shared_ptr<Mesh> {
            if (nameOrPath == "Cube") return std::make_shared<CubeMesh>();
            if (nameOrPath == "Sphere") return std::make_shared<SphereMesh>();
            if (nameOrPath == "Plane") return std::make_shared<PlaneMesh>();
            if (nameOrPath == "Quad") return std::make_shared<QuadMesh>();
            if (nameOrPath == "Cylinder") return std::make_shared<CylinderMesh>();
            if (nameOrPath == "Capsule") return std::make_shared<CapsuleMesh>();
            if (nameOrPath.ends_with(".gltf") || nameOrPath.ends_with(".glb")) {
                return GltfLoader::ExtractFirstMesh(nameOrPath);
            }

            std::cerr << "[ResourceManager] Unknown mesh or primitive: " << nameOrPath << std::endl;
            return nullptr;
        });
    }

    // ----- shaders -----
    std::shared_ptr<Shader> LoadShader(const std::string& name, const std::string& vertPath, const std::string& fragPath, const CacheMode mode = CacheMode::Reuse) {
        return Load<Shader>(m_shaders, name, mode,[&] {
            return std::make_shared<Shader>(vertPath.c_str(), fragPath.c_str());
        });
    }

    std::shared_ptr<Shader> LoadComputeShader(const std::string& name, const std::string& computePath, const CacheMode mode = CacheMode::Reuse) {
        return Load<Shader>(m_shaders, name, mode,[&] {
            return std::make_shared<Shader>(computePath.c_str());
        });
    }

    std::shared_ptr<Shader> GetShader(const std::string& name) {
        if (auto it = m_shaders.find(name); it != m_shaders.end()) {
            return it->second.lock();
        }
        return nullptr;
    }

    // ----- clean up -----
    void ReleaseTexture(const std::string& path) { m_textures.erase(path); }
    void ReleaseMaterial(const std::string& name) { m_materials.erase(name); }
    void ReleaseMesh(const std::string& name) { m_meshes.erase(name); }
    void ReleaseShader(const std::string& name) { m_shaders.erase(name); }

    void ReleaseAll() {
        m_textures.clear();
        m_materials.clear();
        m_meshes.clear();
        m_shaders.clear();
    }

    // remove any entries whose shared_ptr has expired
    void CollectGarbage() {
        EraseExpired(m_textures);
        EraseExpired(m_materials);
        EraseExpired(m_meshes);
        EraseExpired(m_shaders);
    }


private:
    ResourceManager() = default;
    ~ResourceManager() { ReleaseAll(); }

    template<typename T>
    using Cache = std::unordered_map<std::string, std::weak_ptr<T>>;

    // generic loading, handles reuse vs replace and expired weak_ptrs
    template<typename T, typename TFactory>
    std::shared_ptr<T> Load(Cache<T>& cache, const std::string& key, const CacheMode mode, TFactory factory) {
        if (mode == CacheMode::Reuse) {
            if (auto it = cache.find(key); it != cache.end()) {
                if (auto res = it->second.lock()) {
                    return res;
                }
            }
        }

        auto res = factory();
        cache[key] = res;
        return res;
    }

    template<typename T>
    void EraseExpired(Cache<T>& cache) {
        for (auto it = cache.begin(); it != cache.end();) {
            it = it->second.expired() ? cache.erase(it) : ++it;
        }
    }

    Cache<Texture> m_textures;
    Cache<Material> m_materials;
    Cache<Mesh> m_meshes;
    Cache<Shader> m_shaders;
};


#endif //MANGORENDERING_RESOURCEMANAGER_H