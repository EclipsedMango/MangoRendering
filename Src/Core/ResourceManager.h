
#ifndef MANGORENDERING_RESOURCEMANAGER_H
#define MANGORENDERING_RESOURCEMANAGER_H

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

class Texture;
class Material;
class Mesh;
class Shader;

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

    template<typename T> std::shared_ptr<T> Get(const std::string& name) {
        auto& cache = GetCache<T>();
        if (auto it = cache.find(name); it != cache.end()) {
            return it->second.lock();
        }

        return nullptr;
    }

    template<typename T> void Register(const std::string& name, const std::shared_ptr<T>& resource) {
        if (!name.empty() && resource) {
            GetCache<T>()[name] = resource;
        }
    }

    std::shared_ptr<Texture> LoadTextureFromMemory(const std::string& key, const unsigned char* data, int width, int height, int channels);
    std::shared_ptr<Material> DuplicateMaterial(const std::string& sourceName, const std::string& newName);
    std::shared_ptr<Shader> LoadShader(const std::string& name, const std::string& vertFilename, const std::string& fragFilename);
    std::shared_ptr<Shader> LoadShaderWithGeom(const std::string& name, const std::string& vertFilename, const std::string& fragFilename, const std::string& geomFileName);
    std::shared_ptr<Shader> LoadComputeShader(const std::string& name, const std::string& computeFilename);

    void SetUserDirectory(const std::filesystem::path& userPath);
    const std::filesystem::path& GetEngineDirectory() const;
    const std::filesystem::path& GetUserDirectory() const;
    void RefreshAssetRegistry();
    std::string ResolveAssetPath(const std::string& filename);

    template<typename T> void Release(const std::string& name) { GetCache<T>().erase(name); }
    void ReleaseAll();
    void CollectGarbage();

private:
    ResourceManager();
    ~ResourceManager();

    void ScanDirectoryIntoRegistry(const fs::path& directory);
    std::string ManualSearch(const fs::path& directory, const std::string& filename);
    std::string SearchAssetFile(const std::string& filename);

    template<typename T> using Cache = std::unordered_map<std::string, std::weak_ptr<T>>;
    template<typename T> void EraseExpired(Cache<T>& cache);

    Cache<Texture> m_textures;
    Cache<Material> m_materials;
    Cache<Mesh> m_meshes;
    Cache<Shader> m_shaders;

    fs::path m_engineAssetsPath;
    fs::path m_userAssetsPath;
    std::unordered_map<std::string, std::string> m_fileRegistry;

    template<typename T> Cache<T>& GetCache();

public:
    template<typename T> std::shared_ptr<T> Load(const std::string& filepath, CacheMode mode = CacheMode::Reuse);
};

template<> ResourceManager::Cache<Texture>& ResourceManager::GetCache<Texture>();
template<> ResourceManager::Cache<Material>& ResourceManager::GetCache<Material>();
template<> ResourceManager::Cache<Mesh>& ResourceManager::GetCache<Mesh>();
template<> ResourceManager::Cache<Shader>& ResourceManager::GetCache<Shader>();

template<> std::shared_ptr<Texture> ResourceManager::Load<Texture>(const std::string& filepath, CacheMode mode);
template<> std::shared_ptr<Material> ResourceManager::Load<Material>(const std::string& filepath, CacheMode mode);
template<> std::shared_ptr<Mesh> ResourceManager::Load<Mesh>(const std::string& filepath, CacheMode mode);
template<> std::shared_ptr<Shader> ResourceManager::Load<Shader>(const std::string& filepath, CacheMode mode);

#endif //MANGORENDERING_RESOURCEMANAGER_H