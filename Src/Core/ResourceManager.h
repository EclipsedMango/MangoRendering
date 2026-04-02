
#ifndef MANGORENDERING_RESOURCEMANAGER_H
#define MANGORENDERING_RESOURCEMANAGER_H

#include <filesystem>
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
#include "SDL3/SDL_filesystem.h"

namespace fs = std::filesystem;

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
    void RegisterTexture(const std::string& name, const std::shared_ptr<Texture>& texture) {
        if (name.empty() || !texture) return;
        m_textures[name] = texture;
    }

    std::shared_ptr<Texture> GetTexture(const std::string& name) {
        if (const auto it = m_textures.find(name); it != m_textures.end()) {
            return it->second.lock();
        }
        return nullptr;
    }

    std::shared_ptr<Texture> LoadTexture(const std::string& filename, const CacheMode mode = CacheMode::Reuse) {
        if (filename.empty()) return nullptr;

        return Load<Texture>(m_textures, filename, mode, [&]() -> std::shared_ptr<Texture> {
            std::string fullPath = SearchAssetFile(filename);
            if (fullPath.empty()) {
                std::cerr << "[ResourceManager] Failed to load Texture: " << filename << std::endl;
                return nullptr;
            }
            return std::make_shared<Texture>(fullPath);
        });
    }

    std::shared_ptr<Texture> LoadTexture(const std::string& key, const unsigned char* data, int width, int height, int channels, const CacheMode mode = CacheMode::Reuse) {
        if (key.empty()) return nullptr;
        return Load<Texture>(m_textures, key, mode, [&] { return std::make_shared<Texture>(data, width, height, channels, key); });
    }

    // ----- materials -----
    void RegisterMaterial(const std::string& name, const std::shared_ptr<Material>& material) {
        if (name.empty() || !material) return;
        m_materials[name] = material;
    }

    std::shared_ptr<Material> GetMaterial(const std::string& name) {
        if (const auto it = m_materials.find(name); it != m_materials.end()) {
            return it->second.lock();
        }
        return nullptr;
    }

    std::shared_ptr<Material> LoadMaterial(const std::string& name, const CacheMode mode = CacheMode::Reuse) {
        return Load<Material>(m_materials, name, mode, [&] { return std::make_shared<Material>(); });
    }

    std::shared_ptr<Material> DuplicateMaterial(const std::string& sourceName) {
        const auto source = LoadMaterial(sourceName);
        return std::make_shared<Material>(*source);
    }

    // ----- meshes -----
    void RegisterMesh(const std::string& name, const std::shared_ptr<Mesh> &mesh) {
        m_meshes[name] = mesh;
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

            const size_t hashPos = nameOrPath.find_last_of('#');
            if (hashPos != std::string::npos && (nameOrPath.find(".gltf") != std::string::npos || nameOrPath.find(".glb") != std::string::npos)) {
                const std::string filename = nameOrPath.substr(0, hashPos);
                std::string indices = nameOrPath.substr(hashPos + 1);
                const size_t underscore = indices.find('_');

                if (underscore != std::string::npos) {
                    const int meshIdx = std::stoi(indices.substr(0, underscore));
                    const int primIdx = std::stoi(indices.substr(underscore + 1));

                    const std::string fullPath = SearchAssetFile(filename);
                    if (!fullPath.empty()) {
                        return GltfLoader::ExtractMesh(fullPath, meshIdx, primIdx);
                    }
                }
            }

            std::cerr << "[ResourceManager] Unknown mesh, primitive, or file missing: " << nameOrPath << std::endl;
            return nullptr;
        });
    }

    // ----- shaders -----
    std::shared_ptr<Shader> LoadShader(const std::string& name, const std::string& vertFilename, const std::string& fragFilename, const CacheMode mode = CacheMode::Reuse) {
        return Load<Shader>(m_shaders, name, mode,[&]() -> std::shared_ptr<Shader> {
            const std::string vertFullPath = SearchAssetFile(vertFilename);
            const std::string fragFullPath = SearchAssetFile(fragFilename);

            if (vertFullPath.empty() || fragFullPath.empty()) {
                std::cerr << "[ResourceManager] Failed to load Shader files for: " << name << std::endl;
                return nullptr;
            }

            return std::make_shared<Shader>(vertFullPath.c_str(), fragFullPath.c_str());
        });
    }

    std::shared_ptr<Shader> LoadShaderWithGeom(const std::string& name, const std::string& vertFilename, const std::string& fragFilename, const std::string& geomFileName, const CacheMode mode = CacheMode::Reuse) {
        return Load<Shader>(m_shaders, name, mode, [&]() -> std::shared_ptr<Shader> {
            const std::string vertFullPath = SearchAssetFile(vertFilename);
            const std::string fragFullPath = SearchAssetFile(fragFilename);
            const std::string geomFullPath = SearchAssetFile(geomFileName);

            if (vertFullPath.empty() || fragFullPath.empty() || geomFullPath.empty()) {
                std::cerr << "[ResourceManager] Failed to load Shader files for: " << name << std::endl;
                return nullptr;
            }

            return std::make_shared<Shader>(vertFullPath.c_str(), fragFullPath.c_str(), geomFullPath.c_str());
        });
    }

    std::shared_ptr<Shader> LoadComputeShader(const std::string& name, const std::string& computeFilename, const CacheMode mode = CacheMode::Reuse) {
        return Load<Shader>(m_shaders, name, mode,[&]() -> std::shared_ptr<Shader> {
            const std::string compFullPath = SearchAssetFile(computeFilename);
            if (compFullPath.empty()) {
                std::cerr << "[ResourceManager] Failed to load Compute Shader: " << computeFilename << std::endl;
                return nullptr;
            }
            return std::make_shared<Shader>(compFullPath.c_str());
        });
    }

    std::shared_ptr<Shader> GetShader(const std::string& name) {
        if (auto it = m_shaders.find(name); it != m_shaders.end()) {
            return it->second.lock();
        }
        return nullptr;
    }

    // ----- fonts -----
    // TODO: fonts will go here (ui stuff and font file related stuff not imgui fonts)

    // ----- Project Management -----
    // called from your Editor when a project is opened/created
    void SetUserDirectory(const std::filesystem::path& userPath) {
        m_userAssetsPath = userPath;
        RefreshAssetRegistry();
    }

    const std::filesystem::path& GetEngineDirectory() const { return m_engineAssetsPath; }
    const std::filesystem::path& GetUserDirectory() const { return m_userAssetsPath; }

    // called if the user adds new files through the content browser
    void RefreshAssetRegistry() {
        m_fileRegistry.clear();

        ScanDirectoryIntoRegistry(m_engineAssetsPath);

        if (!m_userAssetsPath.empty()) {
            ScanDirectoryIntoRegistry(m_userAssetsPath);
        }
    }

    std::string ResolveAssetPath(const std::string& filename) {
        return SearchAssetFile(filename);
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
    ResourceManager() {
        const char* basePathStr = SDL_GetBasePath();
        fs::path searchPath;

        if (basePathStr) {
            searchPath = fs::path(basePathStr);
            // SDL_free((void*)basePathStr);
        } else {
            searchPath = fs::current_path();
        }

        bool foundEngineAssets = false;
        while (searchPath.has_parent_path()) {
            fs::path potentialEngineAssets = searchPath / "EngineAssets";

            if (fs::exists(potentialEngineAssets) && fs::is_directory(potentialEngineAssets)) {
                m_engineAssetsPath = potentialEngineAssets;
                foundEngineAssets = true;
                break;
            }

            searchPath = searchPath.parent_path();
        }

        if (!foundEngineAssets) {
            std::cerr << "[ResourceManager] FATAL: Could not find EngineAssets folder!" << std::endl;
            m_engineAssetsPath = fs::current_path() / "EngineAssets";
        }

        RefreshAssetRegistry();
    }

    ~ResourceManager() { ReleaseAll(); }

    void ScanDirectoryIntoRegistry(const fs::path& directory) {
        if (fs::exists(directory) && fs::is_directory(directory)) {
            for (const auto& entry : fs::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    m_fileRegistry[entry.path().filename().string()] = entry.path().string();
                }
            }
        }
    }

    std::string ManualSearch(const fs::path& directory, const std::string& filename) {
        if (fs::exists(directory) && fs::is_directory(directory)) {
            for (const auto& entry : fs::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file() && entry.path().filename().string() == filename) {
                    m_fileRegistry[filename] = entry.path().string();
                    return entry.path().string();
                }
            }
        }
        return "";
    }

    std::string SearchAssetFile(const std::string& filename) {
        if (fs::exists(filename)) {
            return filename;
        }

        const auto it = m_fileRegistry.find(filename);
        if (it != m_fileRegistry.end()) {
            if (fs::exists(it->second)) {
                return it->second;
            }
        }

        std::string foundPath;
        if (!m_userAssetsPath.empty()) {
            foundPath = ManualSearch(m_userAssetsPath, filename);
        }

        if (foundPath.empty()) {
            foundPath = ManualSearch(m_engineAssetsPath, filename);
        }

        return foundPath;
    }

    // TODO: this really should be changed to handle IDs instead because shared_ptrs are freaky and bad
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
        if (res) {
            cache[key] = res;
        }
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

    fs::path m_engineAssetsPath;
    fs::path m_userAssetsPath;

    std::unordered_map<std::string, std::string> m_fileRegistry;
};


#endif //MANGORENDERING_RESOURCEMANAGER_H